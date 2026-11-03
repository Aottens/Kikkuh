#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>
#include <vector>

// Plays one note through a plugin offline, block by block, as a host would, and returns both
// channels. kikkuh-render (HARN-04) uses this too, so the tests measure what it writes. Block sizes cycle through `blockSizes`, so tests also cover uneven host blocks.
// `beforeBlock`, if given, runs before every block with the seconds rendered so far: a knob move.
inline juce::AudioBuffer<float> renderNoteStereo (juce::AudioProcessor& plugin, double sampleRate, double seconds,
                                      int noteOnSample = 0, int note = 36, float velocity = 1.0f,
                                      std::vector<int> blockSizes = { 256 },
                                      const std::function<void (double)>& beforeBlock = {})
{
    int largest = 1;
    for (auto size : blockSizes)
        largest = juce::jmax (largest, size);
    plugin.setPlayConfigDetails (0, 2, sampleRate, largest);
    plugin.prepareToPlay (sampleRate, largest);

    const auto total = static_cast<int> (seconds * sampleRate);
    juce::AudioBuffer<float> out (2, total);
    juce::AudioBuffer<float> buffer (2, largest);
    juce::MidiBuffer midi;

    int done = 0;
    for (size_t block = 0; done < total; ++block)
    {
        const auto size = juce::jmin (blockSizes[block % blockSizes.size()], total - done);
        if (beforeBlock)
            beforeBlock (done / sampleRate);
        midi.clear();
        if (noteOnSample >= done && noteOnSample < done + size)
            midi.addEvent (juce::MidiMessage::noteOn (1, note, velocity), noteOnSample - done);

        buffer.setSize (2, size, false, false, true);
        plugin.processBlock (buffer, midi);
        for (int ch = 0; ch < 2; ++ch)
            out.copyFrom (ch, done, buffer, ch, 0, size);
        done += size;
    }
    return out;
}

// The left channel of renderNoteStereo.
inline std::vector<float> renderNote (juce::AudioProcessor& plugin, double sampleRate, double seconds,
                                      int noteOnSample = 0, int note = 36, float velocity = 1.0f,
                                      std::vector<int> blockSizes = { 256 },
                                      const std::function<void (double)>& beforeBlock = {})
{
    const auto out = renderNoteStereo (plugin, sampleRate, seconds, noteOnSample, note, velocity, std::move (blockSizes), beforeBlock);
    return { out.getReadPointer (0), out.getReadPointer (0) + out.getNumSamples() };
}
