#include "Measure.h"
#include "Sketch.h"
#include <algorithm>
#include <climits>
#include <cmath>
#include <iostream>
#include <juce_audio_formats/juce_audio_formats.h>
#include <random>

// HARN-07: kikkuh-round <round.txt> --out DIR --seed N
// Renders the options of a round file with the sketch voice (HARN-08), matches every option and reference
// to -20 LUFS, and writes them as blind, shuffled files A.wav, B.wav ... plus key.txt.
namespace
{
constexpr double targetLufs = -20.0, peakLimitDb = -1.0, trimDb = -90.0, rate = 48000.0;

struct Failure
{
    juce::String message;
};

[[noreturn]] void fail (const juce::String& message)
{
    throw Failure { message };
}

struct Option
{
    std::string name;
    sketch::Params params;
};

struct Audio
{
    std::string name;
    std::vector<float> left, right;
    double rate = 48000.0;
    double lufs = 0.0, gainDb = 0.0, peakDb = 0.0;
};

struct Round
{
    std::vector<Option> options;
    std::vector<std::pair<std::string, juce::File>> references;
};

double parseSeed (const juce::String& text)
{
    const auto utf8 = text.toStdString();
    char* end = nullptr;
    const auto value = std::strtod (utf8.c_str(), &end);
    if (utf8.empty() || std::isspace (static_cast<unsigned char> (utf8[0])) || *end != '\0' || ! std::isfinite (value) || value != std::floor (value) || value < 0 || value > INT_MAX)
        fail ("'" + text + "' is not a whole number from 0 to " + juce::String (INT_MAX) + " (--seed)");
    return value;
}

// base / option / reference lines; everything is checked here, before anything is rendered.
Round parseRound (const juce::File& file)
{
    Round round;
    std::vector<std::pair<std::string, std::string>> base;
    std::vector<std::vector<std::pair<std::string, std::string>>> ownKeys;
    juce::StringArray lines;
    lines.addLines (file.loadFileAsString());

    for (int n = 0; n < lines.size(); ++n)
    {
        const auto where = "'" + file.getFileName() + "' line " + juce::String (n + 1) + ": ";
        auto line = lines[n].trim();
        const auto isReference = line.startsWith ("reference ") || line.startsWith ("reference\t"); // a path may hold ' #'
        if (! isReference && line.contains (" #"))
            line = line.upToFirstOccurrenceOf (" #", false, false).trim();
        if (line.isEmpty() || line.startsWithChar ('#'))
            continue;

        juce::StringArray tokens;
        tokens.addTokens (line, " \t", "");
        const auto word = tokens[0];

        if (isReference)
        {
            const auto rest = line.substring (9).trim();
            const auto name = rest.upToFirstOccurrenceOf (" ", false, false).upToFirstOccurrenceOf ("\t", false, false);
            auto path = rest.substring (name.length()).trim();
            if (name.isEmpty() || path.isEmpty())
                fail (where + "a reference needs a name and a path");
            if (name.containsAnyOf ("/\\"))
                fail (where + "reference name '" + name + "' may not hold a slash");
            for (const auto& r : round.references)
                if (r.first == name.toStdString())
                    fail (where + "duplicate reference name '" + name + "'");
            const auto target = path.startsWith ("~/") ? juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile (path.substring (2))
                                                       : juce::File::getCurrentWorkingDirectory().getChildFile (path);
            round.references.emplace_back (name.toStdString(), target);
        }
        else if (word == "base" || word == "option")
        {
            if (word == "option" && tokens.size() < 2)
                fail (where + "an option needs a name");
            std::vector<std::pair<std::string, std::string>> pairs;
            sketch::Params scratch;
            for (int i = word == "option" ? 2 : 1; i < tokens.size(); ++i)
            {
                const auto eq = tokens[i].indexOfChar ('=');
                if (eq < 0)
                    fail (where + "'" + tokens[i] + "' is not key=value");
                const auto key = tokens[i].substring (0, eq).toStdString(), value = tokens[i].substring (eq + 1).toStdString();
                if (! sketch::set (scratch, key, value))
                    fail (where + "bad key or value '" + tokens[i] + "' (" + key + ")");
                pairs.emplace_back (key, value);
            }
            if (word == "base")
                base.insert (base.end(), pairs.begin(), pairs.end());
            else
            {
                for (const auto& o : round.options)
                    if (o.name == tokens[1].toStdString())
                        fail (where + "duplicate option name '" + tokens[1] + "'");
                ownKeys.push_back (pairs);
                round.options.push_back ({ tokens[1].toStdString(), {} });
            }
        }
        else
            fail (where + "unknown line word '" + word + "'");
    }

    // base first for every option, then its own keys, so an option overrides.
    for (size_t i = 0; i < round.options.size(); ++i)
    {
        for (const auto& [k, v] : base)
            sketch::set (round.options[i].params, k, v);
        for (const auto& [k, v] : ownKeys[i])
            sketch::set (round.options[i].params, k, v);
    }
    if (round.options.size() < 2)
        fail ("'" + file.getFileName() + "' needs at least 2 options, it has " + juce::String (static_cast<int> (round.options.size())));
    if (round.options.size() > 6)
        fail ("'" + file.getFileName() + "' may have at most 6 options, it has " + juce::String (static_cast<int> (round.options.size())));
    return round;
}

Audio loadReference (const juce::String& name, const juce::File& file)
{
    if (! file.existsAsFile())
        fail ("reference '" + name + "': '" + file.getFullPathName() + "' does not exist");
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
    if (reader == nullptr || reader->numChannels < 1 || reader->lengthInSamples < 1)
        fail ("reference '" + name + "': cannot read '" + file.getFullPathName() + "' as audio");

    const auto length = static_cast<int> (reader->lengthInSamples);
    juce::AudioBuffer<float> buffer (2, length);
    buffer.clear();
    if (! reader->read (&buffer, 0, length, 0, true, true))
        fail ("reference '" + name + "': cannot read '" + file.getFullPathName() + "'");
    if (reader->numChannels == 1)
        buffer.copyFrom (1, 0, buffer, 0, 0, length);

    Audio audio;
    audio.name = name.toStdString();
    audio.rate = reader->sampleRate;
    audio.left.assign (buffer.getReadPointer (0), buffer.getReadPointer (0) + length);
    audio.right.assign (buffer.getReadPointer (1), buffer.getReadPointer (1) + length);
    return audio;
}

// Measures the loudness, applies the gain to -20 LUFS and checks the peak.
void match (Audio& audio, const juce::String& what)
{
    audio.lufs = measure::loudness (audio.left, audio.right, audio.rate);
    if (! std::isfinite (audio.lufs))
        fail (what + " is silent");
    audio.gainDb = targetLufs - audio.lufs;
    const auto gain = static_cast<float> (std::pow (10.0, audio.gainDb / 20.0));
    for (auto* channel : { &audio.left, &audio.right })
        for (auto& v : *channel)
            v *= gain;
    audio.peakDb = 20.0 * std::log10 (std::max (static_cast<double> (std::max (measure::peak (audio.left), measure::peak (audio.right))), 1.0e-12));
    if (audio.peakDb > peakLimitDb)
        fail (what + " would peak at " + juce::String (audio.peakDb, 2) + " dBFS after the match to " + juce::String (targetLufs, 0) + " LUFS, above "
              + juce::String (peakLimitDb, 0) + " dBFS; it is not limited");
}

void writeWav (const Audio& audio, const juce::File& target)
{
    juce::AudioBuffer<float> buffer (2, static_cast<int> (audio.left.size()));
    buffer.copyFrom (0, 0, audio.left.data(), buffer.getNumSamples());
    buffer.copyFrom (1, 0, audio.right.data(), buffer.getNumSamples());
    std::unique_ptr<juce::OutputStream> stream (target.createOutputStream().release());
    if (stream == nullptr)
        fail ("cannot write '" + target.getFullPathName() + "'");
    juce::WavAudioFormat format;
    auto writer = format.createWriterFor (stream, juce::AudioFormatWriterOptions().withSampleRate (audio.rate).withNumChannels (2).withBitsPerSample (32));
    if (writer == nullptr || ! writer->writeFromAudioSampleBuffer (buffer, 0, buffer.getNumSamples()))
        fail ("cannot write '" + target.getFullPathName() + "'");
}

int run (int argc, char** argv)
{
    juce::File roundFile, out;
    bool haveSeed = false;
    double seed = 0.0;

    for (int i = 1; i < argc; ++i)
    {
        const auto token = juce::String::fromUTF8 (argv[i]);
        if (! token.startsWith ("--"))
        {
            if (roundFile != juce::File())
                fail ("more than one round file ('" + token + "')");
            roundFile = juce::File::getCurrentWorkingDirectory().getChildFile (token);
            continue;
        }
        if (i + 1 >= argc)
            fail ("option '" + token + "' needs a value");
        const auto value = juce::String::fromUTF8 (argv[++i]);
        if (token == "--out")
            out = juce::File::getCurrentWorkingDirectory().getChildFile (value);
        else if (token == "--seed")
        {
            seed = parseSeed (value);
            haveSeed = true;
        }
        else
            fail ("unknown option '" + token + "'");
    }
    if (roundFile == juce::File())
        fail ("the round file is required");
    if (out == juce::File())
        fail ("--out is required");
    if (! haveSeed)
        fail ("--seed is required");
    if (! roundFile.existsAsFile())
        fail ("round file '" + roundFile.getFullPathName() + "' does not exist");
    if (out.exists() && (! out.isDirectory() || out.getNumberOfChildFiles (juce::File::findFilesAndDirectories) > 0))
        fail ("--out '" + out.getFullPathName() + "' exists and is not empty; a round is never overwritten");

    const auto round = parseRound (roundFile);
    std::vector<Audio> references;
    for (const auto& [name, file] : round.references)
        references.push_back (loadReference (juce::String (name), file));

    // Render and match everything before anything is written.
    std::vector<Audio> options;
    for (const auto& option : round.options)
    {
        Audio audio;
        audio.name = option.name;
        audio.left = sketch::render48k (option.params, 3.0);
        audio.right = audio.left;
        audio.rate = rate;
        match (audio, "option '" + option.name + "'");
        options.push_back (std::move (audio));
    }
    for (auto& reference : references)
        match (reference, "reference '" + reference.name + "'");

    // Every file is as long as the longest kick until it falls below -90 dBFS, plus 50 ms.
    const auto floor = static_cast<float> (std::pow (10.0, trimDb / 20.0));
    size_t length = 0;
    for (const auto& audio : options)
        for (size_t i = audio.left.size(); i > length; --i)
            if (std::abs (audio.left[i - 1]) > floor)
            {
                length = i;
                break;
            }
    length += static_cast<size_t> (std::llround (0.05 * rate));
    for (auto& audio : options)
    {
        audio.left.resize (length, 0.0f);
        audio.right.resize (length, 0.0f);
    }

    // Fisher-Yates by hand: std::shuffle differs between standard libraries.
    std::mt19937 engine (static_cast<std::uint32_t> (seed));
    std::vector<size_t> order (options.size());
    for (size_t i = 0; i < order.size(); ++i)
        order[i] = i;
    for (size_t i = order.size() - 1; i > 0; --i)
        std::swap (order[i], order[engine() % (i + 1)]);

    if (! out.createDirectory())
        fail ("cannot create '" + out.getFullPathName() + "'");
    juce::String key = "seed " + juce::String (static_cast<int> (seed)) + "\n", letters;
    for (size_t i = 0; i < order.size(); ++i)
    {
        const auto& audio = options[order[i]];
        const auto letter = juce::String::charToString (static_cast<juce::juce_wchar> ('A' + i));
        writeWav (audio, out.getChildFile (letter + ".wav"));
        key << letter << " " << audio.name << " lufs=" << juce::String (audio.lufs, 2) << " gain_db=" << juce::String (audio.gainDb, 2)
            << " peak_dbfs=" << juce::String (audio.peakDb, 2) << "\n";
        letters << letter << " ";
    }
    for (const auto& reference : references)
    {
        writeWav (reference, out.getChildFile ("ref-" + juce::String (reference.name) + ".wav"));
        key << "ref " << reference.name << " lufs=" << juce::String (reference.lufs, 2) << " gain_db=" << juce::String (reference.gainDb, 2) << "\n";
    }
    if (! out.getChildFile ("key.txt").replaceWithText (key))
        fail ("cannot write '" + out.getChildFile ("key.txt").getFullPathName() + "'");

    std::cout << "wrote " << letters.trim() << " to " << out.getFullPathName() << std::endl;
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
        std::cerr << "kikkuh-round: " << f.message << std::endl;
        return 1;
    }
}
