#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// FAST-05: a rotary knob on one parameter. Dragging is vertical and incremental (FAST-05d).
class Knob : public juce::Slider
{
public:
    // The disc is 68 or 38 points across; the Wave knob has room around it for its printed positions.
    // A top-row knob (all but little) has a rotary area 86 tall, a bottom-row one 54, so a row shares one disc line.
    enum class Size { little, large, wave };

    Knob (juce::AudioProcessorValueTreeState& state, const juce::String& id, const juce::String& label, Size size);

    int diameter() const;
    int box() const { return size == Size::little ? 54 : 86; }
    int width() const { return size == Size::wave ? 86 : size == Size::large ? 82 : 70; }
    int height() const { return box() + 16 + 17; }

    const juce::String label;
    const Size size;
    double defaultProportion = 0.0;

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    juce::String textFor (double value) const;
    double valueFor (const juce::String& text) const;
    void markValue (bool dragging);

    juce::RangedAudioParameter& parameter;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    float lastY = 0.0f;
    double norm = 0.0;
};
