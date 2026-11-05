#include "AllocationGuard.h"
#include "Measure.h"
#include "Render.h"
#include <PluginProcessor.h>
#include <ui/PanelColours.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <cmath>
#include <juce_gui_basics/juce_gui_basics.h>
#include <limits>
#include <memory>
#include <optional>

namespace
{
constexpr double rate48 = 48000.0;

juce::RangedAudioParameter* findParam (juce::AudioProcessor& plugin, const juce::String& id)
{
    for (auto* p : plugin.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p); ranged != nullptr && ranged->getParameterID() == id)
            return ranged;
    return nullptr;
}

void setParam (juce::AudioProcessor& plugin, const juce::String& id, float plainValue)
{
    auto* p = findParam (plugin, id);
    REQUIRE (p != nullptr);
    p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
}

// A short kick whose first sample is its full level, so every hit can be read to the sample (PLAY-01b).
struct ShortKick : PluginProcessor
{
    ShortKick()
    {
        for (const auto& [id, value] : { std::pair<const char*, float> { "body_drive", 0.0f }, { "body_tail_anchor", 0.0f }, { "body_sweep2_amount", 0.0f },
                                         { "body_pitch_amount", 0.0f }, { "body_amp_decay", 10.0f } })
            setParam (*this, id, value);
    }
};

std::vector<int> hitsIn (const std::vector<float>& x, double rate) { return measure::hits (x, 0.5f, static_cast<int> (0.03 * rate)); }

std::vector<float> render (juce::AudioProcessor& plugin, double rate, double seconds, const std::vector<MidiAt>& events = {},
                           std::vector<int> sizes = { 256 }, const std::function<void (double)>& before = {})
{
    const auto out = renderEventsStereo (plugin, rate, seconds, events, std::move (sizes), before);
    return { out.getReadPointer (0), out.getReadPointer (0) + out.getNumSamples() };
}

struct FakeHead : juce::AudioPlayHead
{
    double ppq = 0.0;
    std::optional<double> bpm = 120.0;
    bool playing = true, hasPosition = true;

    juce::Optional<PositionInfo> getPosition() const override
    {
        if (! hasPosition)
            return {};
        PositionInfo info;
        info.setPpqPosition (ppq);
        if (bpm)
            info.setBpm (*bpm);
        info.setIsPlaying (playing);
        return info;
    }
};

// The loop running along a rolling transport that starts at `startPpq`.
std::vector<int> loopAlongTransport (double rate, double bpm, double startPpq, double seconds, std::vector<int> sizes = { 256 }, double noise = 0.0)
{
    ShortKick plugin;
    FakeHead head;
    head.bpm = bpm;
    plugin.setPlayHead (&head);
    plugin.setLoop (true);
    int block = 0;
    return hitsIn (render (plugin, rate, seconds, {}, std::move (sizes), [&] (double t) {
        head.ppq = startPpq + t * bpm / 60.0 + (block++ % 2 == 0 ? noise : -noise);
    }), rate);
}

void checkOnGrid (const std::vector<int>& hits, double rate, double bpm, double firstSample, int count)
{
    REQUIRE (static_cast<int> (hits.size()) == count);
    for (int i = 0; i < count; ++i)
        CHECK (std::abs (hits[static_cast<size_t> (i)] - (firstSample + i * 60.0 / bpm * rate)) <= 1.0);
}

// --- the panel ---

struct Panel
{
    juce::ScopedJuceInitialiser_GUI gui;
    PluginProcessor plugin;
    std::unique_ptr<juce::AudioProcessorEditor> editor { plugin.createEditor() };
};

juce::Component* findById (juce::Component& parent, const juce::String& id)
{
    for (auto* child : parent.getChildren())
    {
        if (child->getComponentID() == id)
            return child;
        if (auto* found = findById (*child, id))
            return found;
    }
    return nullptr;
}

int countById (juce::Component& parent, const juce::String& id)
{
    int count = 0;
    for (auto* child : parent.getChildren())
        count += (child->getComponentID() == id ? 1 : 0) + countById (*child, id);
    return count;
}

template <typename Visit>
void forEachComponent (juce::Component& parent, Visit&& visit)
{
    for (auto* child : parent.getChildren())
    {
        visit (*child);
        forEachComponent (*child, visit);
    }
}

juce::Slider& knob (Panel& panel, const juce::String& id)
{
    auto* slider = dynamic_cast<juce::Slider*> (findById (*panel.editor, id));
    REQUIRE (slider != nullptr);
    return *slider;
}

juce::Rectangle<float> boundsIn (juce::Component& editor, juce::Component& control)
{
    return editor.getLocalArea (&control, control.getLocalBounds().toFloat());
}

// The sections of the panel, in the order of the signal (FAST-05b).
const std::vector<std::vector<juce::String>> sections {
    { "audition_pad", "audition_loop", "play_velocity", "play_key_track" },
    { "body_pitch_amount", "body_pitch_decay", "body_pitch_shape", "body_sweep2_amount", "body_sweep2_decay", "body_tail_anchor" },
    { "body_tune", "body_wave", "body_start", "tail_readout" },
    { "body_amp_decay", "body_amp_shape", "body_amp_attack", "body_amp_hold" },
    { "body_drive" },
    { "body_filter_cutoff", "body_filter_res", "body_filter_env" }
};

