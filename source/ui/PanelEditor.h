#pragma once

#include "PanelLookAndFeel.h"
#include <PluginProcessor.h>

// FAST-05: the whole panel on one screen. Everything lives in one stage of 1144 x 598 points, scaled to the window.
class PanelEditor : public juce::AudioProcessorEditor, public juce::FileDragAndDropTarget, public juce::Timer
{
public:
    explicit PanelEditor (PluginProcessor&);
    ~PanelEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray&, int x, int y) override; // LAYER-04a: the second band, CLICK left of the section line, TOP right
    void timerCallback() override; // 30 Hz: the tail readout and the lamps

private:
    class Stage;

    PluginProcessor& plugin;
    PanelLookAndFeel look;
    std::unique_ptr<Stage> stage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PanelEditor)
};
