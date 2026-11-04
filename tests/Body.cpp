#include "AllocationGuard.h"
#include "Measure.h"
#include "Render.h"
#include <PluginProcessor.h>
#include <dsp/BodyDrive.h>
#include <dsp/MorphOsc.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <initializer_list>
#include <map>
#include <utility>

namespace
{
// Tune values, as MIDI notes with C3 = 60 (specs/modules/body.md).
constexpr int noteC0 = 24, noteA0 = 33, noteC1 = 36, noteC2 = 48, noteC3 = 60;
constexpr double rate48 = 48000.0;

const std::initializer_list<const char*> bodyIds {
    "body_wave", "body_tune", "body_pitch_amount", "body_pitch_decay", "body_pitch_shape",
    "body_amp_attack", "body_amp_hold", "body_amp_decay", "body_amp_shape",
    "body_drive", "body_filter_cutoff", "body_filter_res", "body_filter_env",
    "body_sweep2_amount", "body_sweep2_decay", "body_start"
};

juce::RangedAudioParameter* findParam (juce::AudioProcessor& plugin, const juce::String& id)
{
    for (auto* p : plugin.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p); ranged != nullptr && ranged->getParameterID() == id)
            return ranged;
    return nullptr;
}

void setParam (juce::AudioProcessor& plugin, const juce::String& id, float plainValue)
{
    auto* p = findParam (plugin, id);
    REQUIRE (p != nullptr);
    p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
}

using Settings = std::initializer_list<std::pair<const char*, float>>;

void set (juce::AudioProcessor& plugin, Settings settings)
{
    for (const auto& [id, value] : settings)
        setParam (plugin, id, value);
}

// The kick before BODY-08. BODY-01, 03 and 04 read the voice itself: at the new defaults the signal passes the
// drive's oversampling filters, which delay and ring, and the extra sweep stage moves the pitch.
struct BarePlugin : PluginProcessor
{
    BarePlugin()
    {
        set (*this, { { "body_drive", 0.0f }, { "body_start", 0.0f }, { "body_sweep2_amount", 0.0f },
                      { "body_pitch_amount", 36.0f }, { "body_pitch_decay", 100.0f }, { "body_pitch_shape", 0.0f } });
    }
};

// No sweep and a long, straight fade: what's left is a steady tone.
const Settings steady { { "body_pitch_amount", 0.0f }, { "body_amp_decay", 2000.0f }, { "body_amp_shape", -100.0f } };

double hzOf (double midi) { return 440.0 * std::pow (2.0, (midi - 69.0) / 12.0); }
double midiOf (double hz) { return 69.0 + 12.0 * std::log2 (hz / 440.0); }
double curve (double x, double shape) { return std::pow (1.0 - x, std::pow (3.0, 1.0 + shape / 100.0)); }

std::vector<float> renderOsc (float wave, double hz, double seconds = 0.5, double rate = rate48)
{
    dsp::MorphOsc osc;
    osc.reset();
    std::vector<float> out (static_cast<size_t> (seconds * rate));
    for (auto& sample : out)
        sample = osc.next (hz, rate, wave);
    return out;
}

const std::initializer_list<float> morphPositions { 0.0f, 0.5f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f };

// Finds the parameter's element in the saved state and removes it.
bool removeParamFromState (juce::XmlElement& xml, const juce::String& id)
{
    for (auto* child : xml.getChildIterator())
    {
        if (child->getStringAttribute ("id") == id)
        {
            xml.removeChildElement (child, true);
            return true;
        }
        if (removeParamFromState (*child, id))
            return true;
    }
    return false;
}
}

// --- BODY-01 Wave ---

TEST_CASE ("body_01a_each_wave_has_its_harmonics")
{
    struct Expected { float wave; double second, third; };
    for (const auto& e : { Expected { 0.0f, 0.0, 0.0 }, Expected { 1.0f, 0.0, 1.0 / 9.0 },
                           Expected { 2.0f, 0.5, 1.0 / 3.0 }, Expected { 3.0f, 0.0, 1.0 / 3.0 } })
    {
        BarePlugin plugin;
        set (plugin, steady);
        set (plugin, { { "body_tune", static_cast<float> (noteA0) }, { "body_wave", e.wave } });
        const auto audio = renderNote (plugin, rate48, 0.8);
        const auto level = [&] (double hz) { return measure::toneLevel (audio, rate48, hz, 0.2, 0.7); };

        INFO ("wave " << e.wave);
        const auto fundamental = level (55.0);
        REQUIRE (fundamental > 0.1);
        for (const auto& [harmonic, expected] : { std::pair { 2.0, e.second }, std::pair { 3.0, e.third } })
        {
            const auto ratio = level (55.0 * harmonic) / fundamental;
            if (expected == 0.0)
                CHECK (ratio < 0.001); // -60 dB
            else
                CHECK (ratio == Catch::Approx (expected).epsilon (0.1));
        }
    }
}

TEST_CASE ("body_01b_between_two_waves_the_knob_crossfades")
{
    const auto secondHarmonic = [] (float wave) {
        const auto audio = renderOsc (wave, 55.0);
        return measure::toneLevel (audio, rate48, 110.0, 0.0, 0.5);
    };
    const auto saw = secondHarmonic (2.0f);
    REQUIRE (saw > 0.1);
    CHECK (secondHarmonic (2.5f) == Catch::Approx ((saw + secondHarmonic (3.0f)) / 2.0).epsilon (0.1));
}

TEST_CASE ("body_01c_waves_share_the_fundamental_and_start_at_zero")
{
    for (auto wave : morphPositions)
    {
        INFO ("wave " << wave);
        const auto audio = renderOsc (wave, 55.0);
        CHECK (std::abs (audio[0]) < 1.0e-4f);
        CHECK (measure::toneLevel (audio, rate48, 55.0, 0.0, 0.5) > 0.5);
    }
}

TEST_CASE ("body_01d_each_wave_peaks_near_one")
{
    for (auto wave : { 0.0f, 1.0f, 2.0f, 3.0f })
    {
        INFO ("wave " << wave);
        const auto p = measure::peak (renderOsc (wave, 55.0));
        CHECK (p >= 0.9f);
        CHECK (p <= 1.0001f);
    }
}

TEST_CASE ("body_01e_every_wave_is_band_limited")
{
    for (auto hz : { 261.6, 4186.0, 12000.0 })
        for (auto wave : morphPositions)
        {
            INFO ("wave " << wave << " at " << hz << " Hz");
            CHECK (measure::inharmonicDb (renderOsc (wave, hz, 0.2), rate48, hz, 1000) < -60.0);
        }
}

TEST_CASE ("body_01f_band_limiting_keeps_the_top_of_a_low_saw")
{
    const auto audio = renderOsc (2.0f, 55.0, 1.0);
    const auto n = 182.0; // 10010 Hz
    const auto ratio = measure::toneLevel (audio, rate48, 55.0 * n, 0.0, 1.0) / measure::toneLevel (audio, rate48, 55.0, 0.0, 1.0);
    CHECK (20.0 * std::log10 (ratio * n) > -3.0);
    CHECK (20.0 * std::log10 (ratio * n) < 3.0);
}

TEST_CASE ("body_01g_a_sweep_does_not_change_the_level")
{
    for (auto wave : { 0.0f, 1.0f, 2.0f, 3.0f })
    {
        const auto fundamental = [wave] (double hz) { return measure::toneLevel (renderOsc (wave, hz), rate48, hz, 0.0, 0.5); };
        const auto low = fundamental (55.0);
        for (auto hz : { 110.0, 220.0, 440.0, 880.0, 1760.0, 3520.0 })
        {
            INFO ("wave " << wave << " at " << hz << " Hz");
            CHECK (fundamental (hz) == Catch::Approx (low).epsilon (0.05));
        }
    }
}

// --- BODY-02 Tune ---