void checkLayout (juce::AudioProcessorEditor& editor)
{
    std::vector<juce::Rectangle<float>> all;
    float rightmostCentre = -1.0f;
    for (const auto& section : sections)
    {
        float leftmost = 1.0e9f, rightmost = -1.0f;
        for (const auto& id : section)
        {
            auto* control = findById (editor, id);
            INFO (id);
            REQUIRE (control != nullptr);
            CHECK (control->isVisible());
            const auto bounds = boundsIn (editor, *control);
            CHECK (editor.getLocalBounds().toFloat().expanded (0.5f).contains (bounds));
            for (const auto& other : all)
                CHECK (bounds.getIntersection (other).getWidth() * bounds.getIntersection (other).getHeight() < 1.0f);
            all.push_back (bounds);
            leftmost = juce::jmin (leftmost, bounds.getCentreX());
            rightmost = juce::jmax (rightmost, bounds.getCentreX());
        }
        CHECK (leftmost > rightmostCentre);
        rightmostCentre = rightmost;
    }
}

juce::MouseEvent mouseAt (juce::Component& c, juce::Point<float> position, juce::Point<float> down, juce::ModifierKeys mods, bool dragged)
{
    return { juce::Desktop::getInstance().getMainMouseSource(), position, mods.withFlags (juce::ModifierKeys::leftButtonModifier),
             1.0f, 0.0f, 0.0f, 0.0f, 0.0f, &c, &c, juce::Time::getCurrentTime(), down, juce::Time::getCurrentTime(), 1, dragged };
}

// A drag of `points` upward, in steps, with Shift held from the given step on; returns nothing, the knob moves.
void drag (juce::Slider& s, float points, juce::ModifierKeys mods = {})
{
    const juce::Point<float> start { 10.0f, 100.0f };
    s.mouseDown (mouseAt (s, start, start, mods, false));
    s.mouseDrag (mouseAt (s, start.translated (0.0f, -points), start, mods, true));
    s.mouseUp (mouseAt (s, start.translated (0.0f, -points), start, mods, true));
}

double luminance (juce::Colour c)
{
    const auto channel = [] (float v) { return v <= 0.03928f ? v / 12.92 : std::pow ((v + 0.055) / 1.055, 2.4); };
    return 0.2126 * channel (c.getFloatRed()) + 0.7152 * channel (c.getFloatGreen()) + 0.0722 * channel (c.getFloatBlue());
}
} // namespace

// --- FAST-05 One screen ---

TEST_CASE ("fast_05a_every_parameter_has_exactly_one_control")
{
    Panel panel;
    for (auto* p : panel.plugin.getParameters())
    {
        const auto id = dynamic_cast<juce::RangedAudioParameter*> (p)->getParameterID();
        INFO (id);
        CHECK (countById (*panel.editor, id) == 1);
        auto* control = findById (*panel.editor, id);
        REQUIRE (control != nullptr);
        CHECK ((dynamic_cast<juce::Slider*> (control) != nullptr || dynamic_cast<juce::Button*> (control) != nullptr));
    }
}

TEST_CASE ("fast_05b_one_screen_in_signal_order_at_every_size")
{
    Panel panel;
    forEachComponent (*panel.editor, [] (juce::Component& c) {
        CHECK (dynamic_cast<juce::TabbedComponent*> (&c) == nullptr);
        CHECK (dynamic_cast<juce::Viewport*> (&c) == nullptr);
    });

    const auto width = panel.editor->getWidth();
    REQUIRE (width == 1154);
    for (const auto scale : { 1.0, 0.8, 2.0 })
    {
        panel.editor->setSize (juce::roundToInt (1154 * scale), juce::roundToInt (318 * scale));
        INFO (scale);
        checkLayout (*panel.editor);

        // It draws: mostly plate, with ink on it.
        const auto image = panel.editor->createComponentSnapshot (panel.editor->getLocalBounds());
        const auto plate = juce::Colour (panel::plate), ink = juce::Colour (panel::ink);
        int plates = 0, inks = 0;
        for (int y = 0; y < image.getHeight(); y += 2)
            for (int x = 0; x < image.getWidth(); x += 2)
            {
                const auto colour = image.getPixelAt (x, y);
                plates += colour == plate ? 1 : 0;
                inks += colour == ink ? 1 : 0;
            }
        const auto seen = (image.getWidth() / 2) * (image.getHeight() / 2);
        CHECK (plates > seen / 2);
        CHECK (inks > seen / 50);

        if (const auto path = juce::SystemStats::getEnvironmentVariable ("KIKKUH_SNAPSHOT", {}); path.isNotEmpty())
        {
            juce::File file (path + juce::String (juce::roundToInt (scale * 100)) + ".png");
            file.deleteFile();
            juce::FileOutputStream stream (file);
            juce::PNGImageFormat().writeImageToStream (image, stream);
        }
    }
}

TEST_CASE ("fast_05c_resizable_at_a_fixed_ratio_and_the_size_comes_back")
{
    Panel panel;
    CHECK (panel.editor->getWidth() == 1154);
    CHECK (panel.editor->getHeight() == 318);
    CHECK (panel.editor->isResizable());
    auto* limits = panel.editor->getConstrainer();
    REQUIRE (limits != nullptr);
    CHECK (limits->getFixedAspectRatio() == Catch::Approx (1154.0 / 318.0).margin (0.001));
    CHECK (limits->getMinimumWidth() == Catch::Approx (1154 * 0.8).margin (1.0));
    CHECK (limits->getMaximumWidth() == 2308);

    panel.editor->setSize (1442, 397);
    panel.editor = nullptr;
    panel.editor.reset (panel.plugin.createEditor());
    CHECK (panel.editor->getWidth() == 1442);
}

