#include "Measure.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <juce_audio_formats/juce_audio_formats.h>
#include <map>

// HARN-07: these run the real kikkuh-round binary and read back what it writes.
namespace
{
struct Result
{
    int exitCode;
    juce::String output;
};

Result run (juce::StringArray args)
{
    args.insert (0, KIKKUH_ROUND_PATH);
    juce::ChildProcess process;
    REQUIRE (process.start (args));
    const auto output = process.readAllProcessOutput();
    process.waitForProcessToFinish (-1);
    return { static_cast<int> (process.getExitCode()), output };
}

struct TempDir
{
    juce::File dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getNonexistentChildFile ("kikkuh-round-test", "");
    TempDir() { dir.createDirectory(); }
    ~TempDir() { dir.deleteRecursively(); }

    juce::File round (const juce::String& text) const
    {
        const auto file = dir.getChildFile ("round.txt");
        REQUIRE (file.replaceWithText (text));
        return file;
    }
    juce::File out (const char* name = "out") const { return dir.getChildFile (name); }
};

const juce::String threeOptions = "# a comment\n"
                                  "base body_tune=A0 body_amp_decay=400   # a trailing comment\n"
                                  "option plainname\n"
                                  "option softname sat_kind=soft sat_drive=12\n"
                                  "option longname body_amp_decay=1200\n";

Result make (const juce::File& round, const juce::File& out, int seed = 7)
{
    return run ({ round.getFullPathName(), "--out", out.getFullPathName(), "--seed", juce::String (seed) });
}

struct Wav
{
    juce::AudioBuffer<float> audio;
    double sampleRate = 0.0;
    unsigned int bits = 0;
    bool isFloat = false;
    std::vector<float> channel (int ch) const { return { audio.getReadPointer (ch), audio.getReadPointer (ch) + audio.getNumSamples() }; }
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

// key.txt: one line per option, letter first, then the option's name.
std::map<juce::String, juce::String> readKey (const juce::File& out)
{
    std::map<juce::String, juce::String> key;
    juce::StringArray lines;
    lines.addLines (out.getChildFile ("key.txt").loadFileAsString());
    for (const auto& line : lines)
    {
        juce::StringArray tokens;
        tokens.addTokens (line, " ", "");
        if (tokens.size() >= 2 && tokens[0].length() == 1)
            key[tokens[1]] = tokens[0];
    }
    return key;
}

double lufs (const Wav& wav)
{
    return measure::loudness (wav.channel (0), wav.channel (1), wav.sampleRate);
}

// Writes a mono 48 kHz sine as a stand-in for a reference.
void writeSine (const juce::File& file, double seconds, double rate = 44100.0)
{
    juce::AudioBuffer<float> audio (1, static_cast<int> (seconds * rate));
    for (int i = 0; i < audio.getNumSamples(); ++i)
        audio.setSample (0, i, 0.5f * static_cast<float> (std::sin (2.0 * 3.14159265358979 * 55.0 * i / rate)));
    std::unique_ptr<juce::OutputStream> stream (file.createOutputStream().release());
    juce::WavAudioFormat format;
    auto writer = format.createWriterFor (stream, juce::AudioFormatWriterOptions().withSampleRate (rate).withNumChannels (1).withBitsPerSample (24));
    REQUIRE (writer != nullptr);
    REQUIRE (writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples()));
}
}

TEST_CASE ("harn_07a_every_option_is_matched_to_minus_20_lufs")
{
    TempDir temp;
    const auto out = temp.out();
    const auto result = make (temp.round (threeOptions), out);
    INFO (result.output);
    REQUIRE (result.exitCode == 0);
    for (const auto* name : { "A.wav", "B.wav", "C.wav" })
        CHECK (lufs (readWav (out.getChildFile (name))) == Catch::Approx (-20.0).margin (0.1));
}