TEST_CASE ("body_02a_each_note_lands_on_its_frequency")
{
    for (int note = noteC0; note <= noteC3; ++note)
    {
        BarePlugin plugin;
        set (plugin, steady);
        setParam (plugin, "body_tune", static_cast<float> (note));
        const auto audio = renderNote (plugin, rate48, 0.5);
        INFO ("note " << note);
        CHECK (measure::frequency (audio, rate48, 0.1, 0.45) == Catch::Approx (hzOf (note)).epsilon (0.005));
    }
}

TEST_CASE ("body_02a_the_played_note_does_not_change_the_kick")
{
    BarePlugin a, b;
    CHECK (renderNote (a, rate48, 0.3, 0, 36) == renderNote (b, rate48, 0.3, 0, 60));
}

TEST_CASE ("body_02b_tune_reads_and_takes_note_names")
{
    BarePlugin plugin;
    auto* tune = findParam (plugin, "body_tune");
    REQUIRE (tune != nullptr);
    for (const auto& [note, name] : { std::pair { 33, "A0" }, std::pair { 30, "F#0" }, std::pair { 36, "C1" } })
    {
        const auto normalised = tune->convertTo0to1 (static_cast<float> (note));
        CHECK (tune->getText (normalised, 32) == name);
        CHECK (tune->getValueForText (name) == Catch::Approx (normalised));
    }
}

TEST_CASE ("body_02c_tune_moves_the_whole_sweep")
{
    const auto sweepHz = [] (int note, double from, double to) {
        BarePlugin plugin;
        set (plugin, { { "body_tune", static_cast<float> (note) }, { "body_pitch_amount", 24.0f }, { "body_pitch_decay", 1000.0f },
                       { "body_amp_decay", 2000.0f }, { "body_amp_shape", -100.0f } });
        return measure::frequency (renderNote (plugin, rate48, 0.4), rate48, from, to);
    };
    CHECK (sweepHz (noteC2, 0.1, 0.14) / sweepHz (noteC1, 0.1, 0.14) == Catch::Approx (2.0).epsilon (0.01));
    CHECK (sweepHz (noteC2, 0.3, 0.34) / sweepHz (noteC1, 0.3, 0.34) == Catch::Approx (2.0).epsilon (0.01));
}

TEST_CASE ("body_02d_readout_says_tune_when_the_pitch_has_arrived")
{
    BarePlugin plugin;
    CHECK (plugin.getTailNote() == Catch::Approx (33.0));
    CHECK (plugin.getTailReadout() == "A0");
}

TEST_CASE ("body_02d_readout_gives_the_note_the_tail_really_lands_on")
{
    BarePlugin plugin;
    set (plugin, { { "body_tune", static_cast<float> (noteC2) }, { "body_pitch_amount", 24.0f }, { "body_pitch_decay", 1000.0f },
                   { "body_amp_decay", 300.0f } });

    // -40 dB on a k = 3 curve: T = 300 ms * (1 - 0.01^(1/3)) = 235.37 ms.
    // Pitch offset there: 24 * (1 - 0.23537)^3 = 10.73 semitones, so 58.73: B2, 27 cents flat.
    const auto expected = noteC2 + 24.0 * curve (0.23537, 0.0);
    CHECK (plugin.getTailNote() == Catch::Approx (expected).margin (0.02));
    CHECK (plugin.getTailReadout() == "B2 -27 ct");

    const auto audio = renderNote (plugin, rate48, 0.3);
    const auto measured = midiOf (measure::frequency (audio, rate48, 0.215, 0.255));
    CHECK (measured == Catch::Approx (plugin.getTailNote()).margin (0.1)); // 10 cents
}

// --- BODY-03 Pitch envelope ---

TEST_CASE ("body_03a_the_sweep_starts_at_tune_plus_amount")
{
    BarePlugin plugin;
    set (plugin, { { "body_tune", static_cast<float> (noteC2) }, { "body_pitch_amount", 24.0f }, { "body_pitch_decay", 1000.0f },
                   { "body_pitch_shape", -100.0f }, { "body_amp_decay", 2000.0f } });
    const auto audio = renderNote (plugin, rate48, 0.1);
    CHECK (midiOf (measure::frequency (audio, rate48, 0.0, 0.01)) == Catch::Approx (noteC2 + 24.0).margin (0.25));
}

TEST_CASE ("body_03a_amount_zero_gives_no_sweep")
{
    BarePlugin plugin;
    set (plugin, steady);
    setParam (plugin, "body_tune", noteC2);
    const auto audio = renderNote (plugin, rate48, 0.1);
    CHECK (measure::frequency (audio, rate48, 0.0, 0.05) == Catch::Approx (hzOf (noteC2)).epsilon (0.005));
}

TEST_CASE ("body_03b_after_pitch_decay_the_pitch_is_tune")
{
    for (auto decayMs : { 10.0f, 100.0f, 1000.0f })
    {
        BarePlugin plugin;
        set (plugin, { { "body_tune", static_cast<float> (noteA0) }, { "body_pitch_amount", 36.0f }, { "body_pitch_decay", decayMs },
                       { "body_amp_decay", 2000.0f }, { "body_amp_shape", -100.0f } });
        const auto decay = decayMs / 1000.0;
        const auto audio = renderNote (plugin, rate48, decay + 0.3);
        INFO ("decay " << decayMs);
        CHECK (measure::frequency (audio, rate48, decay + 0.01, decay + 0.25) == Catch::Approx (55.0).epsilon (0.005));
    }
}

TEST_CASE ("body_03c_pitch_shape_sets_the_curve")
{
    for (auto shape : { -100.0f, 0.0f, 100.0f })
    {
        BarePlugin plugin;
        set (plugin, { { "body_tune", static_cast<float> (noteC2) }, { "body_pitch_amount", 24.0f }, { "body_pitch_decay", 1000.0f },
                       { "body_pitch_shape", shape }, { "body_amp_decay", 2000.0f }, { "body_amp_shape", -100.0f } });
        const auto audio = renderNote (plugin, rate48, 0.6);
        INFO ("shape " << shape);
        const auto measured = midiOf (measure::frequency (audio, rate48, 0.485, 0.515)) - noteC2;
        CHECK (measured == Catch::Approx (24.0 * curve (0.5, shape)).margin (0.5));
    }
}

TEST_CASE ("body_03d_pitch_runs_from_the_note_on_whatever_attack_and_hold_do")
{
    BarePlugin plugin;
    set (plugin, { { "body_tune", static_cast<float> (noteC2) }, { "body_pitch_amount", 24.0f }, { "body_pitch_decay", 1000.0f },
                   { "body_pitch_shape", -100.0f }, { "body_amp_attack", 50.0f }, { "body_amp_hold", 200.0f },
                   { "body_amp_decay", 2000.0f } });
    const auto audio = renderNote (plugin, rate48, 0.4);
    const auto measured = midiOf (measure::frequency (audio, rate48, 0.29, 0.33)) - noteC2;
    CHECK (measured == Catch::Approx (24.0 * curve (0.31, -100.0)).margin (0.5));
}

// --- BODY-04 Amp envelope ---

TEST_CASE ("body_04a_attack_rises_in_a_straight_line")
{
    BarePlugin plugin;
    set (plugin, steady);
    set (plugin, { { "body_tune", static_cast<float> (noteC3) }, { "body_amp_attack", 40.0f } });
    const auto audio = renderNote (plugin, rate48, 0.1);
    CHECK (measure::peakBetween (audio, rate48, 0.018, 0.022) == Catch::Approx (0.5f).margin (0.05f));
    CHECK (measure::peakBetween (audio, rate48, 0.008, 0.012) == Catch::Approx (0.25f).margin (0.05f));
}