TEST_CASE ("fast_05d_a_knob_moves_as_a_pro_expects")
{
    Panel panel;
    auto& velocity = knob (panel, "play_velocity"); // linear, 0..100
    auto* param = findParam (panel.plugin, "play_velocity");

    SECTION ("control and parameter follow each other")
    {
        velocity.setValue (25.0, juce::sendNotificationSync);
        CHECK (param->convertFrom0to1 (param->getValue()) == Catch::Approx (25.0f).margin (0.01f));
        setParam (panel.plugin, "play_velocity", 75.0f);
        CHECK (velocity.getValue() == Catch::Approx (75.0).margin (0.01));
    }
    SECTION ("240 points cover the whole range, vertically; Shift is 5 times finer")
    {
        velocity.setValue (0.0, juce::sendNotificationSync);
        drag (velocity, 120.0f);
        CHECK (velocity.getValue() == Catch::Approx (50.0).margin (0.5));
        velocity.setValue (0.0, juce::sendNotificationSync);
        drag (velocity, 120.0f, juce::ModifierKeys::shiftModifier);
        CHECK (velocity.getValue() == Catch::Approx (10.0).margin (0.5));
        CHECK (velocity.getSliderStyle() == juce::Slider::RotaryHorizontalVerticalDrag);
    }
    SECTION ("pressing Shift during a drag does not jump")
    {
        velocity.setValue (0.0, juce::sendNotificationSync);
        const juce::Point<float> start { 10.0f, 200.0f };
        velocity.mouseDown (mouseAt (velocity, start, start, {}, false));
        velocity.mouseDrag (mouseAt (velocity, start.translated (0.0f, -60.0f), start, {}, true));
        CHECK (velocity.getValue() == Catch::Approx (25.0).margin (0.5));
        // The same mouse position with Shift now held: nothing moved, so the value stays.
        velocity.mouseDrag (mouseAt (velocity, start.translated (0.0f, -60.0f), start, juce::ModifierKeys::shiftModifier, true));
        CHECK (velocity.getValue() == Catch::Approx (25.0).margin (0.5));
        velocity.mouseDrag (mouseAt (velocity, start.translated (0.0f, -120.0f), start, juce::ModifierKeys::shiftModifier, true));
        CHECK (velocity.getValue() == Catch::Approx (30.0).margin (0.5));
        velocity.mouseUp (mouseAt (velocity, start.translated (0.0f, -120.0f), start, {}, true));
    }
    SECTION ("on Tune a semitone takes 30 points, and with Shift 150")
    {
        auto& tune = knob (panel, "body_tune");
        tune.setValue (33.0, juce::sendNotificationSync);
        drag (tune, 120.0f);
        CHECK (tune.getValue() == Catch::Approx (37.0).margin (0.5));
        tune.setValue (33.0, juce::sendNotificationSync);
        drag (tune, 120.0f, juce::ModifierKeys::shiftModifier);
        CHECK (tune.getValue() == Catch::Approx (34.0).margin (0.5));
    }
    SECTION ("the wheel is 5 times finer with Shift")
    {
        const auto wheel = [&] (float dx, float dy, juce::ModifierKeys mods) {
            velocity.setValue (0.0, juce::sendNotificationSync);
            juce::Thread::sleep (3); // the slider ignores a second event with the same time
            const juce::Point<float> at { 10.0f, 10.0f }; // no button down: the slider ignores the wheel during a drag
            const auto now = juce::Time::getCurrentTime();
            velocity.mouseWheelMove ({ juce::Desktop::getInstance().getMainMouseSource(), at, mods, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, &velocity, &velocity, now, at, now, 0, false },
                                     { dx, dy, false, false, false });
            return velocity.getValue();
        };
        const auto plain = wheel (0.0f, 0.1f, {});
        REQUIRE (plain > 0.5);
        CHECK (wheel (0.0f, 0.1f, juce::ModifierKeys::shiftModifier) == Catch::Approx (plain / 5.0).margin (plain * 0.05));
        CHECK (wheel (-0.1f, 0.0f, juce::ModifierKeys::shiftModifier) == Catch::Approx (plain / 5.0).margin (plain * 0.05));
    }
    SECTION ("double-click and Cmd-click return to the default; the wheel works")
    {
        CHECK (velocity.isScrollWheelEnabled());
        velocity.setValue (10.0, juce::sendNotificationSync);
        velocity.mouseDoubleClick (mouseAt (velocity, { 10.0f, 10.0f }, { 10.0f, 10.0f }, {}, false));
        CHECK (velocity.getValue() == Catch::Approx (100.0));
        velocity.setValue (10.0, juce::sendNotificationSync);
        const juce::ModifierKeys command (juce::ModifierKeys::commandModifier);
        velocity.mouseDown (mouseAt (velocity, { 10.0f, 10.0f }, { 10.0f, 10.0f }, command, false));
        velocity.mouseUp (mouseAt (velocity, { 10.0f, 10.0f }, { 10.0f, 10.0f }, command, false));
        CHECK (velocity.getValue() == Catch::Approx (100.0));
    }
}

TEST_CASE ("fast_05e_a_knob_follows_its_parameters_range")
{
    Panel panel;
    CHECK (knob (panel, "body_pitch_decay").proportionOfLengthToValue (0.5) == Catch::Approx (60.0).margin (0.5));
    CHECK (knob (panel, "body_filter_cutoff").proportionOfLengthToValue (0.5) == Catch::Approx (632.0).margin (2.0));
    CHECK (knob (panel, "body_tune").getInterval() == Catch::Approx (1.0));
}

