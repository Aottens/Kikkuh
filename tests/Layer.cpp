// LAYER: specs/modules/layer.md, the CLICK and TOP sections.
#include "AllocationGuard.h"
#include "Measure.h"
#include "Render.h"
#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <initializer_list>
#include <map>
#include <numbers>
#include <random>
#include <tuple>
#include <utility>

namespace
{
constexpr double rate48 = 48000.0;
constexpr float levelOff = -60.0f; // the bottom of Level's range is Off
constexpr int tone = 0, noise = 1, sample = 2;
constexpr int clickSection = 0, topSection = 1;

const std::initializer_list<const char*> layerIds {
    "click_source", "click_level", "click_tune", "click_decay", "click_hp", "click_lp",
    "top_source", "top_level", "top_tune", "top_decay", "top_hp", "top_lp",
    "click_start", "top_start"
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

using Settings = std::vector<std::pair<const char*, float>>;

void set (juce::AudioProcessor& plugin, const Settings& settings)
{
    for (const auto& [id, value] : settings)
        setParam (plugin, id, value);
}

struct Hit
{
    double seconds;
    int note = 36, velocity = 127;
};

std::vector<float> renderHits (juce::AudioProcessor& plugin, double rate, double seconds, const std::vector<Hit>& hits,
                               std::vector<int> blockSizes = { 256 }, const std::function<void (double)>& beforeBlock = {})
{
    std::vector<MidiAt> events;
    for (const auto& hit : hits)
        events.push_back ({ static_cast<int> (std::lround (hit.seconds * rate)),
                            juce::MidiMessage::noteOn (1, hit.note, static_cast<juce::uint8> (hit.velocity)) });
    const auto out = renderEventsStereo (plugin, rate, seconds, events, std::move (blockSizes), beforeBlock);
    return { out.getReadPointer (0), out.getReadPointer (0) + out.getNumSamples() };
}

// The kick with `settings`, BODY at its defaults unless `settings` says otherwise.
std::vector<float> kick (const Settings& settings, double rate, double seconds, const std::vector<Hit>& hits = { { 0.0 } },
                         std::vector<int> blockSizes = { 256 }, const std::function<void (juce::AudioProcessor&, double)>& beforeBlock = {})
{
    PluginProcessor plugin;
    set (plugin, settings);
    return renderHits (plugin, rate, seconds, hits, std::move (blockSizes),
                       beforeBlock ? std::function<void (double)> ([&] (double t) { beforeBlock (plugin, t); }) : std::function<void (double)> {});
}

std::vector<float> minus (const std::vector<float>& a, const std::vector<float>& b)
{
    std::vector<float> out (a.size());
    for (size_t i = 0; i < a.size(); ++i)
        out[i] = a[i] - b[i];
    return out;
}

// "The layer": what a render with the sections as set adds to the same render with both at Level Off.
std::vector<float> layer (Settings settings, double rate, double seconds, const std::vector<Hit>& hits = { { 0.0 } },
                          std::vector<int> blockSizes = { 256 }, const std::function<void (juce::AudioProcessor&, double)>& beforeBlock = {})
{
    const auto on = kick (settings, rate, seconds, hits, blockSizes, beforeBlock);
    settings.push_back ({ "click_level", levelOff });
    settings.push_back ({ "top_level", levelOff });
    const auto off = kick (settings, rate, seconds, hits, blockSizes, {});
    return minus (on, off);
}

// CLICK alone on a steady Tone: long, slow and open.
Settings clickTone (float level = -6.0f, float tune = 0.0f, float decayMs = 2000.0f)
{
    return { { "click_source", tone }, { "click_level", level }, { "click_tune", tune }, { "click_decay", decayMs } };
}

double db (double gain) { return 20.0 * std::log10 (gain); }

// The same envelope on white noise, for the shape of its spectrum.
std::vector<float> whiteNoise (double rate, double seconds, double decaySeconds)
{
    std::mt19937 random (7u);
    std::uniform_real_distribution<float> uniform (-1.0f, 1.0f);
    std::vector<float> x (static_cast<size_t> (seconds * rate));
    for (size_t i = 0; i < x.size(); ++i)
    {
        const auto t = static_cast<double> (i) / rate;
        x[i] = t < decaySeconds ? uniform (random) * static_cast<float> (std::pow (1.0 - t / decaySeconds, 2.0)) : 0.0f;
    }
    return x;
}

// --- LAYER-04 helpers: your own sample ---

std::vector<float> sine (double hz, double rate, double seconds, float amplitude = 0.5f)
{
    std::vector<float> x (static_cast<size_t> (seconds * rate));
    for (size_t i = 0; i < x.size(); ++i)
        x[i] = amplitude * static_cast<float> (std::sin (2.0 * std::numbers::pi * hz * static_cast<double> (i) / rate));
    return x;
}

// LAYER-04a/b: files given to a section, read and handed over before the test plays on.
void load (PluginProcessor& plugin, int section, const juce::File& file)
{
    REQUIRE (plugin.loadSample (section, juce::StringArray { file.getFullPathName() }));
    plugin.waitForSampleLoads();
}

// The kick with `file` in CLICK and `settings` on top of what the drop set.
std::vector<float> kickWithSample (const juce::File& file, const Settings& settings, double rate, double seconds,
                                   const std::vector<Hit>& hits = { { 0.0 } }, std::vector<int> blockSizes = { 256 })
{
    PluginProcessor plugin;
    load (plugin, clickSection, file);
    set (plugin, settings);
    return renderHits (plugin, rate, seconds, hits, std::move (blockSizes));
}

// What CLICK on `file` adds: the same render with CLICK at Off subtracted.
std::vector<float> sampleLayer (const juce::File& file, Settings settings, double rate, double seconds, const std::vector<Hit>& hits = { { 0.0 } })
{
    const auto on = kickWithSample (file, settings, rate, seconds, hits);
    settings.push_back ({ "click_level", levelOff });
    return minus (on, kickWithSample (file, settings, rate, seconds, hits));
}

// CLICK on its sample, long, open and at 0 dB.
Settings clickSample (float tune = 0.0f, float startMs = 0.0f, float decayMs = 2000.0f)
{
    return { { "click_source", sample }, { "click_level", 0.0f }, { "click_tune", tune }, { "click_start", startMs }, { "click_decay", decayMs },
             { "click_hp", 20.0f }, { "click_lp", 20000.0f } };
}

float value (juce::AudioProcessor& plugin, const char* id)
{
    auto* p = findParam (plugin, id);
    return p->convertFrom0to1 (p->getValue());
}

std::unique_ptr<juce::XmlElement> stateOf (juce::AudioProcessor& plugin)
{
    juce::MemoryBlock state;
    plugin.getStateInformation (state);
    return juce::AudioProcessor::getXmlFromBinary (state.getData(), static_cast<int> (state.getSize()));
}

void restore (juce::AudioProcessor& plugin, const juce::XmlElement& xml)
{
    juce::MemoryBlock state;
    juce::AudioProcessor::copyXmlToBinary (xml, state);
    plugin.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
}
} // namespace

// --- LAYER-01 Source ---

TEST_CASE ("layer_01a_tone_sits_on_3_khz_and_follows_tune")
{
    for (const auto rate : { 44100.0, 48000.0, 192000.0 })
        for (const auto& [tune, hz] : { std::pair { 0.0f, 3000.0 }, { 12.0f, 6000.0 }, { -12.0f, 1500.0 } })
        {
            INFO (rate << " Hz, Tune " << tune);
            const auto x = layer (clickTone (-6.0f, tune), rate, 0.3);
            CHECK (measure::frequency (x, rate, 0.01, 0.25) == Catch::Approx (hz).epsilon (0.005));
        }
}

TEST_CASE ("layer_01b_noise_is_broadband")
{
    const auto x = layer ({ { "top_source", noise }, { "top_level", 0.0f }, { "top_decay", 200.0f }, { "top_hp", 20.0f }, { "top_lp", 20000.0f } }, rate48, 0.3);
    const auto white = whiteNoise (rate48, 0.3, 0.2);
    const auto got = measure::bandShares (x, rate48, 0.0, 0.2, 20000.0), want = measure::bandShares (white, rate48, 0.0, 0.2, 20000.0);
    for (size_t band = 1; band < 4; ++band)
    {
        INFO ("band " << band);
        CHECK (got[band] == Catch::Approx (want[band]).margin (3.0));
    }
}

TEST_CASE ("layer_01c_noise_follows_tune")
{
    // How much the share above 4 kHz moves when 12 kHz up is left out: little when there is nothing up there.
    const auto change = [] (float tune) {
        const auto x = layer ({ { "top_level", 0.0f }, { "top_decay", 200.0f }, { "top_hp", 20.0f }, { "top_tune", tune } }, rate48, 0.3);
        return measure::bandShares (x, rate48, 0.0, 0.2)[3] - measure::bandShares (x, rate48, 0.0, 0.2, 12000.0)[3];
    };
    CHECK (std::abs (change (-12.0f)) < 1.0);
    CHECK (change (0.0f) > 10.0);
}

TEST_CASE ("layer_01d_the_same_note_renders_identically")
{
    for (const auto source : { tone, noise })
    {
        INFO ("source " << source);
        const Settings s { { "click_source", static_cast<float> (source) }, { "click_level", 0.0f }, { "click_decay", 50.0f },
                           { "top_source", static_cast<float> (1 - source) }, { "top_level", -3.0f } };
        const auto first = kick (s, rate48, 0.4, { { 0.0 }, { 0.2 } });
        const auto again = kick (s, rate48, 0.4, { { 0.0 }, { 0.2 } }, { 1, 37, 512 });
        CHECK (first == again);
        // Two hits of the same note are the same hit.
        const auto layerOnly = layer (s, rate48, 0.4, { { 0.0 }, { 0.2 } });
        CHECK (measure::largestDifference (layerOnly, 0, layerOnly, static_cast<size_t> (0.2 * rate48), static_cast<size_t> (0.04 * rate48)) < 1.0e-6f);
    }
}

TEST_CASE ("layer_01e_sounds_the_same_at_every_sample_rate")
{
    for (const auto source : { tone, noise })
    {
        const Settings s { { "click_source", static_cast<float> (source) }, { "click_level", 0.0f }, { "click_decay", 30.0f } };
        const auto base = layer (s, rate48, 0.1);
        const auto basePeak = measure::peak (base);
        const auto baseBands = measure::bandShares (base, rate48, 0.0, 0.03, 20000.0);
        for (const auto rate : { 44100.0, 96000.0, 192000.0 })
        {
            INFO ("source " << source << " at " << rate);
            const auto x = layer (s, rate, 0.1);
            CHECK (std::abs (db (measure::peak (x) / basePeak)) < 1.0);
            const auto bands = measure::bandShares (x, rate, 0.0, 0.03, 20000.0);
            for (size_t band = 0; band < 4; ++band)
                CHECK (bands[band] == Catch::Approx (baseBands[band]).margin (3.0));
        }
    }
}

TEST_CASE ("layer_01f_source_is_read_at_the_note_on")
{
    const auto still = layer (clickTone(), rate48, 0.5, { { 0.0 }, { 0.3 } });
    const auto moved = layer (clickTone(), rate48, 0.5, { { 0.0 }, { 0.3 } }, { 256 }, [] (juce::AudioProcessor& p, double t) {
        if (t >= 0.1)
            setParam (p, "click_source", noise);
    });
    const auto split = static_cast<size_t> (0.3 * rate48);
    CHECK_FALSE (measure::largestDifference (still, 0, moved, 0, split) > 0.0f);
    // The next note plays noise, once the old tone's fade (LAYER-03d) has gone.
    const auto noisy = layer ({ { "click_source", noise }, { "click_level", -6.0f }, { "click_decay", 2000.0f } }, rate48, 0.2);
    const auto faded = static_cast<size_t> (0.05 * rate48);
    CHECK (measure::largestDifference (moved, split + faded, noisy, faded, static_cast<size_t> (0.1 * rate48)) < 1.0e-6f);
}

// --- LAYER-02 Knobs ---

TEST_CASE ("layer_02a_level_sets_the_peak_and_off_is_silent")
{
    for (const auto level : { -40.0f, -12.0f, 0.0f, 6.0f })
    {
        INFO ("Level " << level);
        const auto x = layer (clickTone (level), rate48, 0.1);
        CHECK (std::abs (db (measure::peak (x)) - level) < 0.5);
    }

    // At Off nothing of the section is heard, whatever its other knobs say.
    const auto plain = kick ({}, rate48, 0.5);
    const auto off = kick ({ { "click_source", noise }, { "click_level", levelOff }, { "click_tune", 7.0f }, { "click_decay", 900.0f },
                            { "click_hp", 300.0f }, { "click_lp", 5000.0f }, { "top_level", levelOff }, { "top_tune", -5.0f } },
                          rate48, 0.5);
    CHECK (plain == off);
}

TEST_CASE ("layer_02b_velocity_scales_the_layer_as_it_scales_body")
{
    const auto hard = measure::peak (layer (clickTone(), rate48, 0.1, { { 0.0, 36, 127 } }));
    const auto soft = measure::peak (layer (clickTone(), rate48, 0.1, { { 0.0, 36, 64 } }));
    CHECK (soft / hard == Catch::Approx (0.504).epsilon (0.01));
}

TEST_CASE ("layer_02c_decay_falls_as_its_curve_and_ends_exactly")
{
    for (const auto decayMs : { 10.0f, 100.0f, 1000.0f })
    {
        INFO ("Decay " << decayMs);
        const auto decay = decayMs * 0.001;
        const auto x = layer (clickTone (0.0f, 0.0f, decayMs), rate48, decay + 0.1);
        // Halfway the curve (1 - t/D)^2 stands at 0.25; a window of one period of 3 kHz around it.
        CHECK (measure::peakBetween (x, rate48, 0.5 * decay - 0.0002, 0.5 * decay + 0.0002) == Catch::Approx (0.25).epsilon (0.1));
        CHECK (measure::lastSound (x) <= static_cast<int> (std::ceil (decay * rate48)) + 1);
        CHECK (measure::lastSound (x) > static_cast<int> (0.9 * decay * rate48));
    }
}

TEST_CASE ("layer_02d_high_pass_and_low_pass_are_12_db_per_octave")
{
    // A tone's level through the filter against the same tone with both filters open.
    const auto through = [] (double rate, float tune, float hp, float lp) {
        auto s = clickTone (0.0f, tune);
        s.push_back ({ "click_hp", hp });
        s.push_back ({ "click_lp", lp });
        const auto open = layer (clickTone (0.0f, tune), rate, 0.3);
        return db (measure::toneLevel (layer (s, rate, 0.3), rate, 3000.0 * std::pow (2.0, tune / 12.0), 0.05, 0.25)
                   / measure::toneLevel (open, rate, 3000.0 * std::pow (2.0, tune / 12.0), 0.05, 0.25));
    };
    for (const auto rate : { 44100.0, 48000.0, 192000.0 })
    {
        INFO (rate);
        // Low-pass at 750 Hz: 3 kHz is 4 times past it, 6 kHz 8 times.
        CHECK (through (rate, 0.0f, 20.0f, 750.0f) - through (rate, 12.0f, 20.0f, 750.0f) == Catch::Approx (12.0).margin (2.0));
        // High-pass at 12 kHz: 3 kHz is 4 times below it, 1.5 kHz 8 times.
        CHECK (through (rate, 0.0f, 12000.0f, 20000.0f) - through (rate, -12.0f, 12000.0f, 20000.0f) == Catch::Approx (12.0).margin (2.0));
        // The -3 dB point: a tone on the cutoff.
        CHECK (through (rate, 0.0f, 20.0f, 3000.0f) == Catch::Approx (-3.0).margin (1.0));
        CHECK (through (rate, 0.0f, 3000.0f, 20000.0f) == Catch::Approx (-3.0).margin (1.0));
    }

    // At 20 Hz and 20000 Hz the filters leave 100 Hz-10 kHz alone: a tone at the ends of Tune against the sine it should be.
    for (const auto tune : { -24.0f, 0.0f, 20.0f })
    {
        INFO ("Tune " << tune);
        const auto hz = 3000.0 * std::pow (2.0, tune / 12.0);
        std::vector<float> want (static_cast<size_t> (0.3 * rate48));
        for (size_t i = 0; i < want.size(); ++i)
        {
            const auto t = static_cast<double> (i) / rate48;
            want[i] = static_cast<float> (std::sin (2.0 * std::numbers::pi * hz * t) * std::pow (1.0 - t / 2.0, 2.0));
        }
        const auto got = layer (clickTone (0.0f, tune), rate48, 0.3);
        CHECK (std::abs (db (measure::toneLevel (got, rate48, hz, 0.05, 0.25) / measure::toneLevel (want, rate48, hz, 0.05, 0.25))) < 0.1);
    }
}

TEST_CASE ("layer_02e_layers_do_not_follow_the_note")
{
    auto s = clickTone();
    s.push_back ({ "play_key_track", 1.0f });
    s.push_back ({ "top_level", -6.0f });
    const auto low = layer (s, rate48, 0.3, { { 0.0, 24 } });
    const auto high = layer (s, rate48, 0.3, { { 0.0, 60 } });
    CHECK (measure::largestDifference (low, 0, high, 0, low.size()) < 1.0e-6f);
}

TEST_CASE ("layer_02f_the_vs1_09_tone_click_is_a_setting")
{
    const auto x = layer (clickTone (-6.0f, 0.0f, 4.0f), rate48, 0.02);
    std::vector<float> sketch (x.size());
    for (size_t i = 0; i < sketch.size(); ++i)
    {
        const auto ms = 1000.0 * static_cast<double> (i) / rate48;
        sketch[i] = ms < 4.0 ? static_cast<float> (0.5 * std::sin (2.0 * std::numbers::pi * 3000.0 * ms / 1000.0) * std::pow (1.0 - ms / 4.0, 2.0)) : 0.0f;
    }
    CHECK (measure::largestDifference (x, 0, sketch, 0, x.size()) < 0.05f);
    CHECK (std::abs (db (measure::peak (x) / measure::peak (sketch))) < 0.5);
}

TEST_CASE ("layer_02g_the_layers_reach_beefys_first_5_ms")
{
    // BODY at Beefy's fit in VS1-08 (docs/research/reference-kicks.md), the bare voice as that fit was made.
    // The layers' setting is the one VS2-01 found, written into specs/modules/layer.md.
    const Settings beefy {
        { "body_drive", 0.0f }, { "body_start", 0.0f }, { "body_sweep2_amount", 0.0f }, { "body_tail_anchor", 0.0f },
        { "body_tune", 34.0f }, { "body_pitch_amount", 22.42f }, { "body_pitch_decay", 139.7f }, { "body_pitch_shape", 13.6f },
        { "body_amp_hold", 4.1f }, { "body_amp_decay", 234.3f }, { "body_amp_shape", -100.0f },
        { "click_source", noise }, { "click_level", 6.0f }, { "click_decay", 8.0f }, { "click_hp", 1500.0f },
        { "top_source", noise }, { "top_level", 6.0f }, { "top_decay", 10.0f }, { "top_hp", 1000.0f }
    };
    const auto x = kick (beefy, rate48, 0.3);
    const auto first = measure::bandShares (x, rate48, 0.0, 0.005), after = measure::bandShares (x, rate48, 0.005, 0.02);
    INFO ("0-5 ms: " << first[0] << " " << first[1] << " " << first[2] << " " << first[3]);
    CHECK (first[2] + first[3] >= 60.0);
    CHECK (after[2] + after[3] < 5.0);
}

TEST_CASE ("layer_02h_a_knob_jump_during_a_ringing_layer_does_not_click")
{
    // Each knob jumps across its range at 0.3 s. A step may be as large as the tone's own at either end, plus 0.05.
    const std::vector<std::tuple<const char*, float, float>> jumps {
        { "click_level", levelOff, 6.0f }, { "click_level", 6.0f, levelOff }, { "click_tune", -24.0f, 24.0f }, { "click_tune", 24.0f, -24.0f },
        { "click_decay", 2000.0f, 1.0f }, { "click_hp", 20.0f, 20000.0f }, { "click_lp", 20000.0f, 20.0f }, { "click_lp", 20.0f, 20000.0f }
    };
    for (const auto& [id, from, to] : jumps)
    {
        INFO (id << " " << from << " to " << to);
        auto start = clickTone (-6.0f, -24.0f), end = start;
        start.push_back ({ id, from });
        end.push_back ({ id, to });
        const auto own = std::max (measure::largestStepBetween (layer (start, rate48, 0.6), rate48, 0.25, 0.4),
                                   measure::largestStepBetween (layer (end, rate48, 0.6), rate48, 0.25, 0.4));
        const auto jumped = layer (start, rate48, 0.6, { { 0.0 } }, { 256 }, [&] (juce::AudioProcessor& p, double t) {
            if (t >= 0.3)
                setParam (p, id, to);
        });
        CHECK (measure::largestStepBetween (jumped, rate48, 0.25, 0.4) <= own + 0.05f);
    }
}

TEST_CASE ("layer_02h_a_knob_jump_settles_within_50_ms")
{
    // After the jump and 50 ms, the layer is the one that started on the new value, within 1 % of Level's range in level.
    auto start = clickTone (-6.0f, 0.0f), end = clickTone (0.0f, 0.0f);
    const auto settled = layer (end, rate48, 0.6);
    const auto jumped = layer (start, rate48, 0.6, { { 0.0 } }, { 256 }, [] (juce::AudioProcessor& p, double t) {
        if (t >= 0.3)
            setParam (p, "click_level", 0.0f);
    });
    CHECK (std::abs (db (measure::peakBetween (jumped, rate48, 0.36, 0.4) / measure::peakBetween (settled, rate48, 0.36, 0.4))) < 0.66);
}

TEST_CASE ("layer_02h_knob_text_survives_a_round_trip")
{
    PluginProcessor plugin;
    for (auto* id : layerIds)
    {
        auto* p = findParam (plugin, id);
        REQUIRE (p != nullptr);
        CHECK (p->isAutomatable());
        for (int i = 0; i <= 1000; ++i)
        {
            const auto text = p->getText (static_cast<float> (i) / 1000.0f, 32);
            INFO (id << " at " << i << ": " << text);
            CHECK (p->getText (p->getValueForText (text), 32) == text);
        }
    }
    const auto text = [&] (const char* id, float value) { return findParam (plugin, id)->getText (findParam (plugin, id)->convertTo0to1 (value), 32); };
    CHECK (text ("click_level", levelOff) == "Off");
    CHECK (text ("click_level", -6.0f) == "-6.0");
    CHECK (text ("click_tune", -12.0f) == "-12.0");
    CHECK (text ("click_decay", 4.0f) == "4.0");
    CHECK (text ("top_hp", 1000.0f) == "1000");
    CHECK (text ("top_source", tone) == "Tone");
    CHECK (text ("top_source", noise) == "Noise");
    CHECK (text ("top_source", sample) == "Sample");
    CHECK (text ("click_start", 100.0f) == "100.0"); // LAYER-04e
}

// --- LAYER-03 In parallel ---

TEST_CASE ("layer_03a_the_layers_add_and_body_stays_as_it_was")
{
    const Settings both { { "click_level", 0.0f }, { "click_decay", 40.0f }, { "top_level", -3.0f }, { "top_decay", 200.0f }, { "top_hp", 300.0f } };
    const Settings clickOnly { { "click_level", 0.0f }, { "click_decay", 40.0f } };
    const Settings topOnly { { "top_level", -3.0f }, { "top_decay", 200.0f }, { "top_hp", 300.0f } };
    const auto body = kick ({}, rate48, 0.5);
    const auto sum = kick (both, rate48, 0.5), click = minus (kick (clickOnly, rate48, 0.5), body), top = minus (kick (topOnly, rate48, 0.5), body);
    for (size_t i = 0; i < sum.size(); ++i)
        if (std::abs (sum[i] - (body[i] + click[i] + top[i])) > 1.0e-6f)
        {
            FAIL_CHECK ("not additive at sample " << i);
            break;
        }
}

TEST_CASE ("layer_03b_a_layer_starts_on_the_notes_exact_sample")
{
    const Settings s { { "click_source", noise }, { "click_level", 0.0f }, { "click_decay", 20.0f } };
    for (const auto noteAt : { 0, 100, 257, 1023 })
    {
        INFO (noteAt);
        const auto x = layer (s, rate48, 0.1, { { noteAt / rate48 } }, { 1, 37, 512 });
        const auto first = measure::firstSound (x);
        CHECK (first >= noteAt);
        CHECK (first <= noteAt + 1);
    }
}

TEST_CASE ("layer_03c_from_1_ms_up_to_the_full_kick")
{
    const auto shortest = layer (clickTone (0.0f, 0.0f, 1.0f), rate48, 0.05);
    CHECK (measure::lastSound (shortest) <= static_cast<int> (0.001 * rate48) + 1);

    auto longest = clickTone (0.0f, 0.0f, 2000.0f);
    longest.push_back ({ "body_amp_decay", 2000.0f });
    const auto x = layer (longest, rate48, 2.1);
    const auto body = kick ({ { "body_amp_decay", 2000.0f } }, rate48, 2.1);
    CHECK (measure::peakBetween (x, rate48, 1.9, 1.95) > 0.0f);
    CHECK (measure::peakBetween (body, rate48, 1.9, 1.95) > 0.0f);
}

TEST_CASE ("layer_03d_a_retrigger_does_not_click")
{
    // 750 Hz at -6 dB: a step of 0.049 of its own at 48 kHz.
    const auto s = clickTone (-6.0f, -24.0f);
    const auto own = measure::largestStepBetween (layer (s, rate48, 0.6), rate48, 0.3, 0.5);
    const auto period = 1.0 / 750.0;
    for (int point = 0; point < 16; ++point)
    {
        const auto at = 0.5 + period * point / 16.0;
        INFO ("second note at " << at);
        const auto x = layer (s, rate48, at + 0.05, { { 0.0 }, { at } });
        CHECK (measure::largestStepBetween (x, rate48, at - 0.001, at + 0.02) <= own + 0.05f);
    }
}

TEST_CASE ("layer_03e_saved_projects_keep_their_sound")
{
    PluginProcessor saved;
    setParam (saved, "body_amp_decay", 250.0f);
    juce::MemoryBlock state;
    saved.getStateInformation (state);
    auto xml = juce::AudioProcessor::getXmlFromBinary (state.getData(), static_cast<int> (state.getSize()));
    REQUIRE (xml != nullptr);
    CHECK (xml->getIntAttribute ("version") == 3);

    // A state from before this module: no layer knobs in it.
    if (auto* params = xml->getChildByName ("KikkuhParams"))
        for (auto* id : layerIds)
            if (auto* child = params->getChildByAttribute ("id", id))
                params->removeChildElement (child, true);
    juce::MemoryBlock old;
    juce::AudioProcessor::copyXmlToBinary (*xml, old);

    PluginProcessor restored;
    set (restored, { { "click_level", 0.0f }, { "top_level", 0.0f } });
    restored.setStateInformation (old.getData(), static_cast<int> (old.getSize()));
    CHECK (renderHits (restored, rate48, 0.5, { { 0.0 } }) == kick ({ { "body_amp_decay", 250.0f } }, rate48, 0.5));
}

TEST_CASE ("layer_03f_process_block_does_not_allocate_with_the_layers_on")
{
    PluginProcessor plugin;
    set (plugin, { { "click_level", 0.0f }, { "top_level", 0.0f } });
    plugin.prepareToPlay (rate48, 512);
    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;

    int allocations = 0;
    for (int block = 0; block < 200; ++block)
    {
        for (auto* id : layerIds)
            if (auto* p = findParam (plugin, id))
                p->setValueNotifyingHost (static_cast<float> (block % 10) / 9.0f);
        buffer.setSize (2, block % 2 == 0 ? 512 : 37, false, false, true);
        midi.clear();
        if (block % 3 == 0)
            midi.addEvent (juce::MidiMessage::noteOn (1, 36, 1.0f), 0);

        AllocationGuard guard;
        plugin.processBlock (buffer, midi);
        allocations += guard.count();
    }
    CHECK (allocations == 0);
}

// --- LAYER-04 Your own sample ---

TEST_CASE ("layer_04a_a_dropped_file_loads_into_its_section")
{
    for (const auto* name : { "a.wav", "b.WAV", "c.aif", "d.aiff", "e.flac" })
    {
        INFO (name);
        CHECK (PluginProcessor::canLoadSample (juce::File::getCurrentWorkingDirectory().getChildFile (name)));
    }
    for (const auto* name : { "a.mp3", "b.ogg", "c.txt", "d" })
    {
        INFO (name);
        CHECK_FALSE (PluginProcessor::canLoadSample (juce::File::getCurrentWorkingDirectory().getChildFile (name)));
    }

    for (const auto* name : { "drop.wav", "drop.aiff", "drop.flac" })
    {
        INFO (name);
        const auto file = writeAudioFile (name, { sine (1000.0, rate48, 0.2) }, rate48);
        PluginProcessor plugin;
        // At Off a drop sets -6 dB, so it is heard; it chooses Sample; the other section is left alone.
        load (plugin, clickSection, file);
        CHECK (value (plugin, "click_source") == sample);
        CHECK (value (plugin, "click_level") == Catch::Approx (-6.0f).margin (0.01f));
        CHECK (value (plugin, "top_source") == noise);
        CHECK (value (plugin, "top_level") == levelOff);
        CHECK (plugin.getSampleText (clickSection) == name);
        CHECK (plugin.getSampleText (topSection) == "Drop a sample");

        // A Level that is not Off is kept.
        setParam (plugin, "top_level", -12.0f);
        load (plugin, topSection, file);
        CHECK (value (plugin, "top_source") == sample);
        CHECK (value (plugin, "top_level") == Catch::Approx (-12.0f).margin (0.01f));
    }

    // Other files are refused, and of several files the first readable one loads.
    PluginProcessor plugin;
    const auto text = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("kikkuh-tests").getChildFile ("notes.txt");
    text.replaceWithText ("not audio");
    CHECK_FALSE (plugin.loadSample (clickSection, { text.getFullPathName() }));
    CHECK (value (plugin, "click_source") == tone);
    const auto broken = text.getSiblingFile ("broken.wav");
    broken.replaceWithText ("not audio either");
    const auto good = writeAudioFile ("second.wav", { sine (1000.0, rate48, 0.2) }, rate48);
    REQUIRE (plugin.loadSample (clickSection, { text.getFullPathName(), broken.getFullPathName(), good.getFullPathName() }));
    plugin.waitForSampleLoads();
    CHECK (plugin.getSampleText (clickSection) == "second.wav");
    CHECK (value (plugin, "click_source") == sample);
}

TEST_CASE ("layer_04b_a_ringing_layer_keeps_its_sample_and_the_next_note_takes_the_new_one")
{
    const auto first = writeAudioFile ("first.wav", { sine (1000.0, rate48, 1.0) }, rate48);
    const auto second = writeAudioFile ("second.wav", { sine (1500.0, rate48, 1.0) }, rate48);
    const auto hits = std::vector<Hit> { { 0.0 }, { 0.3 } };

    PluginProcessor plugin;
    load (plugin, clickSection, first);
    set (plugin, clickSample());
    const auto swapped = renderHits (plugin, rate48, 0.5, hits, { 256 }, [&, done = false] (double t) mutable {
        if (t >= 0.1 && ! done)
        {
            done = true;
            load (plugin, clickSection, second);
        }
    });
    const auto stayed = kickWithSample (first, clickSample(), rate48, 0.5, hits);
    const auto split = static_cast<size_t> (0.3 * rate48);
    CHECK_FALSE (measure::largestDifference (swapped, 0, stayed, 0, split) > 0.0f);
    // From the second note on it plays the second file.
    const auto layered = minus (swapped, kick ({ { "click_level", levelOff } }, rate48, 0.5, hits));
    CHECK (measure::frequency (layered, rate48, 0.32, 0.45) == Catch::Approx (1500.0).epsilon (0.005));
}

TEST_CASE ("layer_04b_process_block_neither_allocates_nor_frees_while_samples_are_replaced")
{
    const juce::File files[] = { writeAudioFile ("swap-a.wav", { sine (1000.0, rate48, 0.5) }, rate48),
                                 writeAudioFile ("swap-b.wav", { sine (700.0, 44100.0, 3.0), sine (300.0, 44100.0, 3.0) }, 44100.0) };
    PluginProcessor plugin;
    plugin.prepareToPlay (rate48, 512);
    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;

    int allocations = 0, frees = 0;
    for (int block = 0; block < 400; ++block)
    {
        if (block % 20 == 0)
        {
            // 20 drops on each section, outside the guard: the drop is the panel's, not the audio thread's.
            load (plugin, clickSection, files[(block / 20) % 2]);
            load (plugin, topSection, files[(block / 20 + 1) % 2]);
        }
        buffer.setSize (2, block % 2 == 0 ? 512 : 37, false, false, true);
        midi.clear();
        if (block % 3 == 0)
            midi.addEvent (juce::MidiMessage::noteOn (1, 36, 1.0f), 0);

        AllocationGuard guard;
        plugin.processBlock (buffer, midi);
        allocations += guard.count();
        frees += guard.frees();
    }
    CHECK (allocations == 0);
    CHECK (frees == 0);
}

TEST_CASE ("layer_04c_a_sample_plays_as_it_is")
{
    const auto file = writeAudioFile ("sine-1k.wav", { sine (1000.0, rate48, 0.5) }, rate48);
    const auto x = sampleLayer (file, clickSample(), rate48, 0.3);
    CHECK (measure::frequency (x, rate48, 0.01, 0.25) == Catch::Approx (1000.0).epsilon (0.005));
    // The envelope (1 - t / 2 s)^2 stands within 0.05 dB of 1 in the first 5 ms.
    CHECK (std::abs (db (measure::peakBetween (x, rate48, 0.0, 0.005) / 0.5)) < 0.5);

    // Stereo plays the mean of its channels.
    const auto left = sine (1000.0, rate48, 0.3), right = sine (1500.0, rate48, 0.3, 0.3f);
    std::vector<float> mean (left.size());
    for (size_t i = 0; i < mean.size(); ++i)
        mean[i] = 0.5f * (left[i] + right[i]);
    const auto stereo = sampleLayer (writeAudioFile ("stereo.wav", { left, right }, rate48), clickSample(), rate48, 0.3);
    const auto mono = sampleLayer (writeAudioFile ("mean.wav", { mean }, rate48), clickSample(), rate48, 0.3);
    CHECK (measure::largestDifference (stereo, 0, mono, 0, stereo.size()) < 1.0e-4f);
}

TEST_CASE ("layer_04d_tune_is_the_playback_rate")
{
    const auto at48 = writeAudioFile ("rate-48k.wav", { sine (1000.0, rate48, 1.0) }, rate48);
    const auto at44 = writeAudioFile ("rate-44k.wav", { sine (1000.0, 44100.0, 1.0) }, 44100.0);
    for (const auto rate : { 44100.0, 48000.0, 192000.0 })
        for (const auto& [tune, hz] : { std::pair { 0.0f, 1000.0 }, { 12.0f, 2000.0 }, { -12.0f, 500.0 } })
        {
            INFO (rate << " Hz, Tune " << tune);
            CHECK (measure::frequency (sampleLayer (at48, clickSample (tune), rate, 0.3), rate, 0.01, 0.25) == Catch::Approx (hz).epsilon (0.005));
            CHECK (measure::frequency (sampleLayer (at44, clickSample (tune), rate, 0.3), rate, 0.01, 0.25) == Catch::Approx (hz).epsilon (0.005));
        }

    // A file of 0.5 s ends after 0.25 s at +12 and after 1 s at -12.
    const auto half = writeAudioFile ("half-second.wav", { sine (1000.0, rate48, 0.5) }, rate48);
    const auto up = sampleLayer (half, clickSample (12.0f), rate48, 0.4);
    CHECK (measure::peakBetween (up, rate48, 0.2, 0.24) > 0.1f);
    CHECK (measure::peakBetween (up, rate48, 0.27, 0.4) < 0.01f);
    const auto down = sampleLayer (half, clickSample (-12.0f), rate48, 1.2);
    CHECK (measure::peakBetween (down, rate48, 0.9, 0.98) > 0.05f);
    CHECK (measure::peakBetween (down, rate48, 1.02, 1.2) < 0.01f);
}

TEST_CASE ("layer_04e_start_skips_into_the_file")
{
    // A one-sample impulse at 100 ms, so Start 100 ms puts it on the note's own sample.
    std::vector<float> impulse (static_cast<size_t> (0.3 * rate48), 0.0f);
    impulse[static_cast<size_t> (0.1 * rate48)] = 1.0f;
    const auto file = writeAudioFile ("impulse.wav", { impulse }, rate48);
    for (const auto tune : { 0.0f, 12.0f, -12.0f })
        for (const auto noteAt : { 0, 257, 1023 })
        {
            INFO ("Tune " << tune << ", note at " << noteAt);
            auto x = sampleLayer (file, clickSample (tune, 100.0f), rate48, 0.1, { { noteAt / rate48 } });
            for (auto& v : x)
                v = std::abs (v);
            const auto loudest = static_cast<int> (std::max_element (x.begin(), x.end()) - x.begin());
            CHECK (std::abs (loudest - noteAt) <= 1);
        }

    // Past the end of the file: silence.
    CHECK (measure::peak (sampleLayer (file, clickSample (0.0f, 500.0f), rate48, 0.3)) == 0.0f);

    // Read at the note-on: moving it while a layer rings leaves that layer as it was, and the next note follows it.
    const auto tone1k = writeAudioFile ("start-1k.wav", { sine (1000.0, rate48, 1.0) }, rate48);
    const auto hits = std::vector<Hit> { { 0.0 }, { 0.3 } };
    PluginProcessor plugin;
    load (plugin, clickSection, tone1k);
    set (plugin, clickSample());
    const auto moved = renderHits (plugin, rate48, 0.5, hits, { 256 }, [&] (double t) {
        if (t >= 0.1)
            setParam (plugin, "click_start", 250.0f);
    });
    const auto still = kickWithSample (tone1k, clickSample(), rate48, 0.5, hits);
    const auto split = static_cast<size_t> (0.3 * rate48);
    CHECK_FALSE (measure::largestDifference (moved, 0, still, 0, split) > 0.0f);
    const auto later = kickWithSample (tone1k, clickSample (0.0f, 250.0f), rate48, 0.5, hits);
    const auto faded = static_cast<size_t> (0.05 * rate48);
    CHECK (measure::largestDifference (moved, split + faded, later, split + faded, static_cast<size_t> (0.1 * rate48)) < 1.0e-6f);

    // On Tone and Noise Start changes nothing.
    for (const auto source : { tone, noise })
    {
        INFO ("source " << source);
        const Settings s { { "click_source", static_cast<float> (source) }, { "click_level", 0.0f }, { "click_decay", 100.0f } };
        auto skipped = s;
        skipped.push_back ({ "click_start", 1000.0f });
        CHECK (kick (s, rate48, 0.3) == kick (skipped, rate48, 0.3));
    }
}

TEST_CASE ("layer_04f_the_layer_ends_at_decay_or_at_the_files_end_without_a_click")
{
    // A file that ends at full scale.
    const auto loud = writeAudioFile ("ends-loud.wav", { std::vector<float> (static_cast<size_t> (0.1 * rate48), 0.9f) }, rate48);
    const auto x = sampleLayer (loud, clickSample(), rate48, 0.4);
    CHECK (measure::peakBetween (x, rate48, 0.09, 0.1) > 0.5f);
    CHECK (measure::largestStepBetween (x, rate48, 0.095, 0.15) < 0.05f);
    CHECK (measure::peakBetween (x, rate48, 0.25, 0.4) == 0.0f);

    // Decay first: silent from Decay on, within a sample.
    const auto tone1k = writeAudioFile ("decay-1k.wav", { sine (1000.0, rate48, 0.5) }, rate48);
    const auto shortened = sampleLayer (tone1k, clickSample (0.0f, 0.0f, 50.0f), rate48, 0.3);
    CHECK (measure::lastSound (shortened) <= static_cast<int> (std::ceil (0.05 * rate48)) + 1);
}

TEST_CASE ("layer_04g_the_sample_is_saved_with_the_project")
{
    const auto clickFile = writeAudioFile ("saved-click.wav", { sine (1000.0, rate48, 0.4) }, rate48);
    const auto topFile = writeAudioFile ("saved-top.flac", { sine (3000.0, 96000.0, 0.2), sine (2000.0, 96000.0, 0.2) }, 96000.0);
    PluginProcessor saved;
    load (saved, clickSection, clickFile);
    load (saved, topSection, topFile);
    set (saved, { { "click_level", 0.0f }, { "click_tune", 3.0f }, { "click_start", 20.0f }, { "click_decay", 300.0f },
                  { "top_level", -3.0f }, { "top_decay", 100.0f }, { "body_amp_decay", 250.0f } });
    const auto xml = stateOf (saved);
    REQUIRE (xml != nullptr);
    CHECK (xml->getIntAttribute ("version") == 3);
    const auto before = renderHits (saved, rate48, 0.5, { { 0.0 } });

    // The files are gone: the project does not need them.
    clickFile.deleteFile();
    topFile.deleteFile();
    PluginProcessor restored;
    restore (restored, *xml);
    CHECK (restored.getSampleText (clickSection) == "saved-click.wav");
    CHECK (restored.getSampleText (topSection) == "saved-top.flac");
    CHECK (renderHits (restored, rate48, 0.5, { { 0.0 } }) == before);
}

TEST_CASE ("layer_04g_a_sample_is_held_up_to_3_s")
{
    // 4 s at 192 kHz: cut at 3 s, and about 3 MB of state.
    const auto file = writeAudioFile ("long.wav", { sine (1000.0, 192000.0, 4.0) }, 192000.0);
    PluginProcessor plugin;
    load (plugin, clickSection, file);
    juce::MemoryBlock state;
    plugin.getStateInformation (state);
    CHECK (state.getSize() < 3500000);
    CHECK (state.getSize() > 2000000);

    // From Start 1 s at twice the speed the file's 3 s are reached after 1 s.
    const auto x = sampleLayer (file, clickSample (12.0f, 1000.0f), rate48, 1.2);
    CHECK (measure::peakBetween (x, rate48, 0.9, 0.98) > 0.05f);
    CHECK (measure::peakBetween (x, rate48, 1.06, 1.2) < 0.01f);
}

TEST_CASE ("layer_04h_a_missing_sample_never_breaks_the_project")
{
    const auto good = writeAudioFile ("kept.wav", { sine (1000.0, rate48, 0.4) }, rate48);
    const auto broken = good.getSiblingFile ("damaged.wav");
    broken.replaceWithText ("RIFF but not really");

    SECTION ("a file that cannot be read leaves the section as it was")
    {
        PluginProcessor plugin;
        load (plugin, clickSection, good);
        set (plugin, clickSample());
        const auto before = renderHits (plugin, rate48, 0.3, { { 0.0 } });
        REQUIRE (plugin.loadSample (clickSection, { broken.getFullPathName() }));
        plugin.waitForSampleLoads();
        CHECK (plugin.getSampleText (clickSection).startsWith ("Cannot read"));
        CHECK (plugin.getSampleText (clickSection).contains ("damaged.wav"));
        CHECK (renderHits (plugin, rate48, 0.3, { { 0.0 } }) == before);

        // On a section without a sample, its knobs are not touched either.
        PluginProcessor fresh;
        REQUIRE (fresh.loadSample (topSection, { broken.getFullPathName() }));
        fresh.waitForSampleLoads();
        CHECK (value (fresh, "top_source") == noise);
        CHECK (value (fresh, "top_level") == levelOff);
    }
    SECTION ("Sample without a sample is silent")
    {
        PluginProcessor plugin;
        CHECK (plugin.getSampleText (clickSection) == "Drop a sample");
        CHECK (kick ({ { "click_source", sample }, { "click_level", 0.0f }, { "top_source", sample }, { "top_level", 0.0f } }, rate48, 0.3)
               == kick ({}, rate48, 0.3));
    }
    SECTION ("a state whose sample is damaged or absent still loads")
    {
        PluginProcessor saved;
        load (saved, clickSection, good);
        set (saved, { { "click_level", 0.0f }, { "body_amp_decay", 250.0f } });
        const auto xml = stateOf (saved);
        REQUIRE (xml != nullptr);
        auto* held = xml->getChildByName ("Sample");
        REQUIRE (held != nullptr);
        CHECK (held->getStringAttribute ("section") == "click");
        CHECK (held->getStringAttribute ("name") == "kept.wav");

        const auto silent = kick ({ { "body_amp_decay", 250.0f } }, rate48, 0.3);
        held->setAttribute ("data", "not base64 at all!");
        PluginProcessor damaged;
        restore (damaged, *xml);
        CHECK (value (damaged, "body_amp_decay") == Catch::Approx (250.0f));
        CHECK (value (damaged, "click_source") == sample);
        CHECK (damaged.getSampleText (clickSection) == "Missing: kept.wav");
        CHECK (renderHits (damaged, rate48, 0.3, { { 0.0 } }) == silent);

        xml->removeChildElement (held, true);
        PluginProcessor absent;
        restore (absent, *xml);
        CHECK (absent.getSampleText (clickSection) == "Drop a sample");
        CHECK (renderHits (absent, rate48, 0.3, { { 0.0 } }) == silent);
    }
}

TEST_CASE ("layer_04i_a_sample_renders_the_same_every_time_and_at_every_rate")
{
    std::mt19937 random (11u);
    std::uniform_real_distribution<float> uniform (-0.5f, 0.5f);
    std::vector<float> white (static_cast<size_t> (0.3 * rate48));
    for (auto& v : white)
        v = uniform (random);
    const auto file = writeAudioFile ("white.wav", { white }, rate48);
    const auto s = clickSample (0.0f, 10.0f, 30.0f);

    const auto first = kickWithSample (file, s, rate48, 0.4, { { 0.0 }, { 0.2 } });
    CHECK (first == kickWithSample (file, s, rate48, 0.4, { { 0.0 }, { 0.2 } }, { 1, 37, 512 }));

    const auto base = sampleLayer (file, s, rate48, 0.1);
    const auto baseBands = measure::bandShares (base, rate48, 0.0, 0.03, 20000.0);
    for (const auto rate : { 44100.0, 96000.0, 192000.0 })
    {
        INFO (rate);
        const auto x = sampleLayer (file, s, rate, 0.1);
        CHECK (std::abs (db (measure::peak (x) / measure::peak (base))) < 1.0);
        const auto bands = measure::bandShares (x, rate, 0.0, 0.03, 20000.0);
        for (size_t band = 0; band < 4; ++band)
            CHECK (bands[band] == Catch::Approx (baseBands[band]).margin (3.0));
    }
}

TEST_CASE ("layer_04j_saved_projects_keep_their_sound")
{
    // A state from before VS2-02: no Start ids and no sample, Top on Noise.
    PluginProcessor saved;
    set (saved, { { "top_level", 0.0f }, { "body_amp_decay", 250.0f } });
    auto xml = stateOf (saved);
    REQUIRE (xml != nullptr);
    CHECK (xml->getIntAttribute ("version") == 3);
    auto* params = xml->getChildByName ("KikkuhParams");
    REQUIRE (params != nullptr);
    for (const auto* id : { "click_start", "top_start" })
        if (auto* child = params->getChildByAttribute ("id", id))
            params->removeChildElement (child, true);
    // Source is stored as its choice's index.
    REQUIRE (params->getChildByAttribute ("id", "top_source") != nullptr);
    CHECK (params->getChildByAttribute ("id", "top_source")->getDoubleAttribute ("value") == 1.0);

    PluginProcessor restored;
    set (restored, { { "click_start", 300.0f }, { "top_start", 300.0f } });
    restore (restored, *xml);
    CHECK (value (restored, "top_source") == noise);
    CHECK (findParam (restored, "top_source")->getValue() == Catch::Approx (0.5f));
    CHECK (value (restored, "click_start") == 0.0f);
    CHECK (renderHits (restored, rate48, 0.5, { { 0.0 } }) == kick ({ { "top_level", 0.0f }, { "body_amp_decay", 250.0f } }, rate48, 0.5));
}

// --- ENG with LAYER ---

TEST_CASE ("eng_06b_layer_ids_and_defaults_are_as_specified")
{
    PluginProcessor plugin;
    const std::map<juce::String, float> defaults {
        { "click_source", tone }, { "click_level", levelOff }, { "click_tune", 0.0f }, { "click_decay", 4.0f }, { "click_hp", 20.0f }, { "click_lp", 20000.0f },
        { "top_source", noise }, { "top_level", levelOff }, { "top_tune", 0.0f }, { "top_decay", 30.0f }, { "top_hp", 1000.0f }, { "top_lp", 20000.0f },
        { "click_start", 0.0f }, { "top_start", 0.0f } // LAYER-04k
    };
    REQUIRE (defaults.size() == layerIds.size());
    for (const auto& [id, value] : defaults)
    {
        INFO (id);
        auto* p = findParam (plugin, id);
        REQUIRE (p != nullptr);
        CHECK (p->convertFrom0to1 (p->getDefaultValue()) == Catch::Approx (value).margin (0.01f));
    }
    // The ranges of the spec.
    const auto range = [&] (const char* id) { return findParam (plugin, id)->getNormalisableRange(); };
    CHECK (range ("click_level").start == Catch::Approx (-60.0f));
    CHECK (range ("click_level").end == Catch::Approx (6.0f));
    CHECK (range ("top_tune").start == Catch::Approx (-24.0f));
    CHECK (range ("top_tune").end == Catch::Approx (24.0f));
    CHECK (range ("click_decay").start == Catch::Approx (1.0f));
    CHECK (range ("click_decay").end == Catch::Approx (2000.0f));
    CHECK (range ("click_decay").convertFrom0to1 (0.5f) == Catch::Approx (50.0f).epsilon (0.01));
    CHECK (range ("top_hp").convertFrom0to1 (0.5f) == Catch::Approx (632.0f).epsilon (0.01));
    CHECK (range ("click_lp").start == Catch::Approx (20.0f));
    CHECK (range ("click_lp").end == Catch::Approx (20000.0f));
    for (const auto* id : { "click_start", "top_start" })
    {
        INFO (id);
        CHECK (range (id).start == Catch::Approx (0.0f));
        CHECK (range (id).end == Catch::Approx (1000.0f));
        CHECK (range (id).convertFrom0to1 (0.5f) == Catch::Approx (50.0f).epsilon (0.01));
    }
    CHECK (findParam (plugin, "click_source")->getNumSteps() == 3);
}

TEST_CASE ("eng_04_the_kick_with_layers_sounds_the_same_at_every_sample_rate")
{
    const Settings s { { "click_level", -6.0f }, { "click_decay", 10.0f }, { "top_level", -6.0f }, { "top_decay", 60.0f }, { "top_tune", 5.0f } };
    const auto base = kick (s, rate48, 0.5);
    for (const auto rate : { 44100.0, 192000.0 })
    {
        INFO (rate);
        const auto x = kick (s, rate, 0.5);
        CHECK (measure::tailHz (x, rate) == Catch::Approx (measure::tailHz (base, rate48)).epsilon (0.01));
        CHECK (std::abs (db (measure::peak (x) / measure::peak (base))) < 1.0);
    }
}