TEST_CASE ("body_04a_at_attack_zero_every_wave_starts_at_zero_on_its_sample")
{
    for (auto wave : { 0.0f, 1.0f, 2.0f, 3.0f })
    {
        BarePlugin plugin;
        setParam (plugin, "body_wave", wave);
        const auto audio = renderNote (plugin, rate48, 0.05, 1000);
        INFO ("wave " << wave);
        CHECK (measure::firstSound (audio, 1.0e-3f) >= 1000);
        CHECK (measure::firstSound (audio, 1.0e-3f) <= 1001);
        CHECK (std::abs (audio[1000]) < 1.0e-3f);
    }
}

TEST_CASE ("body_04b_hold_keeps_the_level_up")
{
    BarePlugin plugin;
    set (plugin, steady);
    set (plugin, { { "body_tune", static_cast<float> (noteC3) }, { "body_amp_attack", 10.0f }, { "body_amp_hold", 100.0f } });
    const auto audio = renderNote (plugin, rate48, 0.2);
    for (double t = 0.01; t < 0.105; t += 0.004)
    {
        INFO ("at " << t);
        CHECK (measure::peakBetween (audio, rate48, t, t + 0.004) > 0.95f);
    }
}

TEST_CASE ("body_04c_output_is_zero_from_the_end_of_the_envelope_and_not_before")
{
    for (auto decayMs : { 10.0f, 400.0f, 2000.0f })
    {
        BarePlugin plugin;
        set (plugin, { { "body_amp_attack", 5.0f }, { "body_amp_hold", 20.0f }, { "body_amp_decay", decayMs } });
        const auto end = (5.0 + 20.0 + decayMs) / 1000.0;
        const auto audio = renderNote (plugin, rate48, end + 0.1);
        INFO ("decay " << decayMs);
        CHECK (measure::lastSound (audio) <= static_cast<int> (end * rate48) + 1);
        CHECK (measure::lastSound (audio) >= static_cast<int> ((end - 0.001) * rate48));
    }
}

TEST_CASE ("body_04d_amp_shape_sets_the_curve")
{
    for (auto shape : { -100.0f, 0.0f, 100.0f })
    {
        BarePlugin plugin;
        set (plugin, { { "body_tune", static_cast<float> (noteC3) }, { "body_pitch_amount", 0.0f },
                       { "body_amp_decay", 1000.0f }, { "body_amp_shape", shape } });
        const auto audio = renderNote (plugin, rate48, 0.6);
        INFO ("shape " << shape);
        const auto expected = curve (0.5, shape);
        const auto margin = shape > 0.0f ? 0.005 : 0.1 * expected;
        CHECK (measure::peakBetween (audio, rate48, 0.498, 0.502) == Catch::Approx (expected).margin (margin));
    }
}

TEST_CASE ("body_04e_a_kick_ends_without_a_click")
{
    const auto sineStep = static_cast<float> (2.0 * std::numbers::pi * 55.0 / rate48);
    for (auto decayMs : { 10.0f, 100.0f, 400.0f, 2000.0f })
        for (auto shape : { -100.0f, 0.0f, 100.0f })
        {
            BarePlugin plugin;
            set (plugin, { { "body_tune", static_cast<float> (noteA0) }, { "body_pitch_amount", 0.0f },
                           { "body_amp_decay", decayMs }, { "body_amp_shape", shape } });
            const auto end = decayMs / 1000.0;
            const auto audio = renderNote (plugin, rate48, end + 0.01);
            INFO ("decay " << decayMs << ", shape " << shape);
            CHECK (measure::largestStepBetween (audio, rate48, end - 0.005, end + 0.01) < sineStep);
        }
}

// --- BODY-05 Filter ---

namespace
{
std::vector<float> sineAt (double hz, double rate, double seconds = 0.5)
{
    std::vector<float> out (static_cast<size_t> (seconds * rate));
    for (size_t i = 0; i < out.size(); ++i)
        out[i] = static_cast<float> (std::sin (2.0 * std::numbers::pi * hz * static_cast<double> (i) / rate));
    return out;
}

// Level in dB of a sine after the filter, settled.
double filteredDb (double hz, double cutoff, double rate)
{
    dsp::BodyFilter filter;
    filter.prepare (rate);
    auto x = sineAt (hz, rate);
    for (auto& sample : x)
        sample = filter.process (sample, static_cast<float> (cutoff), 0.0f);
    return 20.0 * std::log10 (measure::toneLevel (x, rate, hz, 0.25, 0.5));
}

// The share of a stretch's energy above `hz`, in 50 Hz bins.
double shareAbove (const std::vector<float>& x, double hz, double from, double to)
{
    const auto start = static_cast<size_t> (from * rate48), size = static_cast<size_t> ((to - from) * rate48);
    double total = 1.0e-30, above = 0.0;
    for (double bin = 50.0; bin < rate48 / 2.0; bin += 50.0)
    {
        const auto power = measure::tonePower (x.data() + start, size, rate48, bin);
        total += power;
        if (bin >= hz)
            above += power;
    }
    return above / total;
}
}

TEST_CASE ("body_05a_at_the_defaults_a_sweep_to_5_khz_keeps_its_level")
{
    BarePlugin plugin;
    set (plugin, { { "body_tune", static_cast<float> (noteC3) }, { "body_pitch_amount", 50.0f }, { "body_pitch_decay", 1000.0f },
                   { "body_pitch_shape", -100.0f }, { "body_amp_hold", 500.0f }, { "body_amp_decay", 2000.0f } });
    const auto audio = renderNote (plugin, rate48, 0.5);
    const auto reference = measure::peak (renderOsc (0.0f, 1000.0, 0.1));
    for (double t = 0.0; t < 0.48; t += 0.02)
    {
        const std::vector<float> stretch (audio.begin() + static_cast<long> (t * rate48), audio.begin() + static_cast<long> ((t + 0.02) * rate48));
        INFO ("at " << t << " s");
        CHECK (std::abs (20.0 * std::log10 (measure::peak (stretch) / reference)) < 0.1);
    }
}

TEST_CASE ("body_05b_slope_and_cutoff_at_every_rate")
{
    for (auto rate : { 44100.0, 48000.0, 192000.0 })
    {
        INFO ("rate " << rate);
        const double cutoff = 500.0;
        CHECK (filteredDb (0.9 * cutoff, cutoff, rate) > -3.0);
        CHECK (filteredDb (1.1 * cutoff, cutoff, rate) < -3.0);
        CHECK (filteredDb (4.0 * cutoff, cutoff, rate) - filteredDb (8.0 * cutoff, cutoff, rate) == Catch::Approx (24.0).margin (3.0));
    }
}

TEST_CASE ("body_05c_env_amount_opens_the_cutoff_and_falls_with_the_amp_decay")
{
    BarePlugin plugin;
    set (plugin, { { "body_wave", 2.0f }, { "body_pitch_amount", 0.0f }, { "body_filter_cutoff", 200.0f }, { "body_filter_env", 5.0f } });
    const auto audio = renderNote (plugin, rate48, 0.4);
    CHECK (shareAbove (audio, 1000.0, 0.0, 0.02) > 0.01);
    CHECK (shareAbove (audio, 1000.0, 0.2, 0.24) < 0.001);
}

TEST_CASE ("body_05d_filter_knobs_do_not_click")
{
    struct Jump { const char* id; float from, to; };
    for (const auto& jump : { Jump { "body_filter_res", 0.0f, 100.0f }, Jump { "body_filter_res", 100.0f, 0.0f },
                              Jump { "body_filter_env", 0.0f, 8.0f }, Jump { "body_filter_env", 8.0f, 0.0f } })
    {
        BarePlugin plugin;
        set (plugin, { { "body_tune", static_cast<float> (noteC1) }, { "body_pitch_amount", 0.0f }, { "body_amp_decay", 2000.0f },
                       { "body_amp_shape", -100.0f }, { "body_filter_cutoff", 400.0f } });
        setParam (plugin, jump.id, jump.from);
        const auto audio = renderNote (plugin, rate48, 0.35, 0, 36, 1.0f, { 256 }, [&, moved = false] (double t) mutable {
            if (t >= 0.2 && ! std::exchange (moved, true))
                setParam (plugin, jump.id, jump.to);
        });
        INFO (jump.id << " from " << jump.from);
        CHECK (measure::largestStepBetween (audio, rate48, 0.2, 0.35) < 0.05f);
    }
}

