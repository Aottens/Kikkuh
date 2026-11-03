#pragma once

#include <algorithm>
#include <cmath>
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
}
