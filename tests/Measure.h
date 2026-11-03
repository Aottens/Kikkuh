#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <vector>

// Plain measurements on rendered audio. Claude cannot hear: every claim about how a kick behaves
// is one of these numbers in a test, and only taste is left to the owner's ears.
namespace measure
{
inline float peak (const std::vector<float>& x)
{
    float p = 0.0f;
    for (auto v : x)
        p = std::max (p, std::abs (v));
    return p;
}

inline int firstSound (const std::vector<float>& x, float threshold = 1.0e-6f)
{
    for (size_t i = 0; i < x.size(); ++i)
        if (std::abs (x[i]) > threshold)
            return static_cast<int> (i);
    return -1;
}

// The largest step between neighbouring samples: a click shows up here.
inline float largestStep (const std::vector<float>& x)
{
    float step = 0.0f;
    for (size_t i = 1; i < x.size(); ++i)
        step = std::max (step, std::abs (x[i] - x[i - 1]));
    return step;
}

// Frequency between two times, from interpolated rising zero crossings. For a tail, not a sweep.
inline double frequency (const std::vector<float>& x, double sampleRate, double fromSeconds, double toSeconds)
{
    const auto from = static_cast<size_t> (fromSeconds * sampleRate);
    const auto to = std::min (static_cast<size_t> (toSeconds * sampleRate), x.size());
    double first = -1.0, last = -1.0;
    int crossings = 0;
    for (size_t i = from + 1; i < to; ++i)
        if (x[i - 1] < 0.0f && x[i] >= 0.0f)
        {
            last = static_cast<double> (i - 1) + x[i - 1] / (x[i - 1] - x[i]);
            if (crossings++ == 0)
                first = last;
        }
    return crossings < 2 ? 0.0 : (crossings - 1) * sampleRate / (last - first);
}

inline float peakBetween (const std::vector<float>& x, double sampleRate, double fromSeconds, double toSeconds)
{
    const auto from = std::min (static_cast<size_t> (fromSeconds * sampleRate), x.size());
    const auto to = std::min (static_cast<size_t> (toSeconds * sampleRate), x.size());
    return peak (std::vector<float> (x.begin() + static_cast<std::ptrdiff_t> (from), x.begin() + static_cast<std::ptrdiff_t> (to)));
}

inline float largestStepBetween (const std::vector<float>& x, double sampleRate, double fromSeconds, double toSeconds)
{
    const auto from = std::min (static_cast<size_t> (fromSeconds * sampleRate), x.size());
    const auto to = std::min (static_cast<size_t> (toSeconds * sampleRate), x.size());
    return largestStep (std::vector<float> (x.begin() + static_cast<std::ptrdiff_t> (from), x.begin() + static_cast<std::ptrdiff_t> (to)));
}

// The index of the last sample that is not exactly zero, or -1.
inline int lastSound (const std::vector<float>& x)
{
    for (size_t i = x.size(); i > 0; --i)
        if (x[i - 1] != 0.0f)
            return static_cast<int> (i - 1);
    return -1;
}

// Blackman-Harris weighted power at one frequency (Goertzel), in amplitude-squared units of a sine.
// The window's sidelobes stay below -92 dB; its main lobe is 4 bins to each side.
inline double tonePower (const float* x, size_t size, double sampleRate, double hz)
{
    const auto twoPi = 2.0 * std::numbers::pi;
    const auto coefficient = 2.0 * std::cos (twoPi * hz / sampleRate);
    double s1 = 0.0, s2 = 0.0, windowSum = 0.0;
    for (size_t i = 0; i < size; ++i)
    {
        const auto a = twoPi * static_cast<double> (i) / static_cast<double> (size - 1);
        const auto w = 0.35875 - 0.48829 * std::cos (a) + 0.14128 * std::cos (2.0 * a) - 0.01168 * std::cos (3.0 * a);
        const auto s = w * x[i] + coefficient * s1 - s2;
        s2 = s1;
        s1 = s;
        windowSum += w;
    }
    const auto scale = 2.0 / windowSum;
    return (s1 * s1 + s2 * s2 - coefficient * s1 * s2) * scale * scale;
}

// The amplitude of a sine at `hz` between two times. Needs at least 8 / hz' of signal to separate
// neighbours spaced hz' apart.
inline double toneLevel (const std::vector<float>& x, double sampleRate, double hz, double fromSeconds, double toSeconds)
{
    const auto from = std::min (static_cast<size_t> (fromSeconds * sampleRate), x.size());
    const auto to = std::min (static_cast<size_t> (toSeconds * sampleRate), x.size());
    return std::sqrt (tonePower (x.data() + from, to - from, sampleRate, hz));
}

// Energy away from the harmonics of `fundamental`, in dB relative to all energy, over 4096 samples
// from `fromSample`. This is what aliasing looks like on a steady tone.
inline double inharmonicDb (const std::vector<float>& x, double sampleRate, double fundamental, size_t fromSample = 0)
{
    constexpr size_t size = 4096;
    constexpr double maskBins = 5.0;
    const auto binHz = sampleRate / static_cast<double> (size);
    double total = 1.0e-30, away = 1.0e-30;
    for (size_t bin = 1; bin <= size / 2; ++bin)
    {
        const auto hz = static_cast<double> (bin) * binHz;
        const auto power = tonePower (x.data() + fromSample, size, sampleRate, hz);
        total += power;
        const auto nearest = std::max (1.0, std::round (hz / fundamental)) * fundamental;
        if (std::abs (hz - nearest) > maskBins * binHz && hz > maskBins * binHz)
            away += power;
    }
    return 10.0 * std::log10 (away / total);
}
}
