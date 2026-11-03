#include "PluginProcessor.h"

PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

void PluginProcessor::prepareToPlay (double sampleRate, int)
{
    voice.prepare (sampleRate);
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

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
    copyXmlToBinary (xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // The first parameters bring the migration hook and the restore (ENG-06).
    juce::ignoreUnused (getXmlFromBinary (data, sizeInBytes));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