TEST_CASE ("body_05_and_06_filter_runs_after_drive")
{
    // Before the clip, a filter would leave the clip's harmonics in; after it they are gone.
    BarePlugin plugin;
    set (plugin, steady);
    set (plugin, { { "body_drive", 24.0f }, { "body_filter_cutoff", 100.0f } });
    const auto audio = renderNote (plugin, rate48, 0.5);
    CHECK (measure::toneLevel (audio, rate48, 1375.0, 0.2, 0.4) / measure::toneLevel (audio, rate48, 55.0, 0.2, 0.4) < 0.001);
}

// --- BODY-06 Drive ---

TEST_CASE ("body_06a_at_0_db_a_signal_stays_as_it_was")
{
    for (auto wave : { 0.0f, 2.0f })
    {
        BarePlugin plugin;
        set (plugin, { { "body_wave", wave }, { "body_tune", static_cast<float> (noteA0) }, { "body_pitch_amount", 0.0f },
                       { "body_amp_hold", 500.0f }, { "body_drive", 0.0f } });
        const auto audio = renderNote (plugin, rate48, 0.4);
        const auto bare = renderOsc (wave, hzOf (noteA0), 0.4);
        // 0.01 dB of full scale
        for (size_t i = 0; i < bare.size(); ++i)
            REQUIRE (std::abs (audio[i] - bare[i]) < 0.00115f);
    }
}

TEST_CASE ("body_06a_drive_multiplies_and_clips_at_one")
{
    const auto levelAt = [] (float drive, float velocity) {
        BarePlugin plugin;
        set (plugin, { { "body_tune", static_cast<float> (noteA0) }, { "body_pitch_amount", 0.0f }, { "body_amp_hold", 500.0f }, { "body_drive", drive } });
        return renderNote (plugin, rate48, 0.4, 0, 36, velocity);
    };
    const auto clean = measure::toneLevel (levelAt (0.0f, 0.05f), rate48, 55.0, 0.1, 0.35);
    const auto gained = measure::toneLevel (levelAt (12.0f, 0.05f), rate48, 55.0, 0.1, 0.35);
    CHECK (20.0 * std::log10 (gained / clean) == Catch::Approx (12.0).margin (0.1));

    // A flat top at 1: the filters of the oversampling ring at the corners, so a stretch of the top is read.
    const auto clipped = levelAt (24.0f, 1.0f);
    const auto samplesPerCycle = rate48 / 55.0;
    const auto mid = clipped[static_cast<size_t> (std::round (0.1 * 55.0) * samplesPerCycle + samplesPerCycle / 4.0)];
    CHECK (mid == Catch::Approx (1.0f).margin (0.01f));
    CHECK (measure::peak (clipped) < 1.25f);
}

TEST_CASE ("body_06b_the_clip_makes_odd_harmonics")
{
    BarePlugin plugin;
    set (plugin, { { "body_tune", static_cast<float> (noteA0) }, { "body_pitch_amount", 0.0f }, { "body_amp_hold", 500.0f }, { "body_drive", 12.0f } });
    const auto audio = renderNote (plugin, rate48, 0.45);
    const auto fundamental = measure::toneLevel (audio, rate48, 55.0, 0.1, 0.45);
    CHECK (20.0 * std::log10 (measure::toneLevel (audio, rate48, 165.0, 0.1, 0.45) / fundamental) > -40.0);
    CHECK (20.0 * std::log10 (measure::toneLevel (audio, rate48, 110.0, 0.1, 0.45) / fundamental) < -60.0);
}

TEST_CASE ("body_06c_a_low_tail_stays_clean")
{
    // Set by hand, so this holds for BODY-08's values whatever the defaults become.
    BarePlugin plugin;
    set (plugin, { { "body_pitch_amount", 60.0f }, { "body_pitch_decay", 40.0f }, { "body_pitch_shape", 30.0f },
                   { "body_sweep2_amount", 6.0f }, { "body_sweep2_decay", 300.0f }, { "body_start", 90.0f }, { "body_drive", 6.0f } });
    const auto audio = renderNote (plugin, rate48, 0.8);
    const auto stretch = measure::tailStretch (audio, rate48);
    const auto harmonics = measure::harmonicsDb (audio, rate48, measure::tailHz (audio, rate48), stretch.from, stretch.to);
    REQUIRE (harmonics.size() >= 2);
    CHECK (harmonics[1] < -60.0);
}

TEST_CASE ("body_06d_alias_limit_on_a_low_tone")
{
    // 200 Hz at 48 kHz repeats every 240 samples, so its aliases land on its harmonics and even a bare clip
    // passes. The notes around it have no such luck: G2 is 196 Hz. The measurement's own ceiling is about -90 dB.
    const auto aliasDb = [] (double hz) {
        dsp::BodyDrive drive;
        auto x = sineAt (hz, rate48);
        for (auto& sample : x)
            sample = drive.process (sample, 24.0f);
        return measure::inharmonicDb (x, rate48, hz, 4800);
    };
    const auto bareClipDb = [] (double hz) {
        auto x = sineAt (hz, rate48);
        for (auto& sample : x)
            sample = std::clamp (sample * 15.849f, -1.0f, 1.0f);
        return measure::inharmonicDb (x, rate48, hz, 4800);
    };
    CHECK (aliasDb (200.0) < -80.0);
    CHECK (bareClipDb (196.0) > -80.0); // shows the measurement can fail
    for (int note = 46; note <= 62; ++note) // A#1 to D3: the tails a body plays, and the metric holds from about 120 Hz
    {
        INFO ("note " << note);
        CHECK (aliasDb (hzOf (note)) < -80.0);
    }
}

TEST_CASE ("body_06d_alias_limit_holds_in_the_plugin")
{
    BarePlugin plugin;
    set (plugin, { { "body_tune", 55.0f }, { "body_pitch_amount", 0.0f }, { "body_amp_hold", 500.0f }, { "body_drive", 24.0f } });
    CHECK (measure::inharmonicDb (renderNote (plugin, rate48, 0.45), rate48, hzOf (55.0), 4800) < -80.0);
}

TEST_CASE ("body_06e_drive_adds_no_reported_latency_and_starts_on_the_note")
{
    const auto render = [] (float drive, float velocity) {
        BarePlugin plugin;
        set (plugin, { { "body_pitch_amount", 0.0f }, { "body_drive", drive } });
        CHECK (plugin.getLatencySamples() == 0);
        return renderNote (plugin, rate48, 0.1, 1000, 36, velocity);
    };
    const auto driven = render (24.0f, 1.0f);
    for (int i = 0; i <= 1000; ++i)
        REQUIRE (driven[static_cast<size_t> (i)] == 0.0f);
    CHECK (measure::firstSound (driven) <= 1002);

    // The minimum-phase filters do delay what clips, by less than 0.1 ms: compare where 0.1 is first crossed, unclipped.
    const auto crossing = [] (const std::vector<float>& x) {
        return static_cast<int> (std::find_if (x.begin(), x.end(), [] (float v) { return v > 0.1f; }) - x.begin());
    };
    const auto clean = crossing (render (0.0f, 0.5f));
    const auto wet = crossing (render (24.0f, 0.0315f));
    INFO ("clean " << clean << ", wet " << wet);
    CHECK (wet >= clean - 1);
    CHECK (wet - clean <= 5);
}

