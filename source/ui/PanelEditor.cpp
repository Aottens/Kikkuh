#include "PanelEditor.h"
#include "Knob.h"
#include "PanelColours.h"

namespace
{
constexpr int stageWidth = 1154, stageHeight = 318;

juce::Colour colour (std::uint32_t argb) { return juce::Colour (argb); }

const PanelLookAndFeel& lookOf (const juce::Component& c) { return static_cast<const PanelLookAndFeel&> (c.getLookAndFeel()); }

// FAST-01n: a flash is full for the first 15 % of 260 ms, then fades out linearly.
struct Flash
{
    double started = -1.0e9;
    float level = 0.0f;

    void trigger() { started = juce::Time::getMillisecondCounterHiRes(); }

    // True while the level changes, so the owner repaints only what is moving.
    bool update()
    {
        const auto t = static_cast<float> ((juce::Time::getMillisecondCounterHiRes() - started) / 260.0);
        const auto next = t < 0.15f ? 1.0f : juce::jlimit (0.0f, 1.0f, 1.0f - (t - 0.15f) / 0.85f);
        return ! juce::approximatelyEqual (std::exchange (level, next), next);
    }

    juce::Colour mix() const { return colour (panel::well).interpolatedWith (colour (panel::sounding), level); }
};

class Pad : public juce::Button
{
public:
    Pad() : Button ("Hit")
    {
        setComponentID ("audition_pad");
        setTitle ("Hit");
        setTriggeredOnMouseDown (true);
    }

    Flash flash;

    void paintButton (juce::Graphics& g, bool, bool) override
    {
        g.fillAll (flash.mix());
        g.setColour (colour (panel::ink));
        g.drawRect (getLocalBounds(), 2);
        g.setFont (lookOf (*this).font (PanelLookAndFeel::Cut::name, 25.0f, 0.04f));
        g.drawText ("HIT", getLocalBounds(), juce::Justification::centred);
    }
};

// A switch: ink-edged, filled with ink when on. Loop 1/4 has a lamp that lights on loop hits only.
class Switch : public juce::ToggleButton
{
public:
    Switch (const juce::String& text, const juce::String& id, const juce::String& title, bool withLamp = false)
        : ToggleButton (text), lamp (withLamp)
    {
        setComponentID (id);
        setTitle (title);
    }

    Flash flash;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;

    juce::Rectangle<int> lampBounds() const { return juce::Rectangle<int> (15, 9).withCentre ({ getWidth() - 8 - 8, getHeight() / 2 }); }

    void paintButton (juce::Graphics& g, bool, bool) override
    {
        const auto on = getToggleState();
        const auto& look = lookOf (*this);
        g.setColour (colour (panel::ink));
        if (on)
            g.fillAll();
        g.drawRect (getLocalBounds(), 2);

        g.setColour (colour (on ? panel::plate : panel::ink));
        auto font = look.font (PanelLookAndFeel::Cut::label, 13.0f);
        const auto text = getButtonText();
        g.setFont (font);
        g.drawText (text, 8, 0, getWidth() - 16, getHeight(), juce::Justification::centredLeft);
        if (lamp)
        {
            g.setFont (look.font (PanelLookAndFeel::Cut::value, 13.0f));
            g.drawText ("1/4", 8 + juce::roundToInt (PanelLookAndFeel::widthOf (font, text + " ")), 0, 30, getHeight(), juce::Justification::centredLeft);
            g.setColour (flash.mix());
            g.fillRect (lampBounds());
            g.setColour (colour (panel::print));
            g.drawRect (lampBounds(), 1);
        }
    }

private:
    bool lamp;
};

// Laid out as a knob: the note on the disc line, its caption on the label line, the cents on the value line.
class TailReadout : public juce::Label
{
public:
    TailReadout()
    {
        setComponentID ("tail_readout");
        setTitle ("Tail lands on");
        setInterceptsMouseClicks (false, false);
    }

