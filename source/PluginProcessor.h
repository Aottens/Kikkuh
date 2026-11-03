#pragma once

#include <dsp/BodyVoice.h>
#include <juce_audio_processors/juce_audio_processors.h>

class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();

    static int noteFromText (const juce::String& text);

    void prepareToPlay (double sampleRate, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    // Hosts cope badly with zero programs.
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // BODY-02d: the note the tail lands on, for the panel (VS1-06). Message thread.
    double getTailNote() const { return dsp::bodyTailNote (readParams()); }
    juce::String getTailReadout() const { return dsp::bodyNoteReadout (getTailNote()); }

private:
    static constexpr int stateVersion = 1;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    dsp::BodyParams readParams() const;
    void migrate (juce::XmlElement& xml, int fromVersion);

    juce::AudioProcessorValueTreeState apvts;
    std::array<std::atomic<float>*, 9> raw {};
    dsp::BodyVoice voice;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