TEST_CASE ("body_06f_a_drive_jump_does_not_click")
{
    for (const auto [from, to] : { std::pair { 0.0f, 24.0f }, std::pair { 24.0f, 0.0f } })
    {
        BarePlugin plugin;
        set (plugin, { { "body_tune", static_cast<float> (noteC1) }, { "body_pitch_amount", 0.0f }, { "body_amp_decay", 2000.0f },
                       { "body_amp_shape", -100.0f }, { "body_drive", from } });
        // Velocity 0.05 at 24 dB peaks at 0.79, below clipping.
        const auto audio = renderNote (plugin, rate48, 0.35, 0, 36, 0.05f, { 256 }, [&, moved = false] (double t) mutable {
            if (t >= 0.2 && ! std::exchange (moved, true))
                setParam (plugin, "body_drive", to);
        });
        INFO ("from " << from);
        CHECK (measure::largestStepBetween (audio, rate48, 0.2, 0.35) < 0.05f);
    }
}

TEST_CASE ("eng_04_drive_and_filter_sound_the_same_at_every_sample_rate")
{
    struct Result { double hz; double third; float peak; };
    const auto measureAt = [] (double rate) {
        BarePlugin plugin;
        set (plugin, { { "body_tune", static_cast<float> (noteC1) }, { "body_pitch_amount", 0.0f }, { "body_amp_hold", 400.0f },
                       { "body_drive", 12.0f }, { "body_filter_cutoff", 1000.0f }, { "body_filter_res", 30.0f } });
        const auto audio = renderNote (plugin, rate, 0.4);
        const auto hz = hzOf (noteC1);
        return Result { measure::frequency (audio, rate, 0.1, 0.35), measure::toneLevel (audio, rate, 3.0 * hz, 0.1, 0.35) / measure::toneLevel (audio, rate, hz, 0.1, 0.35), measure::peak (audio) };
    };
    const auto base = measureAt (44100.0);
    for (auto rate : { 48000.0, 96000.0, 192000.0 })
    {
        const auto other = measureAt (rate);
        INFO ("rate " << rate);
        CHECK (other.hz == Catch::Approx (base.hz).epsilon (0.01));
        CHECK (other.third == Catch::Approx (base.third).epsilon (0.02));
        CHECK (other.peak == Catch::Approx (base.peak).epsilon (0.02));
    }
}

// --- BODY-08 New defaults ---

namespace
{
// A render trimmed to time zero, as kikkuh-measure reads it (HARN-05a).
std::vector<float> fromOnset (std::vector<float> x)
{
    const auto start = measure::onset (x);
    REQUIRE (start >= 0);
    x.erase (x.begin(), x.begin() + start);
    return x;
}

double centsBetween (double hz, double reference) { return 1200.0 * std::log2 (hz / reference); }
double stepAtStartDb (const std::vector<float>& x) { return 20.0 * std::log10 (measure::largestStepBetween (x, rate48, 0.0, 0.001) / measure::peak (x)); }

juce::MemoryBlock binaryOf (const juce::XmlElement& xml)
{
    juce::MemoryBlock state;
    juce::AudioProcessor::copyXmlToBinary (xml, state);
    return state;
}

// What a build before VS1-10 saved: version 1, and only the knobs it had.
juce::MemoryBlock oldState (juce::AudioProcessor& plugin, std::initializer_list<const char*> lacking)
{
    juce::MemoryBlock state;
    plugin.getStateInformation (state);
    auto xml = juce::AudioProcessor::getXmlFromBinary (state.getData(), static_cast<int> (state.getSize()));
    REQUIRE (xml != nullptr);
    xml->setAttribute ("version", 1);
    for (const auto* id : lacking)
        REQUIRE (removeParamFromState (*xml, id));
    return binaryOf (*xml);
}

float valueOf (juce::AudioProcessor& plugin, const juce::String& id)
{
    auto* p = findParam (plugin, id);
    REQUIRE (p != nullptr);
    return p->convertFrom0to1 (p->getValue());
}
}

TEST_CASE ("body_08a_a_kick_at_the_defaults_measures_as_the_choice")
{
    PluginProcessor plugin;
    const auto kick = fromOnset (renderNote (plugin, rate48, 1.0));

    for (const auto& [seconds, hz] : { std::pair { 0.02, 89.0 }, std::pair { 0.1, 61.0 }, std::pair { 0.2, 56.0 } })
    {
        const auto measured = measure::pitchAt (kick, rate48, seconds);
        INFO ("at " << seconds << " s: " << measured << " Hz");
        REQUIRE (measured > 0.0);
        CHECK (std::abs (centsBetween (measured, hz)) < 50.0);
    }
    CHECK (measure::secondsBelowDb (kick, rate48, -6.0) == Catch::Approx (0.134).epsilon (0.1));
    CHECK (measure::secondsBelowDb (kick, rate48, -20.0) == Catch::Approx (0.241).epsilon (0.1));
    CHECK (measure::bandShares (kick, rate48, 0.005, 0.02)[1] == Catch::Approx (6.9).margin (2.0));
}

TEST_CASE ("body_08b_loudness_and_first_step_at_the_defaults")
{
    PluginProcessor plugin;
    const auto stereo = renderNoteStereo (plugin, rate48, 1.0);
    const auto channel = [&] (int ch) { return std::vector<float> (stereo.getReadPointer (ch), stereo.getReadPointer (ch) + stereo.getNumSamples()); };
    CHECK (measure::loudness (channel (0), channel (1), rate48) == Catch::Approx (-6.6).margin (0.5));
    CHECK (stepAtStartDb (fromOnset (channel (0))) > -10.0);
}

TEST_CASE ("body_08c_it_ends_without_a_click_with_the_sweep_on")
{
    PluginProcessor plugin;
    const auto audio = renderNote (plugin, rate48, 0.45);
    const auto sineStep = static_cast<float> (2.0 * std::numbers::pi * 55.0 / rate48);
    CHECK (measure::largestStepBetween (audio, rate48, 0.395, 0.45) < sineStep);
}

TEST_CASE ("body_08d_a_project_before_the_second_stage_keeps_its_sound")
{
    // Saved by VS1-01: nine knobs, a shaped kick.
    BarePlugin before;
    set (before, { { "body_tune", static_cast<float> (noteC1) }, { "body_pitch_amount", 40.0f }, { "body_amp_decay", 300.0f } });
    const auto heard = renderNote (before, rate48, 0.5);

    const auto state = oldState (before, { "body_drive", "body_filter_cutoff", "body_filter_res", "body_filter_env",
                                           "body_sweep2_amount", "body_sweep2_decay", "body_start" });
    PluginProcessor restored;
    restored.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
    CHECK (valueOf (restored, "body_sweep2_amount") == 0.0f);
    CHECK (valueOf (restored, "body_start") == 0.0f);
    CHECK (valueOf (restored, "body_drive") == 0.0f);
    CHECK (valueOf (restored, "body_pitch_amount") == Catch::Approx (40.0f).margin (0.01f));
    CHECK (renderNote (restored, rate48, 0.5) == heard);
}

TEST_CASE ("body_08d_a_saved_drive_is_kept")
{
    // Saved by VS1-03: it had Drive, at a value of its own.
    BarePlugin before;
    setParam (before, "body_drive", 12.0f);
    const auto heard = renderNote (before, rate48, 0.5);

    const auto state = oldState (before, { "body_sweep2_amount", "body_sweep2_decay", "body_start" });
    PluginProcessor restored;
    restored.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
    CHECK (valueOf (restored, "body_drive") == Catch::Approx (12.0f).margin (0.01f));
    CHECK (valueOf (restored, "body_sweep2_amount") == 0.0f);
    CHECK (renderNote (restored, rate48, 0.5) == heard);
}