TEST_CASE ("fast_05f_the_tail_readout_follows_the_knobs")
{
    Panel panel;
    auto* readout = dynamic_cast<juce::Label*> (findById (*panel.editor, "tail_readout"));
    REQUIRE (readout != nullptr);
    auto* timer = dynamic_cast<juce::Timer*> (panel.editor.get());
    REQUIRE (timer != nullptr);
    CHECK (timer->isTimerRunning());
    CHECK (timer->getTimerInterval() <= 100);

    CHECK (readout->getText() == panel.plugin.getTailReadout());
    setParam (panel.plugin, "body_amp_decay", 60.0f);
    timer->timerCallback();
    CHECK (panel.plugin.getTailReadout().contains ("ct")); // the sweep is not done yet at -40 dB
    CHECK (readout->getText() == panel.plugin.getTailReadout());

    // Key Track does not change what it describes.
    const auto before = readout->getText();
    setParam (panel.plugin, "play_key_track", 1.0f);
    timer->timerCallback();
    CHECK (readout->getText() == before);
}

TEST_CASE ("fast_05g_wave_names_its_positions")
{
    Panel panel;
    auto& wave = knob (panel, "body_wave");
    CHECK (wave.getTextFromValue (0.0) == "sine");
    CHECK (wave.getTextFromValue (1.0) == "triangle");
    CHECK (wave.getTextFromValue (2.0) == "saw");
    CHECK (wave.getTextFromValue (3.0) == "square");
    CHECK (wave.getTextFromValue (1.5) == "1.50");
}

TEST_CASE ("fast_05h_every_control_has_a_title_a_screen_reader_can_use")
{
    Panel panel;
    for (auto* p : panel.plugin.getParameters())
    {
        const auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p);
        INFO (ranged->getParameterID());
        auto* control = findById (*panel.editor, ranged->getParameterID());
        REQUIRE (control != nullptr);
        CHECK (control->getTitle() == ranged->getName (64));
    }
}

TEST_CASE ("fast_05i_opening_the_editor_leaves_the_audio_alone")
{
    juce::ScopedJuceInitialiser_GUI gui;
    PluginProcessor plain, shown;
    const std::vector<MidiAt> notes { { 100, juce::MidiMessage::noteOn (1, 36, 1.0f) } };
    const auto a = render (plain, rate48, 0.3, notes);
    const auto b = render (shown, rate48, 0.3, notes, { 256 }, [&, opened = false] (double t) mutable {
        if (t > 0.1 && ! opened)
        {
            opened = true;
            std::unique_ptr<juce::AudioProcessorEditor> editor (shown.createEditor());
        }
    });
    CHECK (a == b);
}

TEST_CASE ("fast_05j_typing_a_value")
{
    Panel panel;
    SECTION ("a note, a wave, a number with its unit")
    {
        CHECK (knob (panel, "body_tune").getValueFromText ("F#0") == Catch::Approx (30.0));
        CHECK (knob (panel, "body_wave").getValueFromText ("saw") == Catch::Approx (2.0));
        CHECK (knob (panel, "body_filter_cutoff").getValueFromText ("1k") == Catch::Approx (1000.0));
        CHECK (knob (panel, "body_filter_cutoff").getValueFromText ("1.20 kHz") == Catch::Approx (1200.0));
        CHECK (knob (panel, "body_amp_decay").getValueFromText ("40 ms") == Catch::Approx (40.0));
        CHECK (knob (panel, "body_amp_decay").getValueFromText ("40") == Catch::Approx (40.0));
    }
    SECTION ("a value shows its unit")
    {
        CHECK (knob (panel, "body_amp_decay").getTextFromValue (400.0) == "400.0 ms");
        CHECK (knob (panel, "body_tune").getTextFromValue (33.0) == "A0");
        CHECK (knob (panel, "body_filter_cutoff").getTextFromValue (20000.0) == "20.0 kHz");
        CHECK (knob (panel, "body_filter_cutoff").getTextFromValue (1200.0) == "1.20 kHz");
        CHECK (knob (panel, "body_filter_cutoff").getTextFromValue (632.0) == "632 Hz");
    }
    SECTION ("the text a knob shows, typed back, stays the same text; every knob lets you type")
    {
        for (auto* p : panel.plugin.getParameters())
        {
            auto* slider = dynamic_cast<juce::Slider*> (findById (*panel.editor, dynamic_cast<juce::RangedAudioParameter*> (p)->getParameterID()));
            if (slider == nullptr)
                continue;
            CHECK (slider->isTextBoxEditable());
            for (const auto at : { 0.0, 0.13, 0.5, 0.77, 1.0 })
            {
                const auto shown = slider->getTextFromValue (slider->proportionOfLengthToValue (at));
                INFO (slider->getComponentID() << " " << shown);
                CHECK (slider->getTextFromValue (slider->getValueFromText (shown)) == shown);
            }
        }
    }
}

