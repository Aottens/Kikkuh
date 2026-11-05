#pragma once

#include <dsp/AuditionLoop.h>
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

    // FAST-01: the audition. Message thread; the audio thread reads atomics.
    void pressPad() { padPressed = true; }
    void setLoop (bool on) { loopOn = on; }
    bool isLoopOn() const { return loopOn; }
    // FAST-01n: go up on every kick and every loop hit, so the panel can light.
    int getKickCount() const { return kicks; }
    int getLoopHitCount() const { return loopHits; }

    // FAST-05: what the panel needs. Message thread; the panel width is not part of the saved state.
    juce::AudioProcessorValueTreeState& getValueTreeState() { return apvts; }
    int getPanelWidth() const { return panelWidth; }
    void setPanelWidth (int width) { panelWidth = width; }

private:
    static constexpr int stateVersion = 3;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    dsp::BodyParams readParams() const;
    void migrate (juce::XmlElement& xml, int fromVersion);
    void beginLoop (int numSamples, bool on);

    juce::AudioProcessorValueTreeState apvts;
    std::array<std::atomic<float>*, 19> raw {};
    dsp::BodyVoice voice;
    dsp::AuditionLoop loop;
    double sampleRate = 44100.0;
    std::atomic<bool> padPressed { false }, loopOn { false };
    std::atomic<int> kicks { 0 }, loopHits { 0 };
    int panelWidth = 1144;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
