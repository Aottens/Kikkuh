#include "PanelLookAndFeel.h"
#include "Knob.h"
#include "PanelColours.h"
#include <BinaryData.h>

namespace
{
constexpr std::uint32_t mark = 0xffe9eae5; // the pointer on the disc

juce::Point<float> onCircle (juce::Point<float> centre, float angle, float radius)
{
    return centre + juce::Point<float> (std::sin (angle), -std::cos (angle)) * radius;
}

void drawSpoke (juce::Graphics& g, juce::Point<float> centre, float angle, float from, float to, float thickness)
{
    g.drawLine ({ onCircle (centre, angle, from), onCircle (centre, angle, to) }, thickness);
}
}

PanelLookAndFeel::PanelLookAndFeel()
{
    faces[0] = juce::Typeface::createSystemTypefaceFor (BinaryData::ArchivoName_ttf, BinaryData::ArchivoName_ttfSize);
    faces[1] = juce::Typeface::createSystemTypefaceFor (BinaryData::ArchivoTitle_ttf, BinaryData::ArchivoTitle_ttfSize);
    faces[2] = juce::Typeface::createSystemTypefaceFor (BinaryData::ArchivoLabel_ttf, BinaryData::ArchivoLabel_ttfSize);
    faces[3] = juce::Typeface::createSystemTypefaceFor (BinaryData::ArchivoValue_ttf, BinaryData::ArchivoValue_ttfSize);

    const juce::Colour clear = juce::Colours::transparentBlack;
    setColour (juce::Slider::textBoxTextColourId, juce::Colour (panel::print));
    setColour (juce::Slider::textBoxBackgroundColourId, clear);
    setColour (juce::Slider::textBoxOutlineColourId, clear);
    setColour (juce::TextEditor::focusedOutlineColourId, clear);
    setColour (juce::TextEditor::outlineColourId, clear);
}

juce::Font PanelLookAndFeel::font (Cut cut, float size, float tracking) const
{
    return juce::Font (juce::FontOptions (faces[static_cast<size_t> (cut)]).withPointHeight (size).withKerningFactor (tracking));
}

float PanelLookAndFeel::widthOf (const juce::Font& font, const juce::String& text)
{
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText (font, text, 0.0f, 0.0f);
    return glyphs.getBoundingBox (0, -1, true).getWidth();
}

// The disc with its pointer, the printed ticks and the name under it. The value is the slider's text box.
void PanelLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float position, float startAngle, float endAngle, juce::Slider& slider)
{
    auto& knob = dynamic_cast<Knob&> (slider);
    const auto centre = juce::Rectangle<int> (x, y, width, height).getCentre().toFloat();
    const auto radius = static_cast<float> (knob.diameter()) * 0.5f;
    const auto angleAt = [&] (double proportion) { return startAngle + static_cast<float> (proportion) * (endAngle - startAngle); };

    const auto ticks = knob.size == Knob::Size::little ? 7 : 11;
    g.setColour (juce::Colour (panel::print));
    for (int i = 0; i < ticks; ++i)
        drawSpoke (g, centre, angleAt (i / (ticks - 1.0)), radius + 2.0f, radius + 6.0f, 1.1f);
    g.setColour (juce::Colour (panel::ink));
    drawSpoke (g, centre, angleAt (knob.defaultProportion), radius + 1.5f, radius + 7.0f, 2.4f); // the default

    g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre));
    g.setColour (juce::Colour (mark));
    drawSpoke (g, centre, angleAt (position), radius * 0.42f, radius - 1.5f, 3.0f);

    if (knob.size == Knob::Size::wave)
    {
        g.setColour (juce::Colour (panel::print));
        g.setFont (font (Cut::value, 11.0f));
        const char* names[] = { "SIN", "TRI", "SAW", "SQR" };
        for (int i = 0; i < 4; ++i)
        {
            const auto angle = angleAt (i / 3.0);
            const auto at = onCircle (centre, angle, radius + 15.0f);
            // Kept inside the knob's area, so a name at the side is not clipped.
            const auto area = juce::Rectangle<float> (24.0f, 14.0f).withCentre (at).constrainedWithin (juce::Rectangle<int> (x, y, width, height).toFloat());
            g.drawText (names[i], area, juce::Justification::centred);
        }
    }

    g.setColour (juce::Colour (panel::ink));
    g.setFont (font (Cut::label, 13.0f));
    g.drawText (knob.label, 0, y + height, slider.getWidth(), 16, juce::Justification::centred);
}

juce::Slider::SliderLayout PanelLookAndFeel::getSliderLayout (juce::Slider& slider)
{
    const auto& knob = dynamic_cast<Knob&> (slider);
    return { { 0, 0, slider.getWidth(), knob.box() },
             { 0, knob.box() + 16, slider.getWidth(), 17 } };
}

juce::Label* PanelLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* label = LookAndFeel_V4::createSliderTextBox (slider);
    label->setFont (font (Cut::value, 13.0f));
    label->setBorderSize ({ 0, 2, 0, 2 });
    label->setColour (juce::Label::backgroundWhenEditingColourId, juce::Colour (panel::ink));
    label->setColour (juce::Label::textWhenEditingColourId, juce::Colour (panel::plate));
    label->setColour (juce::Label::outlineWhenEditingColourId, juce::Colour (panel::ink));
    return label;
}
