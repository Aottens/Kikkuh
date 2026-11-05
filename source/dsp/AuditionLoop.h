#pragma once

#include <algorithm>
#include <cmath>

namespace dsp
{
// FAST-01: the audition loop, one hit per quarter note. Plain C++, no allocation.
// Per block: begin(), then next() until it returns -1; each call gives the sample offset of one hit.
class AuditionLoop
{
public:
    // The next switch-on starts fresh (prepareToPlay).
    void reset() { mode = Mode::off; }

    // `rolling`: the host's transport runs and gives a PPQ and a usable tempo. Then every hit comes from `ppq`;
    // otherwise the loop runs free at `bpm` (120 when it is below 1).
    void begin (bool on, int blockSamples, double rate, bool rolling, double ppq, double bpm)
    {
        // Samples since the last hit, as seen from the start of this block (negative: the hit is still to come).
        sinceHit += previous;
        remaining -= previous;
        previous = blockSamples;
        samples = blockSamples;
        perBeat = std::max (1.0, rate * 60.0 / (bpm >= 1.0 ? std::min (bpm, 999.0) : 120.0));
        blockPpq = ppq;

        if (! on)
        {
            mode = Mode::off;
            havePpq = haveBeat = false;
            sinceHit = 1.0e12;
            return;
        }

        const auto was = mode;
        mode = rolling ? Mode::rolling : Mode::free;
        if (mode == Mode::free)
        {
            havePpq = haveBeat = false;
            if (was == Mode::off)
                remaining = 0.0;
            else if (was == Mode::rolling)
                remaining = std::max (0.0, perBeat - sinceHit);
            return;
        }

        // FAST-01f: a block that does not continue the last one (loop, jump) forgets the beat it played.
        if (! havePpq || std::abs (ppq - expectedPpq) > 1.0e-3)
            haveBeat = false;
        havePpq = true;
        expectedPpq = ppq + samples / perBeat;

        // Half a sample of slack, so a beat that rounds to this block's first sample belongs to it.
        beat = std::ceil (ppq - 0.5 / perBeat - 1.0e-6);
        if (haveBeat && beat <= lastBeat)
            beat = lastBeat + 1.0;
    }

    // The offset of the next hit in this block, or -1.
    int next()
    {
        if (mode == Mode::off)
            return -1;

        int offset;
        if (mode == Mode::rolling)
        {
            offset = std::max (0, static_cast<int> (std::lround ((beat - blockPpq) * perBeat)));
            if (offset >= samples)
                return -1;
            lastBeat = beat++;
            haveBeat = true;
        }
        else
        {
            if (remaining >= samples)
                return -1;
            offset = static_cast<int> (remaining);
            remaining += perBeat;
        }
        sinceHit = -offset;
        return offset;
    }

private:
    enum class Mode { off, free, rolling };

    Mode mode = Mode::off;
    int samples = 0, previous = 0;
    double perBeat = 24000.0, blockPpq = 0.0, expectedPpq = 0.0, beat = 0.0, lastBeat = 0.0;
    double remaining = 0.0;      // free: samples to the next hit, from this block's start
    double sinceHit = 1.0e12;    // samples since the last hit, from this block's start
    bool havePpq = false, haveBeat = false;
};
} // namespace dsp
