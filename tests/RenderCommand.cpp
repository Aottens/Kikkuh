#include "Measure.h"
#include "Render.h"
#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <juce_audio_formats/juce_audio_formats.h>
#include <map>

// HARN-04: these run the real kikkuh-render binary and read back the WAV it writes.
namespace
{
struct Result
{
    int exitCode;
    juce::String output;
};

Result run (juce::StringArray args)
{
    args.insert (0, KIKKUH_RENDER_PATH);
    juce::ChildProcess process;
    REQUIRE (process.start (args));
    const auto output = process.readAllProcessOutput();
    process.waitForProcessToFinish (-1); // the output can end before the exit code is there
    return { static_cast<int> (process.getExitCode()), output };
}

struct TempDir
{
    juce::File dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getNonexistentChildFile ("kikkuh-render-test", "");
    TempDir() { dir.createDirectory(); }
    ~TempDir() { dir.deleteRecursively(); }
    juce::File wav (const char* name = "out.wav") const { return dir.getChildFile (name); }
};

struct Wav
{
    juce::AudioBuffer<float> audio;
    double sampleRate = 0.0;
    unsigned int bits = 0;
    bool isFloat = false;
    std::vector<float> left() const { return { audio.getReadPointer (0), audio.getReadPointer (0) + audio.getNumSamples() }; }
};

Wav readWav (const juce::File& file)
{
    juce::WavAudioFormat format;
    std::unique_ptr<juce::AudioFormatReader> reader (format.createReaderFor (file.createInputStream().release(), true));
    REQUIRE (reader != nullptr);
    Wav wav;
    wav.sampleRate = reader->sampleRate;
    wav.bits = reader->bitsPerSample;
    wav.isFloat = reader->usesFloatingPointData;
    wav.audio.setSize (static_cast<int> (reader->numChannels), static_cast<int> (reader->lengthInSamples));
    reader->read (&wav.audio, 0, wav.audio.getNumSamples(), 0, true, true);
    return wav;
}

// Renders with the command's settings and returns the file.
Wav render (const TempDir& temp, juce::StringArray args)
{
    const auto file = temp.wav();
    args.insert (0, "--out");
    args.insert (1, file.getFullPathName());
    const auto result = run (args);
    INFO (result.output);
    REQUIRE (result.exitCode == 0);
    return readWav (file);
}

juce::AudioBuffer<float> renderDirect (const std::map<juce::String, float>& plainValues, double rate, double seconds, float velocity = 1.0f)
{
    PluginProcessor plugin;
    for (auto* p : plugin.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (const auto it = plainValues.find (ranged->getParameterID()); it != plainValues.end())
                ranged->setValueNotifyingHost (ranged->convertTo0to1 (it->second));
    return renderNoteStereo (plugin, rate, seconds, 0, 36, velocity);
}

void checkIdentical (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
{
    REQUIRE (a.getNumChannels() == b.getNumChannels());
    REQUIRE (a.getNumSamples() == b.getNumSamples());
    for (int ch = 0; ch < a.getNumChannels(); ++ch)
    {
        int differing = 0;
        for (int i = 0; i < a.getNumSamples(); ++i)
            differing += juce::exactlyEqual (a.getSample (ch, i), b.getSample (ch, i)) ? 0 : 1;
        CHECK (differing == 0);
    }
}
}

TEST_CASE ("harn_04a_values_are_in_the_knobs_own_units")
{
    TempDir temp;
    // A0 is 55 Hz (BODY-02a); Amp Decay 10 ms ends the kick at 10 ms (BODY-04c).
    const auto tail = render (temp, { "--length", "1", "body_tune=A0", "body_pitch_amount=0", "body_amp_decay=2000", "body_sweep2_amount=0" }).left();
    CHECK (measure::frequency (tail, 48000.0, 0.1, 0.5) == Catch::Approx (55.0).margin (55.0 * 0.005));

    const auto shortKick = render (temp, { "--length", "1", "body_amp_decay=10", "body_drive=0" }).left();
    CHECK (measure::lastSound (shortKick) <= 480);
    CHECK (measure::lastSound (shortKick) > 400);
}

TEST_CASE ("harn_04a_tune_takes_a_note_name_or_a_midi_note")
{
    TempDir temp;
    const auto expected = 440.0 * std::pow (2.0, (30.0 - 69.0) / 12.0); // F#0, MIDI 30 with C3 = 60
    for (const auto* tune : { "body_tune=F#0", "body_tune=Gb0", "body_tune=30" })
    {
        INFO (tune);
        const auto tail = render (temp, { "--length", "1", tune, "body_pitch_amount=0", "body_amp_decay=2000", "body_sweep2_amount=0" }).left();
        CHECK (measure::frequency (tail, 48000.0, 0.1, 0.5) == Catch::Approx (expected).margin (expected * 0.005));
    }
}

TEST_CASE ("harn_04a_values_hold_from_the_first_sample_without_a_glide")
{
    TempDir temp;
    // With a glide from the default Amount of 60, the first 100 ms would run far above 55 Hz.
    const auto audio = render (temp, { "--length", "1", "body_tune=A0", "body_pitch_amount=0", "body_amp_decay=2000", "body_sweep2_amount=0" }).left();
    CHECK (measure::frequency (audio, 48000.0, 0.0, 0.1) == Catch::Approx (55.0).margin (55.0 * 0.005));
}

TEST_CASE ("harn_04b_the_wav_is_stereo_32_bit_float")
{
    TempDir temp;
    const auto wav = render (temp, { "--length", "0.1" });
    CHECK (wav.audio.getNumChannels() == 2);
    CHECK (wav.bits == 32);
    CHECK (wav.isFloat);
}

TEST_CASE ("harn_04b_a_peak_above_full_scale_is_kept")
{
    TempDir temp;
    // A square above about 6 kHz peaks over 1.0 (DECISIONS 2026-11-03).
    const auto wav = render (temp, { "--length", "0.2", "body_wave=3", "body_tune=C3", "body_pitch_amount=72", "body_pitch_decay=1000" }).left();
    CHECK (measure::peak (wav) > 1.0f);
}

TEST_CASE ("harn_04b_an_existing_file_is_replaced_completely")
{
    TempDir temp;
    const auto file = temp.wav();
    REQUIRE (run ({ "--out", file.getFullPathName(), "--length", "2" }).exitCode == 0);
    const auto fresh = temp.wav ("fresh.wav");
    REQUIRE (run ({ "--out", fresh.getFullPathName(), "--length", "0.1" }).exitCode == 0);
    REQUIRE (run ({ "--out", file.getFullPathName(), "--length", "0.1" }).exitCode == 0);
    CHECK (file.getSize() == fresh.getSize());
    CHECK (readWav (file).audio.getNumSamples() == 4800);
}

TEST_CASE ("harn_04c_the_file_is_sample_identical_to_a_direct_render")
{
    TempDir temp;
    SECTION ("defaults")
    {
        checkIdentical (render (temp, { "--length", "1" }).audio, renderDirect ({}, 48000.0, 1.0));
    }
    SECTION ("every knob away from its default")
    {
        const auto wav = render (temp, { "--length", "1", "--rate", "96000", "body_wave=1.5", "body_tune=40", "body_pitch_amount=60",
                                         "body_pitch_decay=40", "body_pitch_shape=30", "body_amp_attack=2.5", "body_amp_hold=20",
                                         "body_amp_decay=300", "body_amp_shape=-20" });
        const auto direct = renderDirect ({ { "body_wave", 1.5f }, { "body_tune", 40.0f }, { "body_pitch_amount", 60.0f },
                                            { "body_pitch_decay", 40.0f }, { "body_pitch_shape", 30.0f }, { "body_amp_attack", 2.5f },
                                            { "body_amp_hold", 20.0f }, { "body_amp_decay", 300.0f }, { "body_amp_shape", -20.0f } },
                                          96000.0, 1.0);
        checkIdentical (wav.audio, direct);
    }
}

TEST_CASE ("harn_04c_the_same_command_twice_gives_identical_samples")
{
    TempDir temp;
    const juce::StringArray settings { "--length", "0.5", "body_wave=2", "body_pitch_amount=48" };
    const auto first = render (temp, settings);
    const auto second = render (temp, settings);
    checkIdentical (first.audio, second.audio);
}

TEST_CASE ("harn_04d_length_and_rate")
{
    TempDir temp;
    SECTION ("defaults: 3 seconds at 48 kHz")
    {
        const auto wav = render (temp, {});
        CHECK (wav.sampleRate == Catch::Approx (48000.0));
        CHECK (wav.audio.getNumSamples() == 144000);
    }
    SECTION ("whole samples, rounded down")
    {
        const auto wav = render (temp, { "--length", "0.0101" });
        CHECK (wav.audio.getNumSamples() == 484);
    }
    SECTION ("every rate from 44.1 to 192 kHz is written in the header")
    {
        for (const auto* rate : { "44100", "96000", "192000" })
        {
            const auto wav = render (temp, { "--length", "0.5", "--rate", rate });
            CHECK (wav.sampleRate == Catch::Approx (juce::String (rate).getDoubleValue()));
            CHECK (wav.audio.getNumSamples() == juce::String (rate).getIntValue() / 2);
        }
    }
    SECTION ("the kick lands on the same pitch at another rate (ENG-04)")
    {
        const auto tail = render (temp, { "--length", "1", "--rate", "192000", "body_pitch_amount=0", "body_amp_decay=2000", "body_sweep2_amount=0" }).left();
        CHECK (measure::frequency (tail, 192000.0, 0.1, 0.5) == Catch::Approx (55.0).margin (55.0 * 0.005));
    }
}

TEST_CASE ("harn_04e_the_note_starts_on_sample_zero")
{
    TempDir temp;
    // Sample 0 is sin (0), as in build_06_note_on_sounds_on_its_exact_sample.
    const auto audio = render (temp, { "--length", "0.1", "body_start=0", "body_drive=0" }).left();
    CHECK (measure::firstSound (audio) == 1);
}

TEST_CASE ("harn_04e_velocity_scales_the_level_linearly")
{
    TempDir temp;
    const auto full = measure::peak (render (temp, { "--length", "0.2", "body_drive=0" }).left());
    const auto at127 = measure::peak (render (temp, { "--length", "0.2", "--velocity", "127", "body_drive=0" }).left());
    const auto at64 = measure::peak (render (temp, { "--length", "0.2", "--velocity", "64", "body_drive=0" }).left());
    CHECK (at127 == Catch::Approx (full));
    CHECK (at64 == Catch::Approx (full * 64.0f / 127.0f).margin (full * 0.01f));
}

TEST_CASE ("harn_04e_every_note_plays_the_same_kick")
{
    TempDir temp;
    checkIdentical (render (temp, { "--length", "0.2", "--note", "60" }).audio, render (temp, { "--length", "0.2" }).audio);
}

TEST_CASE ("harn_04f_bad_input_exits_non_zero_names_it_and_leaves_no_file")
{
    struct Case
    {
        juce::StringArray args;
        const char* named;
    };
    const std::vector<Case> cases {
        { { "body_nonsense=1" }, "body_nonsense" },
        { { "body_wave" }, "body_wave" },
        { { "body_pitch_amount=abc" }, "abc" },
        { { "body_pitch_amount=1.5abc" }, "1.5abc" },
        { { "body_pitch_amount=nan" }, "nan" },
        { { "body_pitch_amount=" }, "body_pitch_amount" },
        { { "body_pitch_amount=73" }, "73" },
        { { "body_pitch_amount=-1" }, "-1" },
        { { "body_pitch_decay=0.5" }, "0.5" },
        { { "body_tune=H1" }, "H1" },
        { { "body_tune=C" }, "body_tune=C" },
        { { "body_tune=C4" }, "C4" },
        { { "body_tune=23" }, "23" },
        { { "--length", "0" }, "--length" },
        { { "--length", "-1" }, "--length" },
        { { "--length", "abc" }, "abc" },
        { { "--rate", "44099" }, "44099" },
        { { "--rate", "192001" }, "192001" },
        { { "--rate", "abc" }, "abc" },
        { { "--note", "-1" }, "-1" },
        { { "--note", "128" }, "128" },
        { { "--velocity", "0" }, "--velocity" },
        { { "--velocity", "128" }, "128" },
        { { "--bogus" }, "--bogus" },
        { { "--length" }, "--length" },
    };

    TempDir temp;
    const auto file = temp.wav();
    for (const auto& c : cases)
    {
        INFO (c.args.joinIntoString (" "));
        auto args = c.args;
        args.insert (0, "--out");
        args.insert (1, file.getFullPathName());
        const auto result = run (args);
        CHECK (result.exitCode != 0);
        CHECK (result.output.contains (c.named));
        CHECK_FALSE (file.exists());
    }
}

TEST_CASE ("harn_04f_an_unknown_id_lists_the_valid_ids")
{
    TempDir temp;
    const auto result = run ({ "--out", temp.wav().getFullPathName(), "body_nonsense=1" });
    CHECK (result.exitCode != 0);
    for (const auto* id : { "body_wave", "body_tune", "body_pitch_amount", "body_pitch_decay", "body_pitch_shape",
                            "body_amp_attack", "body_amp_hold", "body_amp_decay", "body_amp_shape",
                            "body_drive", "body_filter_cutoff", "body_filter_res", "body_filter_env",
                            "body_sweep2_amount", "body_sweep2_decay", "body_start", "play_velocity", "play_key_track" })
        CHECK (result.output.contains (id));
}

TEST_CASE ("harn_04f_a_missing_out_is_an_error")
{
    const auto result = run ({ "--length", "1" });
    CHECK (result.exitCode != 0);
    CHECK (result.output.contains ("--out"));
}

TEST_CASE ("harn_04f_an_unwritable_path_is_an_error")
{
    TempDir temp;
    const auto notADirectory = temp.wav ("file");
    notADirectory.create();
    const auto path = notADirectory.getChildFile ("out.wav").getFullPathName();
    const auto result = run ({ "--out", path });
    CHECK (result.exitCode != 0);
    CHECK (result.output.contains (path));
}

TEST_CASE ("harn_04f_a_directory_as_output_is_an_error_and_stays")
{
    TempDir temp;
    const auto result = run ({ "--out", temp.dir.getFullPathName(), "--length", "0.1" });
    CHECK (result.exitCode != 0);
    CHECK (result.output.contains (temp.dir.getFullPathName()));
    CHECK (temp.dir.isDirectory());
}