TEST_CASE ("fast_05l_one_grid")
{
    Panel panel;
    const auto area = [&] (const juce::String& id) {
        auto* c = findById (*panel.editor, id);
        REQUIRE (c != nullptr);
        return c->getBounds();
    };
    const auto discY = [&] (const juce::String& id) {
        auto& s = knob (panel, id);
        return s.getY() + s.getLookAndFeel().getSliderLayout (s).sliderBounds.getCentreY();
    };
    const auto valueY = [&] (const juce::String& id) {
        auto& s = knob (panel, id);
        return s.getY() + s.getLookAndFeel().getSliderLayout (s).textBoxBounds.getY();
    };

    const std::vector<juce::String> top { "body_pitch_amount", "body_pitch_decay", "body_pitch_shape", "body_tune", "body_wave",
                                          "body_amp_decay", "body_amp_shape", "body_drive", "body_filter_cutoff", "body_filter_res" };
    const std::vector<juce::String> bottom { "play_velocity", "body_sweep2_amount", "body_sweep2_decay", "body_start",
                                             "body_amp_attack", "body_amp_hold", "body_filter_env" };

    SECTION ("each row has one line for discs and one for values")
    {
        for (const auto& row : { top, bottom })
            for (const auto& id : row)
            {
                INFO (id);
                CHECK (discY (id) == discY (row.front()));
                CHECK (valueY (id) == valueY (row.front()));
                CHECK (area (id).getY() == area (row.front()).getY());
            }
        CHECK (discY (top.front()) != discY (bottom.front()));
    }
    SECTION ("switches and the readout stand on the bottom row")
    {
        for (const auto* id : { "play_key_track", "body_tail_anchor" })
            CHECK (area (id).getCentreY() == discY (bottom.front()));
        CHECK (area ("tail_readout").getY() == area (bottom.front()).getY());
        CHECK (area ("tail_readout").getBottom() == area (bottom.front()).getBottom());
    }
    SECTION ("a bottom control stands under the top knob of its column")
    {
        const std::pair<const char*, const char*> columns[] = {
            { "body_sweep2_amount", "body_pitch_amount" }, { "body_sweep2_decay", "body_pitch_decay" }, { "body_tail_anchor", "body_pitch_shape" },
            { "body_start", "body_tune" }, { "tail_readout", "body_wave" }, { "body_amp_attack", "body_amp_decay" },
            { "body_amp_hold", "body_amp_shape" }, { "body_filter_env", "body_filter_cutoff" }
        };
        for (const auto& [below, above] : columns)
        {
            INFO (below);
            CHECK (area (below).getCentreX() == area (above).getCentreX());
        }
    }
    SECTION ("columns in a section are 88 points apart")
    {
        const std::pair<const char*, const char*> neighbours[] = {
            { "body_pitch_amount", "body_pitch_decay" }, { "body_pitch_decay", "body_pitch_shape" }, { "body_tune", "body_wave" },
            { "body_amp_decay", "body_amp_shape" }, { "body_filter_cutoff", "body_filter_res" }
        };
        for (const auto& [left, right] : neighbours)
        {
            INFO (left);
            CHECK (area (right).getCentreX() - area (left).getCentreX() == 88);
        }
    }
    SECTION ("the pad and the Loop switch line up with the row below")
    {
        CHECK (area ("audition_pad").getX() == area ("audition_loop").getX());
        CHECK (area ("audition_pad").getRight() == area ("audition_loop").getRight());
        CHECK (area ("audition_loop").getRight() == area ("play_key_track").getRight());
        CHECK (area ("audition_loop").getBottom() == area (top.front()).getBottom());
    }
}

TEST_CASE ("fast_05m_printed_lines_set_the_sections_apart")
{
    Panel panel;
    const auto image = panel.editor->createComponentSnapshot (panel.editor->getLocalBounds());
    const auto print = juce::Colour (panel::print);
    REQUIRE (std::size (panel::sectionLines) == sections.size() - 1);

    size_t gap = 0;
    for (const auto x : panel::sectionLines)
    {
        INFO (x);
        // It runs from just below the signal line to the bottom row's value line, 1 point wide.
        for (const auto y : { 45, 100, 200, 300 })
            CHECK (image.getPixelAt (x, y) == print);
        CHECK (image.getPixelAt (x - 1, 100) == juce::Colour (panel::plate));
        CHECK (image.getPixelAt (x + 1, 100) == juce::Colour (panel::plate));
        CHECK (image.getPixelAt (x, 310) == juce::Colour (panel::plate));

        // It lies between its two sections and through no control.
        for (const auto& section : sections)
            for (const auto& id : section)
            {
                const auto bounds = findById (*panel.editor, id)->getBounds();
                CHECK (! (bounds.getX() <= x && x < bounds.getRight()));
            }
        for (const auto& id : sections[gap])
            CHECK (findById (*panel.editor, id)->getBounds().getCentreX() < x);
        for (const auto& id : sections[gap + 1])
            CHECK (findById (*panel.editor, id)->getBounds().getCentreX() > x);
        ++gap;
    }
}

TEST_CASE ("fast_05k_text_can_be_read")
{
    const auto contrast = [] (std::uint32_t a, std::uint32_t b) {
        const auto la = luminance (juce::Colour (a)), lb = luminance (juce::Colour (b));
        return (juce::jmax (la, lb) + 0.05) / (juce::jmin (la, lb) + 0.05);
    };
    CHECK (contrast (panel::print, panel::plate) >= 4.5);
    CHECK (contrast (panel::ink, panel::plate) >= 4.5);
    CHECK (contrast (panel::ink, panel::sounding) >= 4.5);

    Panel panel;
    forEachComponent (*panel.editor, [] (juce::Component& c) {
        if (auto* label = dynamic_cast<juce::Label*> (&c))
        {
            INFO (label->getText());
            CHECK (label->getFont().getHeight() >= 11.0f);
        }
    });
    for (auto* p : panel.plugin.getParameters())
        if (auto* slider = dynamic_cast<juce::Slider*> (findById (*panel.editor, dynamic_cast<juce::RangedAudioParameter*> (p)->getParameterID())))
            for (auto* child : slider->getChildren())
                if (auto* value = dynamic_cast<juce::Label*> (child))
                    CHECK (value->getFont().getHeight() >= 13.0f);
}

// --- FAST-01 Audition ---

