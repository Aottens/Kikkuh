#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <juce_audio_formats/juce_audio_formats.h>

// HARN-05: these run the real kikkuh-measure binary on files kikkuh-render wrote, or on files
// written here. Its output is one `key: value` per line.
namespace
{
struct Result
{
    int exitCode;
    juce::String output;
};

Result run (const char* tool, juce::StringArray args)
{
    args.insert (0, tool);
    juce::ChildProcess process;
    REQUIRE (process.start (args));
    const auto output = process.readAllProcessOutput();
    process.waitForProcessToFinish (-1);
    return { static_cast<int> (process.getExitCode()), output };
}

struct TempDir
{
    juce::File dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getNonexistentChildFile ("kikkuh-measure-test", "");
    TempDir() { dir.createDirectory(); }
    ~TempDir() { dir.deleteRecursively(); }
};

juce::File renderKick (const juce::File& file, juce::StringArray params)
{
    params.insert (0, "--out");
    params.insert (1, file.getFullPathName());
    params.add ("--length");
    params.add ("1.5");
    const auto result = run (KIKKUH_RENDER_PATH, params);
    INFO (result.output);
    REQUIRE (result.exitCode == 0);
    return file;
}

juce::String measured (const juce::File& file)
{
    const auto result = run (KIKKUH_MEASURE_PATH, { file.getFullPathName() });
    INFO (result.output);
    REQUIRE (result.exitCode == 0);
    return result.output;
}

juce::String value (const juce::String& output, const juce::String& key)
{
    for (const auto& line : juce::StringArray::fromLines (output))
        if (line.startsWith (key + ": "))
            return line.fromFirstOccurrenceOf (": ", false, false);
    FAIL ("no key " << key << " in\n" << output);
    return {};
}

juce::AudioBuffer<float> read (const juce::File& file)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
    REQUIRE (reader != nullptr);
    juce::AudioBuffer<float> audio (static_cast<int> (reader->numChannels), static_cast<int> (reader->lengthInSamples));
    reader->read (&audio, 0, audio.getNumSamples(), 0, true, true);
    return audio;
}

void write (juce::AudioFormat& format, const juce::AudioBuffer<float>& audio, const juce::File& file, int bits)
{
    std::unique_ptr<juce::OutputStream> stream (file.createOutputStream().release());
    REQUIRE (stream != nullptr);
    auto writer = format.createWriterFor (stream, juce::AudioFormatWriterOptions().withSampleRate (48000.0).withNumChannels (audio.getNumChannels()).withBitsPerSample (bits));
    REQUIRE (writer != nullptr);
    REQUIRE (writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples()));
}
}

TEST_CASE ("harn_05_measures_a_body_render")
{
    TempDir temp;
    const auto output = measured (renderKick (temp.dir.getChildFile ("k.wav"), { "body_tune=A0", "body_pitch_amount=36", "body_pitch_decay=100", "body_pitch_shape=0", "body_amp_decay=800", "body_amp_shape=0" }));

    // BODY-01d: a sine peaks between 0.9 and 1.0.
    CHECK (value (output, "peak_dbfs").getDoubleValue() < 0.1);
    CHECK (value (output, "peak_dbfs").getDoubleValue() > -1.0);

    CHECK (value (output, "tail_hz").getDoubleValue() == Catch::Approx (55.0).epsilon (0.005));
    CHECK (value (output, "tail_note") == "A0");

    // Pitch at 50 ms: 55 * 2^(36 * 0.5^3 / 12) = 71.3 Hz, within 50 cents (3 %).
    CHECK (value (output, "pitch_50ms").getDoubleValue() == Catch::Approx (71.3).epsilon (0.03));

    // A sine has no harmonics to speak of.
    CHECK (value (output, "harmonic_2").getDoubleValue() < -30.0);

    CHECK (value (output, "fit_tune") == "A0");
    CHECK (value (output, "fit_pitch_amount").getDoubleValue() == Catch::Approx (36.0).margin (1.0));
    CHECK (value (output, "fit_pitch_decay_ms").getDoubleValue() == Catch::Approx (100.0).epsilon (0.1));
    CHECK (value (output, "fit_pitch_error_cents").getDoubleValue() < 50.0);
    CHECK (value (output, "fit_amp_decay_ms").getDoubleValue() == Catch::Approx (800.0).epsilon (0.1));
    CHECK (value (output, "fit_amp_error_db").getDoubleValue() < 3.0);

    // A mono voice: L and R are the same.
    CHECK (value (output, "side_db").getDoubleValue() < -100.0);
    CHECK (value (output, "side_0_20ms_db").getDoubleValue() < -100.0);

    for (const auto* key : { "peak_ms", "step_1ms", "peak_0_1ms", "peak_1_2ms", "peak_2_5ms", "peak_5_10ms", "peak_10_20ms",
                             "bands_0_5ms", "bands_5_20ms", "bands_20_50ms", "bands_50_200ms",
                             "below_6db_ms", "below_12db_ms", "below_20db_ms", "below_40db_ms", "below_60db_ms",
                             "pitch_5ms", "pitch_10ms", "pitch_20ms", "pitch_30ms", "pitch_75ms", "pitch_100ms", "pitch_150ms", "pitch_200ms", "pitch_300ms", "pitch_500ms",
                             "harmonic_3", "harmonic_4", "harmonic_5", "harmonic_6", "harmonic_7", "harmonic_8",
                             "fit_pitch_shape", "fit_amp_shape" })
        CHECK (value (output, key).isNotEmpty());
}

