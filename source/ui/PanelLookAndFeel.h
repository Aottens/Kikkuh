#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// FAST-05: the printed panel. Flat, no gradients, shadows or glows; colours from PanelColours.h only.
class PanelLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // Archivo, instanced in four cuts: the name, the section titles, the labels and the values.
    enum class Cut { name, title, label, value };

    PanelLookAndFeel();

    // Sizes are CSS-like: the em in points at 100 %. `tracking` is in em.
    juce::Font font (Cut cut, float size, float tracking = 0.0f) const;
    static float widthOf (const juce::Font& font, const juce::String& text);

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float position, float startAngle, float endAngle, juce::Slider&) override;
    juce::Slider::SliderLayout getSliderLayout (juce::Slider&) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;

private:
    juce::Typeface::Ptr faces[4];
};
