#pragma once

#include <cmath>
#include <numbers>

namespace dsp
{
// The fixed test kick of BUILD-06: proves the MIDI-to-audio path until BODY replaces it.
class TestVoice
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        active = false;
    }

    // ponytail: a hard restart clicks when a kick is still ringing; PLAY-03 replaces this.
    void trigger (float newLevel)
    {
        level = newLevel;
        phase = 0.0;
        time = 0.0;
        active = true;
    }

    bool isActive() const { return active; }

    float next()
    {
        if (! active)
            return 0.0f;

        const auto freq = tailHz + sweepHz * std::exp (-time / sweepSeconds);
        const auto amp = std::exp (-time / decaySeconds);
        const auto out = std::sin (phase) * amp * level;

        phase = std::fmod (phase + 2.0 * std::numbers::pi * freq / sampleRate, 2.0 * std::numbers::pi);
        time += 1.0 / sampleRate;
        active = amp > 1.0e-4;
        return static_cast<float> (out);
    }

private:
    static constexpr double tailHz = 50.0, sweepHz = 200.0, sweepSeconds = 0.02, decaySeconds = 0.15;

    double sampleRate = 44100.0, phase = 0.0, time = 0.0;
    float level = 0.0f;
    bool active = false;
};
}
