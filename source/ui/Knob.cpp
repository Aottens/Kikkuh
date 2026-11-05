#include "Knob.h"
#include "PanelColours.h"
#include <PluginProcessor.h>

namespace
{
const juce::StringArray waveNames { "sine", "triangle", "saw", "square" };
}

Knob::Knob (juce::AudioProcessorValueTreeState& state, const juce::String& id, const juce::String& labelText, Size knobSize)
    : label (labelText), size (knobSize), parameter (*state.getParameter (id))
{
    setComponentID (id);
    setTitle (parameter.getName (64));
    setSliderStyle (RotaryHorizontalVerticalDrag);
    setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    setTextBoxStyle (TextBoxBelow, false, width(), 17);
    if (size == Size::large)
        setColour (textBoxTextColourId, juce::Colour (panel::ink));

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, id, *this);
    defaultProportion = parameter.getDefaultValue();

    // The attachment installs its own text functions and reset, so these come after it.
    setDoubleClickReturnValue (true, parameter.convertFrom0to1 (parameter.getDefaultValue()), juce::ModifierKeys::commandModifier);
    textFromValueFunction = [this] (double value) { return textFor (value); };
    valueFromTextFunction = [this] (const juce::String& text) { return valueFor (text); };
    updateText();
}

int Knob::diameter() const
{
    return size == Size::large ? 68 : size == Size::little ? 38 : 52;
}

// A value with its unit (FAST-05j); Cutoff from 1000 Hz up in kHz, Wave by name on a whole position.
juce::String Knob::textFor (double value) const
{
    const auto id = getComponentID();
    if (id == "body_wave")
    {
        const auto nearest = juce::roundToInt (value);
        return std::abs (value - nearest) < 0.005 ? waveNames[juce::jlimit (0, 3, nearest)] : juce::String (value, 2);
    }
    if (id == "body_filter_cutoff")
        return value >= 999.5 ? juce::String (value / 1000.0, value >= 9995.0 ? 1 : 2) + " kHz" : juce::String (juce::roundToInt (value)) + " Hz";

    const auto text = parameter.getText (parameter.convertTo0to1 (static_cast<float> (value)), 0);
    return parameter.getLabel().isEmpty() ? text : text + " " + parameter.getLabel();
}

double Knob::valueFor (const juce::String& text) const
{
    const auto typed = text.trim();
    if (getComponentID() == "body_tune")
        return PluginProcessor::noteFromText (typed);
    if (getComponentID() == "body_wave")
        for (int i = 0; i < waveNames.size(); ++i)
            if (typed.length() >= 2 && waveNames[i].startsWithIgnoreCase (typed))
                return i;
    if (! typed.containsAnyOf ("0123456789"))
        return getValue();

    const auto rest = typed.trimCharactersAtStart ("+-.0123456789 ");
    return typed.getDoubleValue() * (rest.startsWithIgnoreCase ("k") ? 1000.0 : 1.0);
}

// The value being dragged shows as plate on ink.
void Knob::markValue (bool dragging)
{
    for (auto* child : getChildren())
        if (auto* value = dynamic_cast<juce::Label*> (child))
        {
            value->setColour (juce::Label::backgroundColourId, dragging ? juce::Colour (panel::ink) : juce::Colours::transparentBlack);
            value->setColour (juce::Label::textColourId, dragging ? juce::Colour (panel::plate) : findColour (textBoxTextColourId));
        }
}

void Knob::mouseDown (const juce::MouseEvent& e)
{
    Slider::mouseDown (e); // gestures, and the Cmd-click reset
    lastY = e.position.y;
    norm = valueToProportionOfLength (getValue());
    markValue (true);
}

// 240 points cover the range, 1200 with Shift; incremental, so Shift pressed or released mid-drag does not jump.
void Knob::mouseDrag (const juce::MouseEvent& e)
{
    norm = juce::jlimit (0.0, 1.0, norm + (lastY - e.position.y) / (e.mods.isShiftDown() ? 1200.0 : 240.0));
    lastY = e.position.y;
    setValue (proportionOfLengthToValue (norm), juce::sendNotificationSync);
}

void Knob::mouseUp (const juce::MouseEvent& e)
{
    markValue (false);
    Slider::mouseUp (e);
}
