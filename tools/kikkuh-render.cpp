#include "Render.h"
#include <PluginProcessor.h>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <juce_audio_formats/juce_audio_formats.h>

// HARN-04: kikkuh-render --out kick.wav [--length 3] [--rate 48000] [--note 36] [--velocity 127] [id=value ...]
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

// The whole text must be a finite number.
double parseNumber (const juce::String& text, const juce::String& what)
{
    const auto utf8 = text.toStdString();
    char* end = nullptr;
    const auto value = std::strtod (utf8.c_str(), &end);
    if (utf8.empty() || std::isspace (static_cast<unsigned char> (utf8[0])) || *end != '\0' || ! std::isfinite (value))
        fail ("'" + text + "' is not a number (" + what + ")");
    return value;
}

int parseWhole (const juce::String& text, const juce::String& option, int low, int high)
{
    const auto value = parseNumber (text, option);
    if (value != std::floor (value) || value < low || value > high)
        fail ("'" + text + "' is not a whole number from " + juce::String (low) + " to " + juce::String (high) + " (" + option + ")");
    return static_cast<int> (value);
}

// Letter, optional # or b, optional minus, digits: "F#0", "Bb-1".
bool isNoteName (const juce::String& text)
{
    if (juce::String ("CDEFGAB").indexOfChar (juce::CharacterFunctions::toUpperCase (text[0])) < 0)
        return false;
    auto rest = text.substring (1);
    if (rest.startsWithChar ('#') || rest.startsWithChar ('b'))
        rest = rest.substring (1);
    if (rest.startsWithChar ('-'))
        rest = rest.substring (1);
    return rest.isNotEmpty() && rest.containsOnly ("0123456789");
}

juce::RangedAudioParameter* findParam (PluginProcessor& plugin, const juce::String& id)
{
    for (auto* p : plugin.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (ranged->getParameterID() == id)
                return ranged;

    juce::StringArray ids;
    for (auto* p : plugin.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
            ids.add (ranged->getParameterID());
    fail ("unknown parameter '" + id + "'. Valid ids: " + ids.joinIntoString (", "));
}

void setParam (PluginProcessor& plugin, const juce::String& token)
{
    const auto eq = token.indexOfChar ('=');
    if (eq < 0)
        fail ("'" + token + "' is not id=value");
    const auto id = token.substring (0, eq);
    const auto text = token.substring (eq + 1);
    auto* param = findParam (plugin, id);

    double value = 0.0;
    if (id == "body_tune" && isNoteName (text))
        value = PluginProcessor::noteFromText (text);
    else
        value = parseNumber (text, token);

    const auto range = param->getNormalisableRange();
    if (value < range.start || value > range.end)
        fail ("'" + text + "' is outside " + id + "'s range " + juce::String (range.start) + " to " + juce::String (range.end) + " (" + token + ")");
    param->setValueNotifyingHost (param->convertTo0to1 (static_cast<float> (value)));
}

void writeWav (const juce::AudioBuffer<float>& audio, double rate, const juce::File& target)
{
    const auto path = target.getFullPathName();
    const auto temp = target.getSiblingFile (target.getFileName() + ".tmp").getNonexistentSibling();
    {
        std::unique_ptr<juce::OutputStream> stream (temp.createOutputStream().release());
        if (stream == nullptr)
            fail ("cannot write '" + path + "'");

        juce::WavAudioFormat format;
        auto writer = format.createWriterFor (stream, juce::AudioFormatWriterOptions().withSampleRate (rate).withNumChannels (2).withBitsPerSample (32));
        if (writer == nullptr || ! writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples()))
        {
            writer.reset();
            temp.deleteFile();
            fail ("cannot write '" + path + "'");
        }
    }
    if (target.isDirectory() || ! temp.moveFileTo (target))
    {
        temp.deleteFile();
        fail ("cannot write '" + path + "'");
    }
}

int run (int argc, char** argv)
{
    juce::File out;
    double length = 3.0;
    int rate = 48000, note = 36, velocity = 127;
    std::vector<juce::String> params;

    for (int i = 1; i < argc; ++i)
    {
        const auto token = juce::String::fromUTF8 (argv[i]);
        if (! token.startsWith ("--"))
        {
            params.push_back (token);
            continue;
        }
        if (i + 1 >= argc)
            fail ("option '" + token + "' needs a value");
        const auto value = juce::String::fromUTF8 (argv[++i]);

        if (token == "--out")
            out = juce::File::getCurrentWorkingDirectory().getChildFile (value);
        else if (token == "--length")
        {
            length = parseNumber (value, token);
            if (length <= 0.0)
                fail ("'" + value + "' must be above 0 (" + token + ")");
        }
        else if (token == "--rate")
            rate = parseWhole (value, token, 44100, 192000);
        else if (token == "--note")
            note = parseWhole (value, token, 0, 127);
        else if (token == "--velocity")
            velocity = parseWhole (value, token, 1, 127);
        else
            fail ("unknown option '" + token + "'");
    }
    if (out == juce::File())
        fail ("--out is required");
    if (length * rate < 1.0 || length * rate > std::numeric_limits<int>::max())
        fail ("--length " + juce::String (length) + " does not fit a WAV at " + juce::String (rate) + " Hz");

    PluginProcessor plugin;
    for (const auto& token : params)
        setParam (plugin, token);

    writeWav (renderNoteStereo (plugin, rate, length, 0, note, static_cast<float> (velocity) / 127.0f), rate, out);
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
        std::cerr << "kikkuh-render: " << f.message << std::endl;
        return 1;
    }
}