TEST_CASE ("harn_07a_an_option_that_would_peak_too_high_is_an_error_naming_it")
{
    TempDir temp;
    // 10 ms of kick in a 400 ms window reads quiet, so it would need more gain than its peak can take.
    const auto result = make (temp.round ("option fine\noption shortname body_amp_decay=10 body_pitch_amount=0\n"), temp.out());
    CHECK (result.exitCode != 0);
    CHECK (result.output.contains ("shortname"));
    CHECK_FALSE (temp.out().getChildFile ("A.wav").exists());
}

TEST_CASE ("harn_07b_files_are_unnamed_and_equally_long")
{
    TempDir temp;
    const auto out = temp.out();
    const auto result = make (temp.round (threeOptions), out);
    REQUIRE (result.exitCode == 0);

    for (const auto* name : { "plainname", "softname", "longname" })
    {
        CHECK_FALSE (result.output.contains (name));
        for (const auto* letter : { "A.wav", "B.wav", "C.wav" })
        {
            juce::MemoryBlock bytes;
            REQUIRE (out.getChildFile (letter).loadFileAsData (bytes));
            CHECK (bytes.toString().indexOf (name) < 0);
        }
    }
    CHECK_FALSE (out.getChildFile ("D.wav").exists());

    const auto length = readWav (out.getChildFile ("A.wav")).audio.getNumSamples();
    CHECK (readWav (out.getChildFile ("B.wav")).audio.getNumSamples() == length);
    CHECK (readWav (out.getChildFile ("C.wav")).audio.getNumSamples() == length);
    // The longest kick decides: 1.2 s, and it is matched down to -90 dBFS or so before the end.
    CHECK (length > static_cast<int> (0.6 * 48000.0));
    CHECK (length <= static_cast<int> (1.3 * 48000.0));

    const auto key = readKey (out);
    REQUIRE (key.size() == 3);
    CHECK (key.count ("plainname") == 1);
    CHECK (key.count ("softname") == 1);
    CHECK (key.count ("longname") == 1);
}

TEST_CASE ("harn_07b_the_key_says_the_gain")
{
    TempDir temp;
    const auto out = temp.out();
    REQUIRE (make (temp.round (threeOptions), out).exitCode == 0);
    const auto text = out.getChildFile ("key.txt").loadFileAsString();
    CHECK (text.contains ("lufs"));
    CHECK (text.contains ("gain_db"));
}

TEST_CASE ("harn_07c_the_same_file_and_seed_give_identical_bytes")
{
    TempDir temp;
    const auto round = temp.round (threeOptions);
    REQUIRE (make (round, temp.out ("one")).exitCode == 0);
    REQUIRE (make (round, temp.out ("two")).exitCode == 0);
    for (const auto* name : { "A.wav", "B.wav", "C.wav", "key.txt" })
    {
        INFO (name);
        CHECK (temp.out ("one").getChildFile (name).hasIdenticalContentTo (temp.out ("two").getChildFile (name)));
    }
}

TEST_CASE ("harn_07c_an_options_audio_does_not_depend_on_the_seed")
{
    TempDir temp;
    const auto round = temp.round (threeOptions);
    REQUIRE (make (round, temp.out ("one"), 1).exitCode == 0);
    const auto firstKey = readKey (temp.out ("one"));

    // Some seed among these gives other letters, and the audio travels with its option.
    bool anotherOrder = false;
    for (int seed = 2; seed < 10; ++seed)
    {
        const auto out = temp.out ("other").getSiblingFile ("other" + juce::String (seed));
        REQUIRE (make (round, out, seed).exitCode == 0);
        const auto key = readKey (out);
        anotherOrder = anotherOrder || key != firstKey;
        for (const auto& [name, letter] : key)
        {
            INFO (name);
            CHECK (out.getChildFile (letter + ".wav").hasIdenticalContentTo (temp.out ("one").getChildFile (firstKey.at (name) + ".wav")));
        }
    }
    CHECK (anotherOrder);
}