TEST_CASE ("body_08d_a_current_state_is_not_migrated")
{
    PluginProcessor saved;
    setParam (saved, "body_start", 45.0f);
    juce::MemoryBlock state;
    saved.getStateInformation (state);
    auto xml = juce::AudioProcessor::getXmlFromBinary (state.getData(), static_cast<int> (state.getSize()));
    REQUIRE (xml != nullptr);
    CHECK (xml->getIntAttribute ("version") == 2);

    // A knob lacking from a state of this version takes its default (ENG-06a), and a saved value stays.
    REQUIRE (removeParamFromState (*xml, "body_drive"));
    const auto lacking = binaryOf (*xml);
    PluginProcessor restored;
    restored.setStateInformation (lacking.getData(), static_cast<int> (lacking.getSize()));
    CHECK (valueOf (restored, "body_drive") == Catch::Approx (6.0f).margin (0.01f));
    CHECK (valueOf (restored, "body_start") == Catch::Approx (45.0f).margin (0.5f));
}

// --- BODY-09 A sweep in two stages ---

TEST_CASE ("body_09a_the_pitch_follows_both_stages")
{
    BarePlugin plugin;
    set (plugin, { { "body_tune", static_cast<float> (noteC2) }, { "body_pitch_amount", 12.0f }, { "body_pitch_decay", 50.0f },
                   { "body_sweep2_amount", 3.0f }, { "body_sweep2_decay", 650.0f }, { "body_amp_decay", 2000.0f }, { "body_amp_shape", -100.0f } });
    const auto audio = renderNote (plugin, rate48, 0.9);
    for (const auto ms : { 10.0, 75.0, 300.0, 500.0 })
    {
        const auto first = ms < 50.0 ? 12.0 * curve (ms / 50.0, 0.0) : 0.0;
        const auto expected = noteC2 + first + 3.0 * curve (ms / 650.0, 0.0);
        const auto measured = measure::pitchAt (audio, rate48, ms / 1000.0);
        INFO ("at " << ms << " ms: " << measured << " Hz");
        REQUIRE (measured > 0.0);
        CHECK (midiOf (measured) == Catch::Approx (expected).margin (0.5));
    }
}

TEST_CASE ("body_09b_the_kick_ends_on_tune_once_both_have_passed")
{
    BarePlugin plugin;
    set (plugin, { { "body_pitch_amount", 12.0f }, { "body_pitch_decay", 50.0f }, { "body_sweep2_amount", 6.0f }, { "body_sweep2_decay", 300.0f },
                   { "body_amp_hold", 500.0f }, { "body_amp_decay", 2000.0f } });
    const auto audio = renderNote (plugin, rate48, 0.9);
    CHECK (measure::frequency (audio, rate48, 0.31, 0.85) == Catch::Approx (55.0).epsilon (0.005));
}

TEST_CASE ("body_09c_the_readout_takes_the_second_stage_into_account")
{
    BarePlugin plugin;
    set (plugin, { { "body_tune", static_cast<float> (noteC2) }, { "body_pitch_amount", 0.0f }, { "body_sweep2_amount", 12.0f },
                   { "body_sweep2_decay", 1000.0f }, { "body_amp_decay", 300.0f } });

    // -40 dB on a k = 3 curve lies at 235.37 ms, as in BODY-02d.
    const auto expected = noteC2 + 12.0 * curve (0.23537, 0.0);
    CHECK (plugin.getTailNote() == Catch::Approx (expected).margin (0.02));

    const auto audio = renderNote (plugin, rate48, 0.3);
    CHECK (midiOf (measure::frequency (audio, rate48, 0.215, 0.255)) == Catch::Approx (plugin.getTailNote()).margin (0.1)); // 10 cents
}

TEST_CASE ("body_09c_the_readout_says_tune_when_both_have_arrived")
{
    PluginProcessor plugin; // the defaults: -40 dB comes at 313.8 ms, the second stage ends at 300 ms
    CHECK (plugin.getTailNote() == Catch::Approx (33.0));
    CHECK (plugin.getTailReadout() == "A0");

    // One stage done and the other not: the finished one adds nothing.
    set (plugin, { { "body_pitch_decay", 1.0f }, { "body_sweep2_decay", 2000.0f } });
    CHECK (plugin.getTailNote() > 33.0);
    set (plugin, { { "body_pitch_decay", 1000.0f }, { "body_sweep2_decay", 1.0f } });
    CHECK (plugin.getTailNote() > 33.0);
    CHECK (std::isfinite (plugin.getTailNote()));
}

TEST_CASE ("body_09d_a_second_stage_jump_settles_within_50_ms")
{
    for (auto rate : { 44100.0, 192000.0 })
    {
        INFO ("rate " << rate);
        BarePlugin amount;
        set (amount, steady);
        set (amount, { { "body_tune", static_cast<float> (noteC1) }, { "body_sweep2_decay", 2000.0f } });
        const auto rise = renderNote (amount, rate, 0.4, 0, 36, 1.0f, { 256 }, [&, moved = false] (double t) mutable {
            if (t >= 0.2 && ! std::exchange (moved, true))
                setParam (amount, "body_sweep2_amount", 24.0f);
        });
        CHECK (midiOf (measure::frequency (rise, rate, 0.26, 0.28)) == Catch::Approx (noteC1 + 24.0 * curve (0.135, 0.0)).margin (0.24 + 0.1));

        BarePlugin decay;
        set (decay, steady);
        set (decay, { { "body_tune", static_cast<float> (noteC1) }, { "body_sweep2_amount", 12.0f }, { "body_sweep2_decay", 2000.0f } });
        const auto fall = renderNote (decay, rate, 0.4, 0, 36, 1.0f, { 256 }, [&, moved = false] (double t) mutable {
            if (t >= 0.2 && ! std::exchange (moved, true))
                setParam (decay, "body_sweep2_decay", 1.0f);
        });
        CHECK (midiOf (measure::frequency (fall, rate, 0.26, 0.35)) == Catch::Approx (noteC1).margin (0.24));
    }
}

// --- BODY-10 Start phase ---

TEST_CASE ("body_10a_start_is_the_phase_at_the_note_on")
{
    const auto firstSample = [] (float start, float wave = 0.0f) {
        BarePlugin plugin;
        set (plugin, { { "body_start", start }, { "body_wave", wave } });
        return renderNote (plugin, rate48, 0.05, 1000)[1000];
    };
    CHECK (firstSample (90.0f) == Catch::Approx (1.0f).margin (0.001f));
    CHECK (firstSample (45.0f) == Catch::Approx (std::sqrt (0.5f)).margin (0.001f));
    CHECK (std::abs (firstSample (0.0f)) < 1.0e-3f);

    // Every wave: 240 Hz has 200 samples per cycle, so sample 50 of a run from zero is a quarter cycle in.
    for (auto wave : { 1.0f, 2.0f, 3.0f })
    {
        INFO ("wave " << wave);
        dsp::MorphOsc osc;
        osc.reset (0.25);
        CHECK (osc.next (240.0, rate48, wave) == Catch::Approx (renderOsc (wave, 240.0)[50]).margin (1.0e-3f));
    }
}

TEST_CASE ("body_10a_a_start_at_90_steps_at_the_first_millisecond")
{
    BarePlugin plugin;
    setParam (plugin, "body_start", 90.0f);
    CHECK (stepAtStartDb (fromOnset (renderNote (plugin, rate48, 0.2))) > -10.0);
}

TEST_CASE ("body_10b_a_single_hit_from_silence_on_the_notes_sample")
{
    const auto check = [] (juce::AudioProcessor& plugin, int latestFirst) {
        const auto audio = renderNote (plugin, rate48, 0.05, 1000);
        for (size_t i = 0; i < 1000; ++i)
            REQUIRE (audio[i] == 0.0f);
        CHECK (measure::firstSound (audio) >= 1000);
        CHECK (measure::firstSound (audio) <= latestFirst);
    };
    BarePlugin bare;
    setParam (bare, "body_start", 90.0f);
    check (bare, 1000);

    PluginProcessor defaults; // through the drive's filters: BODY-06e's two samples
    check (defaults, 1002);
}

