#include "PluginProcessor.h"

namespace
{
constexpr const char* paramIds[] = { "body_wave", "body_tune", "body_pitch_amount", "body_pitch_decay", "body_pitch_shape",
                                     "body_amp_attack", "body_amp_hold", "body_amp_decay", "body_amp_shape",
                                     "body_drive", "body_filter_cutoff", "body_filter_res", "body_filter_env" };
}

// C3 = 60. Takes "F#0", "C1", "Bb2" or a plain MIDI note number.
int PluginProcessor::noteFromText (const juce::String& text)
{
    const auto s = text.trim();
    const auto letter = juce::String ("CDEFGAB").indexOfChar (juce::CharacterFunctions::toUpperCase (s[0]));
    if (letter < 0)
        return s.getIntValue();

    static constexpr int semitones[] = { 0, 2, 4, 5, 7, 9, 11 };
    auto note = semitones[letter];
    auto rest = s.substring (1);
    if (rest.startsWithChar ('#') || rest.startsWithChar ('b'))
    {
        note += rest[0] == '#' ? 1 : -1;
        rest = rest.substring (1);
    }
    return (rest.getIntValue() + 2) * 12 + note;
}

PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "KikkuhParams", createLayout())
{
    for (size_t i = 0; i < raw.size(); ++i)
        raw[i] = apvts.getRawParameterValue (paramIds[i]);

    // The wavetables take a moment to build: do it here, never on the audio thread.
    dsp::MorphOsc::warmUp();
}

juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createLayout()
{
    using Float = juce::AudioParameterFloat;
    using Attributes = juce::AudioParameterFloatAttributes;
    const auto ranged = [] (float low, float high, float centre = 0.0f) {
        juce::NormalisableRange<float> range (low, high);
        if (centre > 0.0f)
            range.setSkewForCentre (centre);
        return range;
    };

    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const auto add = [&] (const char* id, const char* name, juce::NormalisableRange<float> range, float def, const char* unit, int decimals) {
        // Fixed decimals, so the text survives value -> text -> value -> text (clap-validator).
        layout.add (std::make_unique<Float> (juce::ParameterID { id, 1 }, name, range, def,
                                             Attributes().withLabel (unit).withStringFromValueFunction ([decimals] (float v, int) { return decimals == 0 ? juce::String (juce::roundToInt (v)) : juce::String (v, decimals); })));
    };

    add ("body_wave", "Wave", ranged (0.0f, 3.0f), 0.0f, "", 2);
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "body_tune", 1 }, "Tune", 24, 60, 33,
        juce::AudioParameterIntAttributes()
            .withStringFromValueFunction ([] (int v, int) { return juce::String (dsp::bodyNoteName (v)); })
            .withValueFromStringFunction (&PluginProcessor::noteFromText)));
    add ("body_pitch_amount", "Pitch Amount", ranged (0.0f, 72.0f), 36.0f, "st", 1);
    add ("body_pitch_decay", "Pitch Decay", ranged (1.0f, 1000.0f, 60.0f), 100.0f, "ms", 1);
    add ("body_pitch_shape", "Pitch Shape", ranged (-100.0f, 100.0f), 0.0f, "%", 0);
    add ("body_amp_attack", "Attack", ranged (0.0f, 50.0f, 5.0f), 0.0f, "ms", 1);
    add ("body_amp_hold", "Hold", ranged (0.0f, 500.0f, 50.0f), 0.0f, "ms", 1);
    add ("body_amp_decay", "Amp Decay", ranged (10.0f, 2000.0f, 200.0f), 400.0f, "ms", 1);
    add ("body_amp_shape", "Amp Shape", ranged (-100.0f, 100.0f), 0.0f, "%", 0);
    add ("body_drive", "Drive", ranged (0.0f, 24.0f), 0.0f, "dB", 1);
    add ("body_filter_cutoff", "Cutoff", ranged (20.0f, 20000.0f, 632.0f), 20000.0f, "Hz", 0);
    add ("body_filter_res", "Resonance", ranged (0.0f, 100.0f), 0.0f, "%", 0);
    add ("body_filter_env", "Env Amount", ranged (0.0f, 8.0f), 0.0f, "oct", 1);
    return layout;
}

dsp::BodyParams PluginProcessor::readParams() const
{
    return { raw[0]->load(), raw[1]->load(), raw[2]->load(), raw[3]->load(), raw[4]->load(),
             raw[5]->load(), raw[6]->load(), raw[7]->load(), raw[8]->load(),
             raw[9]->load(), raw[10]->load(), raw[11]->load(), raw[12]->load() };
}

void PluginProcessor::prepareToPlay (double sampleRate, int)
{
    voice.setParams (readParams());
    voice.prepare (sampleRate);
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    voice.setParams (readParams());

    auto render = [&] (int start, int end) {
        for (int i = start; i < end; ++i)
        {
            const auto sample = voice.next();
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.getWritePointer (ch)[i] = sample;
        }
    };

    int position = 0;
    for (const auto metadata : midi)
    {
        const auto offset = juce::jlimit (position, buffer.getNumSamples(), metadata.samplePosition);
        render (position, offset);
        position = offset;

        const auto message = metadata.getMessage();
        if (message.isNoteOn())
            voice.trigger (message.getFloatVelocity());
    }
    render (position, buffer.getNumSamples());
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return layouts.getMainInputChannelSet().isDisabled()
           && (out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono());
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::XmlElement xml ("KikkuhState");
    xml.setAttribute ("version", stateVersion);
    xml.addChildElement (apvts.copyState().createXml().release());
    copyXmlToBinary (xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName ("KikkuhState"))
        return;

    migrate (*xml, xml->getIntAttribute ("version", 1));

    // A state without parameters (VS0-01) restores every parameter to its default.
    const auto* saved = xml->getChildByName (apvts.state.getType());
    apvts.replaceState (saved != nullptr ? juce::ValueTree::fromXml (*saved) : juce::ValueTree (apvts.state.getType()));
}

void PluginProcessor::migrate (juce::XmlElement&, int)
{
    // Later versions rewrite older states here, one step per version.
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
