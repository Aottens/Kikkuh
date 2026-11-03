#include "Measure.h"
#include "Render.h"
#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE ("build_05_identity")
{
    PluginProcessor plugin;
    CHECK (plugin.getName() == "Kikkuh");
    CHECK (juce::String (JucePlugin_Manufacturer) == "Colourmotion");
    CHECK (plugin.acceptsMidi());
    CHECK (plugin.getTotalNumInputChannels() == 0);
    CHECK (plugin.getTotalNumOutputChannels() == 2);
}

TEST_CASE ("build_06_note_on_sounds_on_its_exact_sample")
{
    for (auto sizes : { std::vector<int> { 256 }, std::vector<int> { 64 }, std::vector<int> { 1, 37, 512 } })
    {
        PluginProcessor plugin;
        const auto audio = renderNote (plugin, 48000.0, 0.1, 1000, 36, 1.0f, sizes);
        CHECK (measure::firstSound (audio) == 1001); // sample 1000 is sin (0)
        CHECK (measure::peak (audio) > 0.5f);
    }
}

TEST_CASE ("build_06_silent_without_a_note")
{
    PluginProcessor plugin;
    juce::AudioBuffer<float> buffer (2, 256);
    buffer.clear();
    plugin.prepareToPlay (48000.0, 256);
    juce::MidiBuffer midi;
    plugin.processBlock (buffer, midi);
    CHECK (buffer.getMagnitude (0, 256) == 0.0f);
}

TEST_CASE ("eng_02_reports_no_latency")
{
    PluginProcessor plugin;
    plugin.prepareToPlay (48000.0, 256);
    CHECK (plugin.getLatencySamples() == 0);
}

TEST_CASE ("eng_03_same_note_renders_identically")
{
    PluginProcessor a, b;
    CHECK (renderNote (a, 48000.0, 0.5) == renderNote (b, 48000.0, 0.5, 0, 36, 1.0f, { 1, 37, 512 }));
}

TEST_CASE ("eng_04_sounds_the_same_at_every_sample_rate")
{
    PluginProcessor reference;
    const auto base = renderNote (reference, 44100.0, 0.5);
    const auto baseFreq = measure::frequency (base, 44100.0, 0.2, 0.4);
    REQUIRE (baseFreq > 0.0);

    for (auto rate : { 48000.0, 96000.0, 192000.0 })
    {
        PluginProcessor plugin;
        const auto audio = renderNote (plugin, rate, 0.5);
        CHECK (measure::frequency (audio, rate, 0.2, 0.4) == Catch::Approx (baseFreq).epsilon (0.01));
        CHECK (measure::peak (audio) == Catch::Approx (measure::peak (base)).epsilon (0.01));
    }
}

TEST_CASE ("eng_06_state_carries_a_version")
{
    PluginProcessor plugin;
    juce::MemoryBlock state;
    plugin.getStateInformation (state);
    const auto xml = juce::AudioProcessor::getXmlFromBinary (state.getData(), static_cast<int> (state.getSize()));
    REQUIRE (xml != nullptr);
    CHECK (xml->getIntAttribute ("version") >= 1);
    plugin.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
}