    void paint (juce::Graphics& g) override
    {
        const auto& look = lookOf (*this);
        const auto at = [this] (int y, int height) { return juce::Rectangle<int> (0, y, getWidth(), height); };
        g.setColour (colour (panel::ink));
        g.setFont (look.font (PanelLookAndFeel::Cut::title, 25.0f));
        g.drawText (getText().upToFirstOccurrenceOf (" ", false, false), at (11, 30), juce::Justification::centred);
        g.setFont (look.font (PanelLookAndFeel::Cut::label, 13.0f));
        g.drawText ("Tail lands on", at (52, 16), juce::Justification::centred);
        g.setFont (look.font (PanelLookAndFeel::Cut::value, 13.0f));
        g.drawText (getText().fromFirstOccurrenceOf (" ", false, false), at (68, 17), juce::Justification::centred);
    }
};

struct KnobSpec
{
    const char* id;
    const char* label;
    int centreX, top;
    Knob::Size size;
};

using S = Knob::Size;
constexpr KnobSpec knobSpecs[] = {
    { "play_velocity", "Velocity", 41, 216, S::little },
    { "body_pitch_amount", "Amount", 250, 70, S::large },
    { "body_pitch_decay", "Decay", 338, 70, S::large },
    { "body_pitch_shape", "Shape", 426, 70, S::medium },
    { "body_sweep2_amount", "Amount", 250, 216, S::little },
    { "body_sweep2_decay", "Decay", 338, 216, S::little },
    { "body_tune", "Tune", 530, 70, S::large },
    { "body_wave", "Wave", 618, 70, S::wave },
    { "body_start", "Start", 530, 216, S::little },
    { "body_amp_attack", "Attack", 722, 216, S::little },
    { "body_amp_hold", "Hold", 810, 216, S::little },
    { "body_amp_decay", "Decay", 722, 70, S::large },
    { "body_amp_shape", "Shape", 810, 70, S::medium },
    { "body_drive", "Drive", 914, 70, S::large },
    { "body_filter_cutoff", "Cutoff", 1018, 70, S::large },
    { "body_filter_res", "Resonance", 1106, 70, S::medium },
    { "body_filter_env", "Env", 1018, 216, S::little },
};
} // namespace

class PanelEditor::Stage : public juce::Component
{
public:
    Stage (PluginProcessor& p, PanelLookAndFeel& lf)
        : loop ("Loop", "audition_loop", "Loop", true),
          keyTrack ("Key Track", "play_key_track", "Key Track"),
          tailAnchor ("Tail Anchor", "body_tail_anchor", "Tail Anchor"),
          lastKicks (p.getKickCount()), lastLoopHits (p.getLoopHitCount())
    {
        setLookAndFeel (&lf); // before the children, which build their text boxes from it
        setOpaque (true);
        setSize (stageWidth, stageHeight);

        auto& state = p.getValueTreeState();
        for (const auto& spec : knobSpecs)
        {
            auto& knob = *knobs.emplace_back (std::make_unique<Knob> (state, spec.id, spec.label, spec.size));
            addAndMakeVisible (knob);
            knob.setBounds (spec.centreX - knob.width() / 2, spec.top, knob.width(), knob.height());
        }

        pad.setBounds (22, 82, 160, 67);
        pad.onClick = [&p] { p.pressPad(); };
        loop.setBounds (22, 157, 160, 32);
        loop.onClick = [this, &p] { p.setLoop (loop.getToggleState()); }; // also from setToggleState (FAST-01j)
        keyTrack.setBounds (104, 226, 78, 32);
        tailAnchor.setBounds (383, 226, 86, 32);
        keyTrack.attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (state, "play_key_track", keyTrack);
        tailAnchor.attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (state, "body_tail_anchor", tailAnchor);

        tail.setBounds (574, 216, 88, 85);
        tail.setText (p.getTailReadout(), juce::dontSendNotification);
        for (auto* c : std::initializer_list<juce::Component*> { &pad, &loop, &keyTrack, &tailAnchor, &tail })
            addAndMakeVisible (c);
        sendLookAndFeelChange(); // the knobs' text boxes were built before they had this look
    }

    ~Stage() override { setLookAndFeel (nullptr); }

    // FAST-05f, FAST-01n: only what changed is set and repainted.
    void refresh (PluginProcessor& p)
    {
        if (const auto readout = p.getTailReadout(); readout != tail.getText())
            tail.setText (readout, juce::dontSendNotification);
        if (const auto kicks = p.getKickCount(); kicks != std::exchange (lastKicks, kicks))
            pad.flash.trigger();
        if (const auto hits = p.getLoopHitCount(); hits != std::exchange (lastLoopHits, hits))
            loop.flash.trigger();
        if (pad.flash.update())
            pad.repaint();
        if (loop.flash.update())
            loop.repaint (loop.lampBounds());
    }