TEST_CASE ("harn_07d_output_format_and_references")
{
    TempDir temp;
    const auto reference = temp.dir.getChildFile ("My Reference.wav");
    writeSine (reference, 1.0);
    const auto out = temp.out();
    const auto result = make (temp.round (threeOptions + "reference Kick09 " + reference.getFullPathName() + "\n"), out);
    INFO (result.output);
    REQUIRE (result.exitCode == 0);

    const auto a = readWav (out.getChildFile ("A.wav"));
    CHECK (a.sampleRate == Catch::Approx (48000.0));
    CHECK (a.audio.getNumChannels() == 2);
    CHECK (a.bits == 32);
    CHECK (a.isFloat);

    const auto ref = readWav (out.getChildFile ("ref-Kick09.wav"));
    CHECK (ref.sampleRate == Catch::Approx (44100.0));
    CHECK (ref.audio.getNumChannels() == 2);
    CHECK (lufs (ref) == Catch::Approx (-20.0).margin (0.1));
    CHECK_FALSE (out.getChildFile ("D.wav").exists());
}

TEST_CASE ("harn_07d_a_note_name_in_a_round_works")
{
    TempDir temp;
    const auto out = temp.out();
    REQUIRE (make (temp.round ("base body_pitch_amount=0 body_amp_decay=2000\noption a body_tune=F#0\noption b body_tune=F#0 tune_cents=0\n"), out).exitCode == 0);
    const auto x = readWav (out.getChildFile ("A.wav")).channel (0);
    const auto expected = 440.0 * std::pow (2.0, (30.0 - 69.0) / 12.0);
    CHECK (measure::frequency (x, 48000.0, 0.1, 0.5) == Catch::Approx (expected).margin (expected * 0.005));
}

TEST_CASE ("harn_07e_bad_input_exits_non_zero_names_it_and_writes_no_audio")
{
    struct Case
    {
        juce::String round;
        const char* named;
    };
    const std::vector<Case> cases {
        { "option a\noption b body_nonsense=1\n", "body_nonsense" },
        { "option a\noption b tune_cents=500\n", "tune_cents" },
        { "base sat_kind=fuzz\noption a\noption b\n", "sat_kind" },
        { "option a\nwhatisthis b\n", "whatisthis" },
        { "option onlyone\n", "2" },
        { "option a\noption b\noption c\noption d\noption e\noption f\noption seventh\n", "6" },
        { "option a\noption b\noption a sat_kind=soft\n", "a" },
        { "option a\noption b\nreference gone /nonexistent/gone.wav\n", "gone" },
    };

    TempDir temp;
    for (const auto& c : cases)
    {
        INFO (c.round);
        const auto out = temp.out();
        const auto result = make (temp.round (c.round), out);
        CHECK (result.exitCode != 0);
        CHECK (result.output.contains (c.named));
        CHECK_FALSE (out.getChildFile ("A.wav").exists());
    }
}

TEST_CASE ("harn_07e_missing_arguments_and_a_full_out_are_errors")
{
    TempDir temp;
    const auto round = temp.round (threeOptions);

    SECTION ("no seed")
    {
        const auto result = run ({ round.getFullPathName(), "--out", temp.out().getFullPathName() });
        CHECK (result.exitCode != 0);
        CHECK (result.output.contains ("--seed"));
    }
    SECTION ("no out")
    {
        const auto result = run ({ round.getFullPathName(), "--seed", "1" });
        CHECK (result.exitCode != 0);
        CHECK (result.output.contains ("--out"));
    }
    SECTION ("no file")
    {
        const auto result = run ({ "/nonexistent/round.txt", "--out", temp.out().getFullPathName(), "--seed", "1" });
        CHECK (result.exitCode != 0);
        CHECK (result.output.contains ("round.txt"));
    }
    SECTION ("an out that is not empty is never overwritten")
    {
        const auto out = temp.out();
        out.createDirectory();
        const auto kept = out.getChildFile ("A.wav");
        kept.replaceWithText ("kept");
        const auto result = make (round, out);
        CHECK (result.exitCode != 0);
        CHECK (result.output.contains (out.getFullPathName()));
        CHECK (kept.loadFileAsString() == "kept");
    }
}
