#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>
#include <vector>

// Plays one note through a plugin offline, block by block, as a host would, and returns the left
// channel. Block sizes cycle through `blockSizes`, so tests also cover uneven host blocks.
// `beforeBlock`, if given, runs before every block with the seconds rendered so far: a knob move.
inline std::vector<float> renderNote (juce::AudioProcessor& plugin, double sampleRate, double seconds,
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
    std::vector<float> out;
    out.reserve (static_cast<size_t> (total));
    juce::AudioBuffer<float> buffer (2, largest);
    juce::MidiBuffer midi;

    for (size_t block = 0; static_cast<int> (out.size()) < total; ++block)
    {
        const auto done = static_cast<int> (out.size());
        const auto size = juce::jmin (blockSizes[block % blockSizes.size()], total - done);
        if (beforeBlock)
            beforeBlock (done / sampleRate);
        midi.clear();
        if (noteOnSample >= done && noteOnSample < done + size)
            midi.addEvent (juce::MidiMessage::noteOn (1, note, velocity), noteOnSample - done);

        buffer.setSize (2, size, false, false, true);
        plugin.processBlock (buffer, midi);
        out.insert (out.end(), buffer.getReadPointer (0), buffer.getReadPointer (0) + size);
    }
    return out;
}
