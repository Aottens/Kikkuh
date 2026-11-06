#pragma once

#include <dsp/AuditionLoop.h>
#include <dsp/BodyVoice.h>
#include <dsp/LayerVoice.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

class PluginProcessor : public juce::AudioProcessor, private juce::AsyncUpdater
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

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

    // LAYER-04: your own sample in CLICK (section 0) or TOP (1). Message thread. False, and nothing changes, when no file has a
    // supported type; otherwise the first readable one is read on a background thread and published on the message thread.
    static bool canLoadSample (const juce::File& file);
    bool loadSample (int section, const juce::StringArray& paths);
    void waitForSampleLoads(); // blocks until every started load is published
    juce::String getSampleText (int section) const; // the file's name, or "Drop a sample", "Cannot read ...", "Missing: ..."

    // FAST-05: what the panel needs. Message thread; the panel width is not part of the saved state.
    juce::AudioProcessorValueTreeState& getValueTreeState() { return apvts; }
    int getPanelWidth() const { return panelWidth; }
    void setPanelWidth (int width) { panelWidth = width; }

private:
    static constexpr int stateVersion = 3;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    dsp::BodyParams readParams() const;
    dsp::LayerParams readLayer (size_t first, size_t start) const;
    void migrate (juce::XmlElement& xml, int fromVersion);
    void beginLoop (int numSamples, bool on);

    // LAYER-04b: a loaded sample, ready to publish. `ok` false: the file could not be read.
    struct Loaded
    {
        int section = 0;
        bool ok = false;
        juce::String name;
        std::vector<float> data;
        double rate = 0.0;
    };
    void handleAsyncUpdate() override;
    void publishSample (int section, const juce::String& name, std::vector<float> data, double rate, const juce::String& text);
    const dsp::LayerSample* takeSample (size_t section);

    juce::AudioProcessorValueTreeState apvts;
    std::array<std::atomic<float>*, 33> raw {};
    dsp::BodyVoice voice;
    dsp::LayerVoice click, top;
    dsp::AuditionLoop loop;
    double sampleRate = 44100.0;
    std::atomic<bool> padPressed { false }, loopOn { false };
    std::atomic<int> kicks { 0 }, loopHits { 0 };
    int panelWidth = 1144;

    // The samples: the message thread allocates and frees, the audio thread only swaps pointers, at a note-on. A published sample
    // waits in `pending`; the audio thread moves it to `current` and the one it replaces to `retired`, which the next publish frees.
    std::array<std::atomic<dsp::LayerSample*>, 2> pendingSample {}, retiredSample {};
    std::array<dsp::LayerSample*, 2> currentSample {}; // audio thread
    struct Held // what the state saves, kept apart from the audio thread's copy
    {
        juce::String name, text { "Drop a sample" };
        std::vector<float> data;
        double rate = 0.0;
    };
    mutable juce::CriticalSection heldLock;
    std::array<Held, 2> held;
    juce::CriticalSection loadedLock;
    std::vector<Loaded> loaded;
    std::atomic<int> loadsInFlight { 0 };
    juce::ThreadPool loader { 1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
