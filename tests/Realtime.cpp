#include "AllocationGuard.h"
#include <PluginProcessor.h>
#include <catch2/catch_test_macros.hpp>
#include <atomic>
#include <cmath>
#include <thread>

// Escapes through a volatile pointer, so the compiler cannot elide the new/delete pair.
static void allocateOne()
{
    static int* volatile sink = nullptr;
    sink = new int (1);
    delete sink;
}

TEST_CASE ("eng_01_allocation_guard_counts_only_armed_thread")
{
    {
        AllocationGuard guard;
        allocateOne();
        CHECK (guard.count() == 1);
    }

    allocateOne(); // disarmed: must not count anywhere
    std::atomic<bool> go { false };
    std::thread other ([&] { while (! go) {} allocateOne(); }); // created before the guard: std::thread allocates
    AllocationGuard guard;
    go = true;
    other.join();
    CHECK (guard.count() == 0);
}

TEST_CASE ("eng_01_process_block_does_not_allocate")
{
    PluginProcessor plugin;
    plugin.prepareToPlay (48000.0, 512);
    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;

    int allocations = 0;
    for (int block = 0; block < 100; ++block)
    {
        const int sizes[] = { 256, 1, 37 };
        buffer.setSize (2, sizes[block % 3], false, false, true);
        midi.clear();
        if (block % 4 == 0)
            midi.addEvent (juce::MidiMessage::noteOn (1, 36, 1.0f), 0);

        AllocationGuard guard;
        plugin.processBlock (buffer, midi);
        allocations += guard.count();
    }
    CHECK (allocations == 0);
}

TEST_CASE ("eng_01_process_block_flushes_denormals_and_restores_mode")
{
    PluginProcessor plugin;
    plugin.prepareToPlay (48000.0, 4800);
    juce::AudioBuffer<float> buffer (2, 4800);
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 36, 1.0f / 127.0f), 0);

    const auto before = juce::FloatVectorOperations::areDenormalsDisabled();
    plugin.processBlock (buffer, midi);
    CHECK (juce::FloatVectorOperations::areDenormalsDisabled() == before);

    int subnormal = 0;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 4800; ++i)
            subnormal += std::fpclassify (buffer.getSample (ch, i)) == FP_SUBNORMAL ? 1 : 0;
    CHECK (subnormal == 0);
}
