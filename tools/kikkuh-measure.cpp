#include "Measure.h"
#include <cmath>
#include <iostream>
#include <juce_audio_formats/juce_audio_formats.h>

// HARN-05: kikkuh-measure kick.wav [more.wav ...]
namespace
{
struct Failure
{
    juce::String message;
};

[[noreturn]] void fail (const juce::String& message)
{
    throw Failure { message };
}

struct Audio
{
    std::vector<float> left, right, mid;
    double rate = 48000.0;
};

// Reads a WAV or AIFF; mono is L = R. Everything is trimmed to time zero (HARN-05a).
Audio load (const juce::String& name)
{
    const auto file = juce::File::getCurrentWorkingDirectory().getChildFile (name);
    if (! file.existsAsFile())
        fail ("'" + name + "' does not exist");

    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
    if (reader == nullptr || reader->numChannels < 1 || reader->lengthInSamples < 1)
        fail ("cannot read '" + name + "' as a WAV or AIFF");

    const auto length = static_cast<int> (reader->lengthInSamples);
    juce::AudioBuffer<float> buffer (2, length);
    buffer.clear();
    if (! reader->read (&buffer, 0, length, 0, true, true))
        fail ("cannot read '" + name + "'");
    if (reader->numChannels == 1)
        buffer.copyFrom (1, 0, buffer, 0, 0, length);

    Audio audio;
    audio.rate = reader->sampleRate;
    audio.left.assign (buffer.getReadPointer (0), buffer.getReadPointer (0) + length);
    audio.right.assign (buffer.getReadPointer (1), buffer.getReadPointer (1) + length);
    audio.mid.resize (audio.left.size());
    for (size_t i = 0; i < audio.mid.size(); ++i)
        audio.mid[i] = 0.5f * (audio.left[i] + audio.right[i]);

    const auto start = measure::onset (audio.mid);
    if (start < 0)
        fail ("'" + name + "' is silent");
    for (auto* v : { &audio.left, &audio.right, &audio.mid })
        v->erase (v->begin(), v->begin() + start);
    return audio;
}

juce::String number (double value, int decimals = 2)
{
    const auto text = juce::String (value, decimals);
    return text.containsOnly ("-0.") ? text.trimCharactersAtStart ("-") : text; // no "-0.0"
}

// The level in dB relative to the peak, or "-" for silence.
juce::String relativeDb (double level, double top)
{
    return level > 0.0 ? number (20.0 * std::log10 (level / top), 1) : juce::String ("-");
}

// MIDI 60 = C3, so A0 = 33 and 55 Hz.
juce::String noteName (double hz)
{
    static const char* const names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const auto midi = static_cast<int> (std::lround (69.0 + 12.0 * std::log2 (hz / 440.0)));
    return juce::String (names[((midi % 12) + 12) % 12]) + juce::String (static_cast<int> (std::floor (midi / 12.0)) - 2);
}

double noteHz (const juce::String& name)
{
    static const char* const names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const auto octave = name.getTrailingIntValue();
    for (int i = 0; i < 12; ++i)
        if (name.dropLastCharacters (juce::String (octave).length()) == names[i])
            return 440.0 * std::pow (2.0, ((octave + 2) * 12 + i - 69) / 12.0);
    return 0.0;
}

void say (const juce::String& key, const juce::String& value)
{
    std::cout << key.toStdString() << ": " << value.toStdString() << "\n";
}

void measureFile (const juce::String& name)
{
    const auto audio = load (name);
    const auto& x = audio.mid;
    const auto rate = audio.rate;
    const auto top = static_cast<double> (measure::peak (x));

    say ("file", juce::File::createFileWithoutCheckingPath (name).getFileName());
    say ("peak_dbfs", number (20.0 * std::log10 (top)));

    const auto tail = measure::tailHz (x, rate);
    say ("tail_hz", tail > 0.0 ? number (tail) : "-");
    say ("tail_note", tail > 0.0 ? noteName (tail) : "-");

    for (auto ms : { 5, 10, 20, 30, 50, 75, 100, 150, 200, 300, 500 })
    {
        const auto hz = measure::pitchAt (x, rate, ms / 1000.0);
        say ("pitch_" + juce::String (ms) + "ms", hz > 0.0 ? number (hz) : "-");
    }

    const auto stretch = measure::tailStretch (x, rate);
    const auto harmonics = measure::harmonicsDb (x, rate, tail, stretch.from, stretch.to);
    if (harmonics.empty())
        say ("harmonics", "tail too short");
    else
        for (size_t h = 0; h < harmonics.size(); ++h)
            say ("harmonic_" + juce::String (h + 2), number (harmonics[h], 1));

    say ("peak_ms", number (measure::peakSeconds (x, rate) * 1000.0));
    say ("step_1ms", relativeDb (static_cast<double> (measure::largestStepBetween (x, rate, 0.0, 0.001)), top));
    for (auto [a, b] : { std::pair { 0, 1 }, std::pair { 1, 2 }, std::pair { 2, 5 }, std::pair { 5, 10 }, std::pair { 10, 20 } })
        say ("peak_" + juce::String (a) + "_" + juce::String (b) + "ms",
             relativeDb (static_cast<double> (measure::peakBetween (x, rate, a / 1000.0, b / 1000.0)), top));
    for (auto [a, b] : { std::pair { 0, 5 }, std::pair { 5, 20 }, std::pair { 20, 50 }, std::pair { 50, 200 } })
    {
        const auto shares = measure::bandShares (x, rate, a / 1000.0, b / 1000.0);
        say ("bands_" + juce::String (a) + "_" + juce::String (b) + "ms",
             number (shares[0], 1) + " " + number (shares[1], 1) + " " + number (shares[2], 1) + " " + number (shares[3], 1));
    }
    for (auto db : { 6, 12, 20, 40, 60 })
        say ("below_" + juce::String (db) + "db_ms", number (measure::secondsBelowDb (x, rate, -db) * 1000.0));

    const auto pitch = measure::fitPitch (x, rate);
    if (pitch.tuneHz > 0.0)
    {
        const auto note = noteName (pitch.tuneHz);
        say ("fit_tune", note);
        say ("fit_tune_error_cents", number (1200.0 * std::log2 (tail / noteHz (note)), 1));
        say ("fit_pitch_amount", number (pitch.amount));
        say ("fit_pitch_decay_ms", number (pitch.decaySeconds * 1000.0, 1));
        say ("fit_pitch_shape", number (pitch.shape, 1));
        say ("fit_pitch_error_cents", number (pitch.errorCents, 1));
        say ("fit_pitch_outliers", juce::String (pitch.outliers));
    }
    else
        for (const auto* key : { "fit_tune", "fit_tune_error_cents", "fit_pitch_amount", "fit_pitch_decay_ms", "fit_pitch_shape", "fit_pitch_error_cents", "fit_pitch_outliers" })
            say (key, "-");

    const auto amp = measure::fitAmp (x, rate);
    say ("fit_amp_decay_ms", number (amp.decaySeconds * 1000.0, 1));
    say ("fit_amp_shape", number (amp.shape, 1));
    say ("fit_amp_error_db", number (amp.errorDb));

    say ("side_db", number (measure::sideToMidDb (audio.left, audio.right, rate, 0.0, 1.0e9), 1));
    say ("side_0_20ms_db", number (measure::sideToMidDb (audio.left, audio.right, rate, 0.0, 0.02), 1));
}

int run (int argc, char** argv)
{
    if (argc < 2)
        fail ("no file named. Usage: kikkuh-measure kick.wav [more.wav ...]");
    for (int i = 1; i < argc; ++i)
    {
        if (i > 1)
            std::cout << "\n";
        measureFile (juce::String::fromUTF8 (argv[i]));
    }
    return 0;
}
}

int main (int argc, char** argv)
{
    try
    {
        return run (argc, argv);
    }
    catch (const Failure& f)
    {
        std::cerr << "kikkuh-measure: " << f.message << std::endl;
        return 1;
    }
}