TEST_CASE ("fast_01a_a_press_is_a_note_at_full_level")
{
    for (const auto velocityKnob : { 100.0f, 30.0f })
    {
        PluginProcessor midi, pad;
        setParam (midi, "play_velocity", velocityKnob);
        setParam (pad, "play_velocity", velocityKnob);
        const auto sample = 256 * 4;
        const auto a = render (midi, rate48, 0.5, { { sample, juce::MidiMessage::noteOn (1, 36, (juce::uint8) 127) } });
        const auto b = render (pad, rate48, 0.5, {}, { 256 }, [&, pressed = false] (double t) mutable {
            if (! pressed && t >= sample / rate48)
            {
                pressed = true;
                pad.pressPad();
            }
        });
        CHECK (measure::peak (b) > 0.5f);
        CHECK (a == b);
    }
}

TEST_CASE ("fast_01b_the_pad_plays_tune_with_key_track_on")
{
    PluginProcessor off, on;
    setParam (off, "body_tune", 40.0f);
    setParam (on, "body_tune", 40.0f);
    setParam (on, "play_key_track", 1.0f);
    off.pressPad();
    on.pressPad();
    const auto pressedOff = render (off, rate48, 0.5);
    CHECK (measure::peak (pressedOff) > 0.5f);
    CHECK (pressedOff == render (on, rate48, 0.5));

    // And after a keyed note, too: the note it played does not stick.
    PluginProcessor after;
    setParam (after, "body_tune", 40.0f);
    setParam (after, "play_key_track", 1.0f);
    const auto press = 96000;
    const auto a = render (after, rate48, 2.5, { { 0, juce::MidiMessage::noteOn (1, 60, 1.0f) } }, { 256 }, [&, pressed = false] (double t) mutable {
        if (! pressed && t >= press / rate48)
        {
            pressed = true;
            after.pressPad();
        }
    });
    PluginProcessor fresh;
    setParam (fresh, "body_tune", 40.0f);
    fresh.pressPad();
    const auto b = render (fresh, rate48, 0.5);
    CHECK (measure::largestDifference (a, static_cast<size_t> (press), b, 0, b.size()) < 0.001f);
}

TEST_CASE ("fast_01c_a_press_during_a_ringing_kick_does_not_click")
{
    for (int ms = 50; ms <= 310; ms += 20)
    {
        PluginProcessor plugin;
        plugin.pressPad();
        const auto gap = ms * 0.001;
        const auto x = render (plugin, rate48, gap + 0.05, {}, { 64 }, [&, pressed = false] (double t) mutable {
            if (! pressed && t >= gap)
            {
                pressed = true;
                plugin.pressPad();
            }
        });
        INFO (ms);
        CHECK (measure::peakBetween (x, rate48, gap, gap + 0.01) > 0.5f);
        CHECK (measure::largestStepBetween (x, rate48, gap - 0.005, gap + 0.05) < 1.15f);
    }
}

TEST_CASE ("fast_01d_transport_hits_lie_on_the_grid")
{
    for (const auto rate : { 44100.0, 48000.0 })
        for (const auto bpm : { 120.0, 137.0, 174.0 })
        {
            const auto seconds = 15.5 * 60.0 / bpm;
            const auto reference = loopAlongTransport (rate, bpm, 0.0, seconds);
            INFO (rate << " " << bpm);
            checkOnGrid (reference, rate, bpm, 0.0, 16);
            CHECK (loopAlongTransport (rate, bpm, 0.0, seconds, { 64 }) == reference);
            CHECK (loopAlongTransport (rate, bpm, 0.0, seconds, { 1, 37, 512 }) == reference);
        }
}

TEST_CASE ("fast_01e_one_hit_per_quarter_note")
{
    // 120 BPM at 48 kHz: a beat is 24000 samples.
    SECTION ("on a block's first sample")
    {
        checkOnGrid (loopAlongTransport (rate48, 120.0, 0.0, 1.9, { 24000 }), rate48, 120.0, 0.0, 4);
    }
    SECTION ("on a block's last sample")
    {
        checkOnGrid (loopAlongTransport (rate48, 120.0, 1.0 - 23999.0 / 24000.0, 1.9, { 24000 }), rate48, 120.0, 23999.0, 3);
    }
    SECTION ("between two blocks")
    {
        // Blocks of 16000 and a first beat at 7999.5: beats 1 and 3 land half a sample before a block's end.
        checkOnGrid (loopAlongTransport (rate48, 120.0, 1.0 - 31999.5 / 24000.0, 1.9, { 16000 }), rate48, 120.0, 7999.5, 4);
        checkOnGrid (loopAlongTransport (rate48, 120.0, 1.0 - 255.5 / 24000.0, 1.9, { 256 }), rate48, 120.0, 255.5, 4);
    }
    SECTION ("with rounding noise")
    {
        checkOnGrid (loopAlongTransport (rate48, 120.0, 0.0, 1.9, { 24000 }, 1.0e-9), rate48, 120.0, 0.0, 4);
        checkOnGrid (loopAlongTransport (rate48, 120.0, 0.0, 1.9, { 1000 }, 1.0e-9), rate48, 120.0, 0.0, 4);
    }
}