    void paint (juce::Graphics& g) override
    {
        const auto& lf = lookOf (*this);
        g.fillAll (colour (panel::plate));
        g.setColour (colour (panel::ink));
        g.setFont (lf.font (PanelLookAndFeel::Cut::name, 40.0f, -0.012f));
        g.drawText ("Kikkuh", 22, 14, 300, 40, juce::Justification::topLeft);
        g.setColour (colour (panel::print));
        g.setFont (lf.font (PanelLookAndFeel::Cut::value, 12.0f));
        g.drawText ("by Colourmotion", 23, 58, 200, 16, juce::Justification::topLeft);

        drawFlow (g, lf);
        drawSweep (g, lf);
    }

private:
    // The printed signal line, in the real order of the signal: arrowheads end each stage, titles break the line.
    void drawFlow (juce::Graphics& g, const PanelLookAndFeel& lf)
    {
        g.setColour (colour (panel::ink));
        g.fillRect (190.0f, 37.0f - 1.25f, 942.0f, 2.5f);
        for (const int x : { 210, 490, 682, 874, 978, 1132 })
        {
            juce::Path arrow;
            arrow.addTriangle ((float) x - 9.0f, 31.0f, (float) x, 37.0f, (float) x - 9.0f, 43.0f);
            g.fillPath (arrow);
        }

        const auto font = lf.font (PanelLookAndFeel::Cut::title, 15.0f, 0.07f);
        const std::pair<const char*, int> titles[] = { { "PITCH", 210 }, { "OSC", 490 }, { "AMP", 682 }, { "DRIVE", 874 }, { "FILTER", 978 } };
        for (const auto& [text, x] : titles)
        {
            const auto width = juce::roundToInt (PanelLookAndFeel::widthOf (font, text));
            g.setColour (colour (panel::plate));
            g.fillRect (x, 26, width + 14, 22);
            g.setColour (colour (panel::ink));
            g.setFont (font);
            g.drawText (text, x + 6, 26, width + 4, 22, juce::Justification::centredLeft);
        }
    }

    void drawSweep (juce::Graphics& g, const PanelLookAndFeel& lf)
    {
        g.setColour (colour (panel::print));
        g.fillRect (216.0f, 208.0f, 156.0f, 1.5f);
        const auto font = lf.font (PanelLookAndFeel::Cut::title, 12.0f, 0.06f);
        const auto width = juce::roundToInt (PanelLookAndFeel::widthOf (font, "SWEEP 2"));
        g.setColour (colour (panel::plate));
        g.fillRect (224, 200, width + 10, 15);
        g.setColour (colour (panel::print));
        g.setFont (font);
        g.drawText ("SWEEP 2", 229, 200, width + 4, 15, juce::Justification::centredLeft);
    }

    std::vector<std::unique_ptr<Knob>> knobs;
    Pad pad;
    Switch loop, keyTrack, tailAnchor;
    TailReadout tail;
    int lastKicks, lastLoopHits;
};

PanelEditor::PanelEditor (PluginProcessor& p)
    : AudioProcessorEditor (p), plugin (p), stage (std::make_unique<Stage> (p, look))
{
    const auto width = plugin.getPanelWidth(); // before the limits resize the window
    addAndMakeVisible (*stage);
    setResizable (true, true);
    setResizeLimits (juce::roundToInt (stageWidth * 0.8), juce::roundToInt (stageHeight * 0.8), stageWidth * 2, stageHeight * 2);
    getConstrainer()->setFixedAspectRatio (static_cast<double> (stageWidth) / stageHeight);
    setSize (width, juce::roundToInt (width * static_cast<double> (stageHeight) / stageWidth));
    startTimerHz (30);
}

PanelEditor::~PanelEditor()
{
    plugin.setLoop (false); // FAST-01j: its switch can no longer be seen
}

void PanelEditor::paint (juce::Graphics& g)
{
    g.fillAll (colour (panel::plate));
}

void PanelEditor::resized()
{
    plugin.setPanelWidth (getWidth());
    stage->setTransform (juce::AffineTransform::scale (static_cast<float> (getWidth()) / stageWidth));
}

void PanelEditor::timerCallback()
{
    stage->refresh (plugin);
}