TEST_CASE ("body_10c_eng_03_holds_at_every_start")
{
    for (auto start : { 0.0f, 37.0f, 90.0f })
    {
        INFO ("start " << start);
        PluginProcessor a, b;
        setParam (a, "body_start", start);
        setParam (b, "body_start", start);
        CHECK (renderNote (a, rate48, 0.3) == renderNote (b, rate48, 0.3, 0, 36, 1.0f, { 1, 37, 512 }));
    }
}

TEST_CASE ("body_10c_eng_04_holds_at_every_start")
{
    struct Result { double hz; float peak; };
    for (auto start : { 0.0f, 37.0f, 90.0f })
        for (auto drive : { 0.0f, 6.0f })
        {
            const auto measureAt = [&] (double rate) {
                PluginProcessor plugin;
                set (plugin, { { "body_start", start }, { "body_drive", drive } });
                const auto audio = renderNote (plugin, rate, 0.45);
                return Result { measure::frequency (audio, rate, 0.31, 0.39), measure::peak (audio) };
            };
            const auto base = measureAt (44100.0);
            for (auto rate : { 48000.0, 96000.0, 192000.0 })
            {
                const auto other = measureAt (rate);
                INFO ("start " << start << ", drive " << drive << ", rate " << rate);
                CHECK (other.hz == Catch::Approx (base.hz).epsilon (0.01));
                CHECK (other.peak == Catch::Approx (base.peak).epsilon (0.01));
            }
        }
}

// --- ENG-01, ENG-03, ENG-04, ENG-06 with BODY ---

TEST_CASE ("eng_01a_every_body_parameter_is_automatable")
{
    BarePlugin plugin;
    for (auto* id : bodyIds)
    {
        INFO (id);
        auto* p = findParam (plugin, id);
        REQUIRE (p != nullptr);
        CHECK (p->isAutomatable());
    }
}

TEST_CASE ("eng_01b_a_knob_jump_settles_within_50_ms")
{
    for (auto rate : { 44100.0, 192000.0 })
    {
        BarePlugin tune;
        set (tune, steady);
        setParam (tune, "body_tune", noteC1);
        const auto glide = renderNote (tune, rate, 0.4, 0, 36, 1.0f, { 256 }, [&, moved = false] (double t) mutable {
            if (t >= 0.2 && ! std::exchange (moved, true))
                setParam (tune, "body_tune", noteC3);
        });
        INFO ("rate " << rate);
        // 1 % of the 36-semitone range.
        CHECK (midiOf (measure::frequency (glide, rate, 0.26, 0.35)) == Catch::Approx (noteC3).margin (0.36));

        BarePlugin wave;
        set (wave, steady);
        const auto morph = renderNote (wave, rate, 0.6, 0, 36, 1.0f, { 256 }, [&, moved = false] (double t) mutable {
            if (t >= 0.2 && ! std::exchange (moved, true))
                setParam (wave, "body_wave", 1.0f);
        });
        CHECK (measure::toneLevel (morph, rate, 165.0, 0.26, 0.56) / measure::toneLevel (morph, rate, 55.0, 0.26, 0.56)
               == Catch::Approx (1.0 / 9.0).epsilon (0.1));
    }
}

TEST_CASE ("eng_01b_a_knob_moved_between_kicks_holds_from_the_first_sample")
{
    BarePlugin plugin;
    set (plugin, steady);
    setParam (plugin, "body_tune", noteC1);
    const auto audio = renderNote (plugin, rate48, 0.4, static_cast<int> (0.3 * rate48), 36, 1.0f, { 256 }, [&, moved = false] (double t) mutable {
        if (t >= 0.2 && ! std::exchange (moved, true))
            setParam (plugin, "body_tune", noteC3);
    });
    CHECK (measure::frequency (audio, rate48, 0.3, 0.32) == Catch::Approx (hzOf (noteC3)).epsilon (0.005));
}

TEST_CASE ("eng_01c_a_full_range_knob_jump_does_not_click")
{
    struct Jump { const char* id; float from, to; double atSeconds; };
    for (const auto& jump : { Jump { "body_wave", 0.0f, 1.0f, 0.2 },
                              Jump { "body_tune", 24.0f, 60.0f, 0.2 },
                              Jump { "body_pitch_amount", 0.0f, 72.0f, 0.5 },
                              Jump { "body_pitch_decay", 1000.0f, 1.0f, 0.5 },
                              Jump { "body_pitch_shape", 100.0f, -100.0f, 0.5 },
                              Jump { "body_amp_attack", 50.0f, 0.0f, 0.02 },
                              Jump { "body_amp_hold", 500.0f, 0.0f, 0.2 },
                              Jump { "body_amp_decay", 2000.0f, 10.0f, 0.2 },
                              Jump { "body_amp_shape", 100.0f, -100.0f, 0.2 },
                              Jump { "body_filter_cutoff", 20000.0f, 20.0f, 0.2 },
                              Jump { "body_filter_cutoff", 20.0f, 20000.0f, 0.2 },
                              Jump { "body_sweep2_amount", 0.0f, 24.0f, 0.5 },
                              Jump { "body_sweep2_amount", 24.0f, 0.0f, 0.5 },
                              Jump { "body_sweep2_decay", 2000.0f, 1.0f, 0.5 },
                              Jump { "body_sweep2_decay", 1.0f, 2000.0f, 0.5 } })
    {
        BarePlugin plugin;
        set (plugin, { { "body_tune", static_cast<float> (noteC1) }, { "body_pitch_amount", 12.0f }, { "body_pitch_decay", 1000.0f },
                       { "body_pitch_shape", 100.0f }, { "body_amp_decay", 2000.0f }, { "body_amp_shape", -100.0f } });
        const auto isSweepKnob = juce::String (jump.id).startsWith ("body_pitch");
        if (! isSweepKnob)
            setParam (plugin, "body_pitch_amount", 0.0f);
        if (juce::String (jump.id).startsWith ("body_sweep2")) // the second stage still running at a low pitch
            set (plugin, { { "body_sweep2_amount", 12.0f }, { "body_sweep2_decay", 2000.0f } });
        setParam (plugin, jump.id, jump.from);

        const auto audio = renderNote (plugin, rate48, jump.atSeconds + 0.15, 0, 36, 1.0f, { 256 }, [&, moved = false] (double t) mutable {
            if (t >= jump.atSeconds && ! std::exchange (moved, true))
                setParam (plugin, jump.id, jump.to);
        });
        INFO (jump.id);
        CHECK (measure::largestStepBetween (audio, rate48, jump.atSeconds, jump.atSeconds + 0.15) < 0.05f);
    }
}

TEST_CASE ("eng_01d_process_block_does_not_allocate_while_knobs_move")
{
    BarePlugin plugin;
    plugin.prepareToPlay (rate48, 512);
    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;

    int allocations = 0;
    for (int block = 0; block < 200; ++block)
    {
        for (auto* id : bodyIds)
            if (auto* p = findParam (plugin, id))
                p->setValueNotifyingHost (static_cast<float> (block % 10) / 9.0f);
        buffer.setSize (2, block % 2 == 0 ? 512 : 37, false, false, true);
        midi.clear();
        if (block % 8 == 0)
            midi.addEvent (juce::MidiMessage::noteOn (1, 36, 1.0f), 0);

        AllocationGuard guard;
        plugin.processBlock (buffer, midi);
        allocations += guard.count();
    }
    CHECK (allocations == 0);
}