TEST_CASE ("fast_01f_the_grid_follows_the_host")
{
    SECTION ("a tempo change takes effect in the block it arrives in")
    {
        ShortKick plugin;
        FakeHead head;
        plugin.setPlayHead (&head);
        plugin.setLoop (true);
        const auto x = render (plugin, rate48, 2.0, {}, { 256 }, [&] (double t) {
            // 120 BPM up to beat 2, at 1 s; 180 BPM after it.
            head.bpm = t < 1.0 ? 120.0 : 180.0;
            head.ppq = t < 1.0 ? t * 2.0 : 2.0 + (t - 1.0) * 3.0;
        });
        const auto hits = hitsIn (x, rate48);
        REQUIRE (hits.size() >= 5);
        CHECK (std::abs (hits[1] - 24000) <= 1);
        CHECK (std::abs (hits[2] - 48000) <= 1);
        CHECK (std::abs (hits[3] - 64000) <= 1);
        CHECK (std::abs (hits[4] - 80000) <= 1);
    }
    SECTION ("a loop back plays the beat again")
    {
        ShortKick plugin;
        FakeHead head;
        plugin.setPlayHead (&head);
        plugin.setLoop (true);
        // A loop of one beat, from beat 1 to beat 2.
        const auto x = render (plugin, rate48, 1.9, {}, { 1000 }, [&] (double t) { head.ppq = 1.0 + std::fmod (t * 2.0, 1.0); });
        checkOnGrid (hitsIn (x, rate48), rate48, 120.0, 0.0, 4);
    }
}

TEST_CASE ("fast_01g_stopped_the_loop_runs_free_at_the_hosts_tempo")
{
    const auto beat = 60.0 / 137.0;
    SECTION ("the interval")
    {
        ShortKick plugin;
        FakeHead head;
        head.bpm = 137.0;
        head.playing = false;
        head.ppq = 8.25;
        plugin.setPlayHead (&head);
        plugin.setLoop (true);
        checkOnGrid (hitsIn (render (plugin, rate48, 15.5 * beat, {}, { 1, 37, 512 }), rate48), rate48, 137.0, 0.0, 16);
    }
    SECTION ("starting the transport moves the hits to the grid")
    {
        ShortKick plugin;
        FakeHead head;
        head.bpm = 120.0;
        head.playing = false;
        plugin.setPlayHead (&head);
        plugin.setLoop (true);
        // Stopped for 0.8 s, then rolling from beat 10.25: the next beat is 0.375 s later, at 1.175 s.
        const auto x = render (plugin, rate48, 2.0, {}, { 100 }, [&] (double t) {
            head.playing = t >= 0.8;
            head.ppq = head.playing ? 10.25 + (t - 0.8) * 2.0 : 0.0;
        });
        const auto hits = hitsIn (x, rate48);
        REQUIRE (hits.size() == 4);
        CHECK (hits[0] == 0);
        CHECK (hits[1] == 24000);
        CHECK (std::abs (hits[2] - 1.175 * rate48) <= 1.0);
        CHECK (std::abs (hits[3] - 1.675 * rate48) <= 1.0);
    }
    SECTION ("stopping it keeps the interval from the last hit")
    {
        ShortKick plugin;
        FakeHead head;
        plugin.setPlayHead (&head);
        plugin.setLoop (true);
        const auto x = render (plugin, rate48, 1.6, {}, { 100 }, [&] (double t) {
            head.playing = t < 0.6;
            head.ppq = t * 2.0;
        });
        const auto hits = hitsIn (x, rate48);
        // Beats 0 and 1 on the grid, then free at 0.5 s from beat 1.
        REQUIRE (hits.size() == 4);
        CHECK (std::abs (hits[2] - 48000) <= 1);
        CHECK (std::abs (hits[3] - 72000) <= 1);
    }
}

TEST_CASE ("fast_01h_without_a_usable_tempo_the_loop_runs_at_120")
{
    FakeHead noTempo, zero, noPosition;
    noTempo.bpm.reset();
    noTempo.playing = false;
    zero.bpm = 0.0;
    zero.playing = false;
    noPosition.hasPosition = false;

    for (auto* head : { (juce::AudioPlayHead*) nullptr, (juce::AudioPlayHead*) &noTempo, (juce::AudioPlayHead*) &zero, (juce::AudioPlayHead*) &noPosition })
    {
        ShortKick plugin;
        plugin.setPlayHead (head);
        plugin.setLoop (true);
        checkOnGrid (hitsIn (render (plugin, rate48, 1.9), rate48), rate48, 120.0, 0.0, 4);
    }
    SECTION ("a position that is not a number does not hang")
    {
        ShortKick plugin;
        FakeHead head;
        head.ppq = std::numeric_limits<double>::quiet_NaN();
        plugin.setPlayHead (&head);
        plugin.setLoop (true);
        checkOnGrid (hitsIn (render (plugin, rate48, 1.9), rate48), rate48, 120.0, 0.0, 4);
    }
    SECTION ("a running transport with a tempo of 0 does not divide by zero")
    {
        ShortKick plugin;
        FakeHead head;
        head.bpm = 0.0;
        head.ppq = 4.5;
        plugin.setPlayHead (&head);
        plugin.setLoop (true);
        CHECK (measure::peak (render (plugin, rate48, 0.5)) <= 1.01f);
    }
}

TEST_CASE ("fast_01i_switching_the_loop")
{
    SECTION ("stopped: a hit in the next block")
    {
        ShortKick plugin;
        FakeHead head;
        head.playing = false;
        plugin.setPlayHead (&head);
        const auto x = render (plugin, rate48, 0.1, {}, { 256 }, [&, on = false] (double t) mutable {
            if (! on && t >= 0.05)
            {
                on = true;
                plugin.setLoop (true);
            }
        });
        const auto hits = hitsIn (x, rate48);
        REQUIRE (hits.size() == 1);
        CHECK (hits[0] >= 0.05 * rate48);
        CHECK (hits[0] <= 0.05 * rate48 + 256);
    }
    SECTION ("running: the first hit is the next quarter note")
    {
        const auto hits = loopAlongTransport (rate48, 120.0, 0.5, 0.4);
        REQUIRE (hits.size() == 1);
        CHECK (std::abs (hits[0] - 12000) <= 1);
    }
    SECTION ("off: no new hit, and the kick that rings plays out")
    {
        FakeHead head;
        head.playing = false;
        PluginProcessor looped, pressed;
        looped.setPlayHead (&head);
        looped.setLoop (true);
        const auto a = render (looped, rate48, 2.0, {}, { 256 }, [&] (double t) {
            if (t >= 0.01)
                looped.setLoop (false);
        });
        pressed.pressPad();
        CHECK (a == render (pressed, rate48, 2.0));
    }
}