TEST_CASE ("harn_05a_silence_in_front_changes_no_number")
{
    TempDir temp;
    const auto dirA = temp.dir.getChildFile ("a"), dirB = temp.dir.getChildFile ("b");
    dirA.createDirectory();
    dirB.createDirectory();
    const auto plain = renderKick (dirA.getChildFile ("k.wav"), {});

    const auto audio = read (plain);
    juce::AudioBuffer<float> delayed (2, audio.getNumSamples() + 1000);
    delayed.clear();
    for (int channel = 0; channel < 2; ++channel)
        delayed.copyFrom (channel, 1000, audio, channel, 0, audio.getNumSamples());
    const auto shifted = dirB.getChildFile ("k.wav");
    juce::WavAudioFormat wav;
    write (wav, delayed, shifted, 32);

    CHECK (measured (plain) == measured (shifted));
}

TEST_CASE ("harn_05b_a_quiet_time_prints_no_pitch")
{
    TempDir temp;
    const auto output = measured (renderKick (temp.dir.getChildFile ("k.wav"), { "body_amp_decay=100" }));
    CHECK (value (output, "pitch_500ms") == "-");
    CHECK (value (output, "pitch_5ms") != "-");
}

TEST_CASE ("harn_05c_a_short_tail_says_so")
{
    TempDir temp;
    const auto output = measured (renderKick (temp.dir.getChildFile ("k.wav"), { "body_pitch_decay=100", "body_amp_decay=100" }));
    CHECK (value (output, "harmonics") == "tail too short");
    CHECK_FALSE (output.contains ("harmonic_2"));
}

TEST_CASE ("harn_05_reads_aiff_and_mono_16_bit")
{
    TempDir temp;
    const auto audio = read (renderKick (temp.dir.getChildFile ("k.wav"), {}));

    juce::AudioBuffer<float> mono (1, audio.getNumSamples());
    mono.copyFrom (0, 0, audio, 0, 0, audio.getNumSamples());
    juce::WavAudioFormat wav;
    const auto monoFile = temp.dir.getChildFile ("mono.wav");
    write (wav, mono, monoFile, 16);
    CHECK (value (measured (monoFile), "side_db").getDoubleValue() < -100.0);

    juce::AiffAudioFormat aiff;
    const auto aiffFile = temp.dir.getChildFile ("k.aif");
    write (aiff, audio, aiffFile, 24);
    CHECK (value (measured (aiffFile), "tail_hz").getDoubleValue() == Catch::Approx (55.0).epsilon (0.005));
}

TEST_CASE ("harn_05_measures_each_file_named")
{
    TempDir temp;
    const auto a = renderKick (temp.dir.getChildFile ("first.wav"), {});
    const auto b = renderKick (temp.dir.getChildFile ("second.wav"), {});
    const auto result = run (KIKKUH_MEASURE_PATH, { a.getFullPathName(), b.getFullPathName() });
    CHECK (result.exitCode == 0);
    CHECK (result.output.contains ("first.wav"));
    CHECK (result.output.contains ("second.wav"));
}

TEST_CASE ("harn_05g_bad_input_exits_non_zero_naming_it")
{
    TempDir temp;

    CHECK (run (KIKKUH_MEASURE_PATH, {}).exitCode != 0);

    const auto missing = run (KIKKUH_MEASURE_PATH, { temp.dir.getChildFile ("nothing.wav").getFullPathName() });
    CHECK (missing.exitCode != 0);
    CHECK (missing.output.contains ("nothing.wav"));

    const auto text = temp.dir.getChildFile ("notes.wav");
    text.replaceWithText ("not audio");
    const auto unreadable = run (KIKKUH_MEASURE_PATH, { text.getFullPathName() });
    CHECK (unreadable.exitCode != 0);
    CHECK (unreadable.output.contains ("notes.wav"));

    juce::AudioBuffer<float> silence (2, 4800);
    silence.clear();
    juce::WavAudioFormat wav;
    const auto silent = temp.dir.getChildFile ("silent.wav");
    write (wav, silence, silent, 32);
    const auto quiet = run (KIKKUH_MEASURE_PATH, { silent.getFullPathName() });
    CHECK (quiet.exitCode != 0);
    CHECK (quiet.output.contains ("silent.wav"));
}