TEST_CASE ("eng_01e_knob_text_survives_a_round_trip")
{
    BarePlugin plugin;
    for (auto* id : bodyIds)
    {
        auto* p = findParam (plugin, id);
        REQUIRE (p != nullptr);
        for (int i = 0; i <= 1000; ++i)
        {
            const auto text = p->getText (static_cast<float> (i) / 1000.0f, 32);
            INFO (id << " at " << i << ": " << text);
            CHECK (p->getText (p->getValueForText (text), 32) == text);
        }
    }

    CHECK (findParam (plugin, "body_amp_decay")->getText (findParam (plugin, "body_amp_decay")->convertTo0to1 (87.0f), 32) == "87.0");
    CHECK (findParam (plugin, "body_pitch_amount")->getText (findParam (plugin, "body_pitch_amount")->convertTo0to1 (36.0f), 32) == "36.0");
    CHECK (findParam (plugin, "body_amp_shape")->getText (findParam (plugin, "body_amp_shape")->convertTo0to1 (-30.0f), 32) == "-30");
    CHECK (findParam (plugin, "body_wave")->getText (findParam (plugin, "body_wave")->convertTo0to1 (1.5f), 32) == "1.50");
    CHECK (findParam (plugin, "body_drive")->getText (findParam (plugin, "body_drive")->convertTo0to1 (6.0f), 32) == "6.0");
    CHECK (findParam (plugin, "body_filter_cutoff")->getText (findParam (plugin, "body_filter_cutoff")->convertTo0to1 (632.0f), 32) == "632");
    CHECK (findParam (plugin, "body_filter_res")->getText (findParam (plugin, "body_filter_res")->convertTo0to1 (30.0f), 32) == "30");
    CHECK (findParam (plugin, "body_filter_env")->getText (findParam (plugin, "body_filter_env")->convertTo0to1 (2.5f), 32) == "2.5");
    CHECK (findParam (plugin, "body_sweep2_amount")->getText (findParam (plugin, "body_sweep2_amount")->convertTo0to1 (6.0f), 32) == "6.0");
    CHECK (findParam (plugin, "body_sweep2_decay")->getText (findParam (plugin, "body_sweep2_decay")->convertTo0to1 (300.0f), 32) == "300.0");
    CHECK (findParam (plugin, "body_start")->getText (findParam (plugin, "body_start")->convertTo0to1 (90.0f), 32) == "90");
}

TEST_CASE ("eng_03_same_settings_render_identically_at_any_block_size")
{
    const Settings shaped { { "body_wave", 1.5f }, { "body_tune", 40.0f }, { "body_pitch_amount", 50.0f }, { "body_pitch_decay", 37.0f },
                            { "body_pitch_shape", 40.0f }, { "body_amp_attack", 3.0f }, { "body_amp_hold", 11.0f },
                            { "body_amp_decay", 150.0f }, { "body_amp_shape", -30.0f }, { "body_drive", 12.0f },
                            { "body_filter_cutoff", 900.0f }, { "body_filter_res", 30.0f }, { "body_filter_env", 3.0f } };
    BarePlugin a, b;
    set (a, shaped);
    set (b, shaped);
    CHECK (renderNote (a, rate48, 0.3) == renderNote (b, rate48, 0.3, 0, 36, 1.0f, { 1, 37, 512 }));
}

TEST_CASE ("eng_04_shaped_kicks_sound_the_same_at_every_sample_rate")
{
    struct Result { double seconds, hz; float peak; };
    const auto measureAt = [] (double rate) {
        BarePlugin plugin;
        set (plugin, { { "body_wave", 1.0f }, { "body_tune", static_cast<float> (noteC1) }, { "body_pitch_decay", 30.0f },
                       { "body_amp_attack", 5.0f }, { "body_amp_hold", 20.0f }, { "body_amp_decay", 300.0f } });
        const auto audio = renderNote (plugin, rate, 0.5);
        return Result { measure::lastSound (audio) / rate, measure::frequency (audio, rate, 0.1, 0.25), measure::peak (audio) };
    };
    const auto base = measureAt (44100.0);
    for (auto rate : { 48000.0, 96000.0, 192000.0 })
    {
        const auto other = measureAt (rate);
        INFO ("rate " << rate);
        CHECK (other.seconds == Catch::Approx (base.seconds).epsilon (0.01));
        CHECK (other.hz == Catch::Approx (base.hz).epsilon (0.01));
        CHECK (other.peak == Catch::Approx (base.peak).epsilon (0.01));
    }
}

TEST_CASE ("eng_06a_state_restores_every_parameter")
{
    PluginProcessor saved;
    std::map<juce::String, float> values;
    float position = 0.1f;
    for (auto* id : bodyIds)
    {
        auto* p = findParam (saved, id);
        REQUIRE (p != nullptr);
        p->setValueNotifyingHost (position);
        values[id] = p->getValue();
        position += 0.08f;
    }
    juce::MemoryBlock state;
    saved.getStateInformation (state);

    PluginProcessor restored;
    restored.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
    for (auto* id : bodyIds)
    {
        INFO (id);
        CHECK (findParam (restored, id)->getValue() == Catch::Approx (values[id]));
    }
}

TEST_CASE ("eng_06a_a_missing_parameter_takes_its_default")
{
    PluginProcessor saved;
    setParam (saved, "body_tune", noteC2);
    setParam (saved, "body_amp_decay", 123.0f);
    juce::MemoryBlock state;
    saved.getStateInformation (state);
    auto xml = juce::AudioProcessor::getXmlFromBinary (state.getData(), static_cast<int> (state.getSize()));
    REQUIRE (xml != nullptr);
    REQUIRE (removeParamFromState (*xml, "body_tune"));
    juce::MemoryBlock withoutTune;
    juce::AudioProcessor::copyXmlToBinary (*xml, withoutTune);

    PluginProcessor restored;
    setParam (restored, "body_tune", noteC3);
    restored.setStateInformation (withoutTune.getData(), static_cast<int> (withoutTune.getSize()));
    auto* tune = findParam (restored, "body_tune");
    CHECK (tune->getValue() == Catch::Approx (tune->getDefaultValue()));
    CHECK (findParam (restored, "body_amp_decay")->convertFrom0to1 (findParam (restored, "body_amp_decay")->getValue())
           == Catch::Approx (123.0f).margin (0.5f));
}

TEST_CASE ("eng_06a_a_state_without_parameters_gives_defaults")
{
    // What a VS0-01 project saved.
    juce::XmlElement old ("KikkuhState");
    old.setAttribute ("version", 1);
    juce::MemoryBlock state;
    juce::AudioProcessor::copyXmlToBinary (old, state);

    PluginProcessor plugin;
    setParam (plugin, "body_tune", noteC3);
    plugin.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
    auto* tune = findParam (plugin, "body_tune");
    CHECK (tune->getValue() == Catch::Approx (tune->getDefaultValue()));
    CHECK (valueOf (plugin, "body_drive") == Catch::Approx (6.0f).margin (0.01f)); // no knobs saved, so no sound of its own to keep
}

TEST_CASE ("eng_06b_parameter_ids_and_defaults_are_as_specified")
{
    PluginProcessor plugin;
    CHECK (plugin.getParameters().size() == static_cast<int> (bodyIds.size()));

    const std::map<juce::String, float> defaults {
        { "body_wave", 0.0f }, { "body_tune", static_cast<float> (noteA0) }, { "body_pitch_amount", 60.0f },
        { "body_pitch_decay", 40.0f }, { "body_pitch_shape", 30.0f }, { "body_amp_attack", 0.0f },
        { "body_amp_hold", 0.0f }, { "body_amp_decay", 400.0f }, { "body_amp_shape", 0.0f },
        { "body_drive", 6.0f }, { "body_filter_cutoff", 20000.0f }, { "body_filter_res", 0.0f }, { "body_filter_env", 0.0f },
        { "body_sweep2_amount", 6.0f }, { "body_sweep2_decay", 300.0f }, { "body_start", 90.0f }
    };
    for (const auto& [id, value] : defaults)
    {
        INFO (id);
        auto* p = findParam (plugin, id);
        REQUIRE (p != nullptr);
        CHECK (p->convertFrom0to1 (p->getDefaultValue()) == Catch::Approx (value).margin (0.01f));
    }
}