TEST_CASE ("fast_01j_the_loop_stops_when_the_editor_closes")
{
    Panel panel;
    auto* loop = dynamic_cast<juce::Button*> (findById (*panel.editor, "audition_loop"));
    REQUIRE (loop != nullptr);
    loop->setToggleState (true, juce::sendNotificationSync);
    CHECK (panel.plugin.isLoopOn());
    panel.editor = nullptr;
    CHECK (! panel.plugin.isLoopOn());

    panel.editor.reset (panel.plugin.createEditor());
    loop = dynamic_cast<juce::Button*> (findById (*panel.editor, "audition_loop"));
    REQUIRE (loop != nullptr);
    CHECK (! loop->getToggleState());
}

TEST_CASE ("fast_01k_a_midi_note_does_not_move_the_loop")
{
    ShortKick plugin;
    FakeHead head;
    plugin.setPlayHead (&head);
    plugin.setLoop (true);
    const auto x = render (plugin, rate48, 1.2, { { 12000, juce::MidiMessage::noteOn (1, 36, 1.0f) } }, { 256 },
                           [&] (double t) { head.ppq = t * 2.0; });
    const auto hits = hitsIn (x, rate48);
    REQUIRE (hits.size() == 4);
    CHECK (hits[0] == 0);
    CHECK (hits[1] == 12000);
    CHECK (std::abs (hits[2] - 24000) <= 1);
    CHECK (std::abs (hits[3] - 48000) <= 1);
}

TEST_CASE ("fast_01l_pad_and_loop_stay_out_of_the_saved_state")
{
    PluginProcessor plugin;
    juce::MemoryBlock off, on;
    plugin.getStateInformation (off);
    plugin.setLoop (true);
    plugin.pressPad();
    plugin.getStateInformation (on);
    CHECK (off == on);
    plugin.setStateInformation (off.getData(), static_cast<int> (off.getSize()));
    CHECK (plugin.isLoopOn());
    plugin.setLoop (false);
    plugin.setStateInformation (on.getData(), static_cast<int> (on.getSize()));
    CHECK (! plugin.isLoopOn());
    CHECK (std::none_of (plugin.getParameters().begin(), plugin.getParameters().end(), [] (auto* p) {
        return dynamic_cast<juce::RangedAudioParameter*> (p)->getParameterID().startsWith ("audition");
    }));
}

TEST_CASE ("fast_01m_silent_in_an_offline_render")
{
    PluginProcessor plugin;
    plugin.setNonRealtime (true);
    plugin.pressPad();
    plugin.setLoop (true);
    CHECK (measure::peak (render (plugin, rate48, 0.5)) < 1.0e-9f);

    // MIDI still plays.
    CHECK (measure::peak (render (plugin, rate48, 0.1, { { 0, juce::MidiMessage::noteOn (1, 36, 1.0f) } })) > 0.5f);
}

TEST_CASE ("fast_01n_the_panel_can_see_a_kick")
{
    ShortKick plugin;
    FakeHead head;
    head.playing = false;
    plugin.setPlayHead (&head);
    CHECK (plugin.getKickCount() == 0);
    plugin.pressPad();
    render (plugin, rate48, 0.01);
    CHECK (plugin.getKickCount() == 1);
    CHECK (plugin.getLoopHitCount() == 0);
    render (plugin, rate48, 0.01, { { 0, juce::MidiMessage::noteOn (1, 36, 1.0f) } });
    CHECK (plugin.getKickCount() == 2);
    plugin.setLoop (true);
    render (plugin, rate48, 0.01);
    CHECK (plugin.getKickCount() == 3);
    CHECK (plugin.getLoopHitCount() == 1);
}

// --- Engineering ---

TEST_CASE ("eng_01d_the_audition_does_not_allocate")
{
    for (const auto playing : { false, true })
    {
        PluginProcessor plugin;
        FakeHead head;
        head.playing = playing;
        plugin.setPlayHead (&head);
        plugin.prepareToPlay (rate48, 512);
        juce::AudioBuffer<float> buffer (2, 512);
        juce::MidiBuffer midi;
        plugin.setLoop (true);
        for (int block = 0; block < 400; ++block)
        {
            head.ppq = block * 512.0 / rate48 * 2.0;
            if (block % 7 == 0)
                plugin.pressPad();
            AllocationGuard guard;
            plugin.processBlock (buffer, midi);
            CHECK (guard.count() == 0);
        }
    }
}

TEST_CASE ("eng_03_the_same_hits_render_identically")
{
    CHECK (loopAlongTransport (48000.0, 137.0, 0.0, 3.0) == loopAlongTransport (48000.0, 137.0, 0.0, 3.0));
    const auto play = [] {
        PluginProcessor plugin;
        FakeHead head;
        head.playing = false;
        plugin.setPlayHead (&head);
        plugin.setLoop (true);
        return render (plugin, rate48, 2.0);
    };
    CHECK (play() == play());
}
