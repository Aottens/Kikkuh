#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstddef>
#include <limits>
#include <numbers>
#include <tuple>
#include <utility>
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

// The largest difference between `count` samples of two signals, each from its own start.
inline float largestDifference (const std::vector<float>& a, size_t fromA, const std::vector<float>& b, size_t fromB, size_t count)
{
    float difference = 0.0f;
    for (size_t i = 0; i < count; ++i)
        difference = std::max (difference, std::abs (a[fromA + i] - b[fromB + i]));
    return difference;
}

// Frequency between two times, from interpolated rising zero crossings. For a tail, not a sweep.
// The sample of each hit: where the level rises above `threshold` after at least `gap` samples below it.
inline std::vector<int> hits (const std::vector<float>& x, float threshold, int gap)
{
    std::vector<int> found;
    int last = -gap;
    for (int i = 0; i < static_cast<int> (x.size()); ++i)
        if (std::abs (x[static_cast<size_t> (i)]) > threshold)
        {
            if (i - last >= gap)
                found.push_back (i);
            last = i;
        }
    return found;
}

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

// The RMS level between two times: for noise, where the peak between samples moves with the sample rate.
inline double rmsBetween (const std::vector<float>& x, double sampleRate, double fromSeconds, double toSeconds)
{
    const auto from = std::min (static_cast<size_t> (fromSeconds * sampleRate), x.size());
    const auto to = std::min (static_cast<size_t> (toSeconds * sampleRate), x.size());
    double sum = 0.0;
    for (size_t i = from; i < to; ++i)
        sum += static_cast<double> (x[i]) * x[i];
    return to > from ? std::sqrt (sum / static_cast<double> (to - from)) : 0.0;
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

// HARN-05: measurements on a whole kick, as kikkuh-measure prints them. Time zero is the file's start;
// the tool trims to `onset` first.

// The first sample above -60 dB of the peak, or -1 for silence.
inline int onset (const std::vector<float>& x)
{
    const auto threshold = static_cast<double> (peak (x)) * 0.001;
    if (threshold <= 0.0)
        return -1;
    for (size_t i = 0; i < x.size(); ++i)
        if (static_cast<double> (std::abs (x[i])) > threshold)
            return static_cast<int> (i);
    return -1;
}

// The time of the absolute peak.
inline double peakSeconds (const std::vector<float>& x, double sampleRate)
{
    size_t at = 0;
    for (size_t i = 1; i < x.size(); ++i)
        if (std::abs (x[i]) > std::abs (x[at]))
            at = i;
    return static_cast<double> (at) / sampleRate;
}

// The time after which the level stays below `db` relative to the peak (the last sample above it).
inline double secondsBelowDb (const std::vector<float>& x, double sampleRate, double db)
{
    const auto threshold = static_cast<double> (peak (x)) * std::pow (10.0, db / 20.0);
    for (size_t i = x.size(); i > 0; --i)
        if (static_cast<double> (std::abs (x[i - 1])) > threshold)
            return static_cast<double> (i - 1) / sampleRate;
    return 0.0;
}

// In-place radix-2 FFT; the size must be a power of two.
inline void fft (std::vector<std::complex<double>>& a)
{
    const auto n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i)
    {
        auto bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j)
            std::swap (a[i], a[j]);
    }
    for (size_t length = 2; length <= n; length <<= 1)
    {
        const auto step = std::polar (1.0, -2.0 * std::numbers::pi / static_cast<double> (length));
        for (size_t i = 0; i < n; i += length)
        {
            std::complex<double> w = 1.0;
            for (size_t k = 0; k < length / 2; ++k)
            {
                const auto u = a[i + k], v = a[i + k + length / 2] * w;
                a[i + k] = u + v;
                a[i + k + length / 2] = u - v;
                w *= step;
            }
        }
    }
}

// The analytic signal of the real signal in `a` (the size a power of two), each positive bin weighted by
// `weight (cycles per sample)`.
template <typename Weight>
inline void analytic (std::vector<std::complex<double>>& a, Weight weight)
{
    const auto n = a.size();
    fft (a);
    a[0] *= weight (0.0);
    for (size_t k = 1; k < n / 2; ++k)
        a[k] *= 2.0 * weight (static_cast<double> (k) / static_cast<double> (n));
    for (size_t k = n / 2; k < n; ++k)
        a[k] = 0.0;
    for (auto& v : a)
        v = std::conj (v);
    fft (a); // an inverse FFT through conjugation: the analytic signal is conj (a) / n
    for (auto& v : a)
        v = std::conj (v) / static_cast<double> (n);
}

// The local pitch in Hz around `seconds`; 0 when the level there is below -40 dB of the peak or fewer
// than four cycles are found. Three steps. 1: the Hilbert phase of 0.3 s of signal counts the cycles
// (one marker per 2 pi, so harmonics add none). 2: a smooth curve through the markers warps time so that
// the fundamental has a steady 64 samples per cycle and the harmonics sit at whole multiples of that.
// 3: a band around the fundamental is cut out, and the slope of its Hilbert phase, mapped back to real
// time, is the pitch. A sweep that halves within one cycle is followed, because the filter works on the
// warped signal and not on a window of the real one.
// ponytail: assumes the fundamental is the strongest part, so a kick whose 2nd harmonic outweighs it
// reads an octave up; and content above harmonic 32 aliases into the warp.
inline double pitchAt (const std::vector<float>& x, double sampleRate, double seconds)
{
    const auto size = static_cast<long> (x.size());
    const auto centre = std::lround (seconds * sampleRate);
    const auto level = std::lround (0.01 * sampleRate);
    float local = 0.0f;
    for (auto i = std::max (0L, centre - level); i < std::min (size, centre + level); ++i)
        local = std::max (local, std::abs (x[static_cast<size_t> (i)]));
    if (local < 0.01f * peak (x))
        return 0.0;

    // 1: cycle markers (time, cycle number)
    constexpr auto twoPi = 2.0 * std::numbers::pi;
    const auto first = std::max (0L, centre - std::lround (0.15 * sampleRate));
    const auto last = std::min (size, centre + std::lround (0.15 * sampleRate));
    if (last - first < 16)
        return 0.0;
    size_t n = 1;
    while (n < 2 * static_cast<size_t> (last - first))
        n <<= 1;
    std::vector<std::complex<double>> a (n);
    for (long i = first; i < last; ++i)
        a[static_cast<size_t> (i - first)] = static_cast<double> (x[static_cast<size_t> (i)]);
    analytic (a, [] (double) { return 1.0; });

    std::vector<std::pair<double, double>> markers;
    double phase = std::arg (a[0]), previous = phase, lastCycle = std::floor (phase / twoPi);
    for (long i = 1; i < last - first; ++i)
    {
        const auto angle = std::arg (a[static_cast<size_t> (i)]);
        auto step = angle - previous;
        previous = angle;
        step -= twoPi * std::round (step / twoPi);
        const auto cycle = std::floor ((phase + step) / twoPi);
        if (cycle > lastCycle)
        {
            markers.push_back ({ (static_cast<double> (first) + static_cast<double> (i - 1) + (cycle * twoPi - phase) / step) / sampleRate, cycle });
            lastCycle = cycle;
        }
        phase += step;
    }
    // A kick's pitch only falls, so a cycle much shorter than the one before it, near the start, is a
    // click and not a tone: the markers up to there are noise and would bend the warp.
    // ponytail: only the first 14 markers are checked, and a rising pitch there is taken for noise.
    size_t noise = 0;
    for (size_t i = 2; i < std::min<size_t> (markers.size(), 14); ++i)
        if (markers[i].first - markers[i - 1].first < 0.7 * (markers[i - 1].first - markers[i - 2].first))
            noise = i - 1;
    while (noise + 2 < markers.size() && markers[noise + 1].first - markers[noise].first < 0.3 * (markers[noise + 2].first - markers[noise + 1].first))
        ++noise;
    if (noise > 0)
        markers.erase (markers.begin(), markers.begin() + static_cast<std::ptrdiff_t> (noise));
    if (markers.size() < 4)
        return 0.0;

    // 2: up to 17 markers around `seconds`
    size_t at = 0;
    while (at + 1 < markers.size() && markers[at + 1].first <= seconds)
        ++at;
    const auto low = at > 8 ? at - 8 : 0;
    const auto high = std::min (markers.size() - 1, at + 9);
    std::vector<std::pair<double, double>> knots (markers.begin() + static_cast<std::ptrdiff_t> (low), markers.begin() + static_cast<std::ptrdiff_t> (high) + 1);

    constexpr size_t perCycle = 64, reach = perCycle / 8;
    double result = 0.0;
    for (int pass = 0; pass < 2; ++pass)
    {
        if (knots.size() < 4)
            return 0.0;
        // When the stretch starts with the file, the warp starts there too, so the real onset is not cut.
        const auto from = first == 0 ? 0L : static_cast<long> (std::ceil (knots.front().first * sampleRate));
        const auto to = static_cast<long> (std::floor (knots.back().first * sampleRate));
        if (to - from < 8)
            return 0.0;
        const auto samples = static_cast<size_t> (to - from + 1);
        std::vector<double> cycles (samples); // n at each sample from `from` to `to`, landing on every knot
        const auto timeAt = [&] (size_t i) { return static_cast<double> (from + static_cast<long> (i)) / sampleRate; };

        if (pass == 0)
        {
            // ln (pitch) is a cubic Hermite curve through one value per cycle, each value nudged until the
            // cycle holds exactly one cycle; a pitch that halves from cycle to cycle comes out smooth.
            const auto intervals = knots.size() - 1;
            std::vector<double> middle (intervals), logPitch (intervals), slope (intervals), integral (samples);
            for (size_t i = 0; i < intervals; ++i)
            {
                middle[i] = 0.5 * (knots[i].first + knots[i + 1].first);
                logPitch[i] = std::log ((knots[i + 1].second - knots[i].second) / (knots[i + 1].first - knots[i].first));
            }
            const auto integralAt = [&] (double t)
            {
                const auto position = t * sampleRate - static_cast<double> (from);
                const auto i = static_cast<size_t> (std::clamp (std::floor (position), 0.0, static_cast<double> (samples - 2)));
                return integral[i] + (integral[i + 1] - integral[i]) * (position - static_cast<double> (i));
            };
            for (int round = 0; round < 6; ++round)
            {
                for (size_t i = 0; i < intervals; ++i)
                {
                    const auto before = i > 0 ? i - 1 : i, after = i + 1 < intervals ? i + 1 : i;
                    slope[i] = before == after ? 0.0 : (logPitch[after] - logPitch[before]) / (middle[after] - middle[before]);
                }
                size_t k = 0;
                for (size_t i = 0; i < samples; ++i)
                {
                    const auto t = timeAt (i);
                    double v;
                    if (t <= middle.front())
                        v = logPitch.front(); // before the first marker the pitch stays put: an exponential run-out would explode
                    else if (t >= middle.back())
                        v = logPitch.back() + slope.back() * (t - middle.back());
                    else
                    {
                        while (k + 1 < intervals && middle[k + 1] < t)
                            ++k;
                        const auto h = middle[k + 1] - middle[k], u = (t - middle[k]) / h;
                        v = logPitch[k] * (2 * u * u * u - 3 * u * u + 1) + h * slope[k] * (u * u * u - 2 * u * u + u)
                            + logPitch[k + 1] * (-2 * u * u * u + 3 * u * u) + h * slope[k + 1] * (u * u * u - u * u);
                    }
                    const auto hz = std::exp (v);
                    integral[i] = i == 0 ? 0.0 : integral[i - 1] + hz / sampleRate; // rectangle rule: the next round corrects the sum
                }
                if (round == 5)
                    break;
                for (size_t i = 0; i < intervals; ++i)
                    logPitch[i] -= std::log (std::max (integralAt (knots[i + 1].first) - integralAt (knots[i].first), 1.0e-9));
            }
            for (size_t i = 0, k = 0; i < samples; ++i)
            {
                while (k + 1 < intervals && knots[k + 1].first <= timeAt (i))
                    ++k;
                const auto start = integralAt (knots[k].first), whole = integralAt (knots[k + 1].first) - start;
                cycles[i] = knots[k].second + (integral[i] - start) / whole * (knots[k + 1].second - knots[k].second);
            }
        }
        else
        {
            // The knots are dense now: n (t) is a straight line between them.
            for (size_t i = 0, k = 0; i < samples; ++i)
            {
                while (k + 2 < knots.size() && knots[k + 1].first <= timeAt (i))
                    ++k;
                cycles[i] = knots[k].second + (timeAt (i) - knots[k].first) / (knots[k + 1].first - knots[k].first) * (knots[k + 1].second - knots[k].second);
            }
        }

        // The signal at whole fractions of a cycle (cubic interpolation), and the real time of each.
        std::vector<double> times, warped;
        const auto firstTarget = knots.front().second - std::floor ((knots.front().second - cycles.front()) * perCycle) / perCycle;
        size_t p = 0;
        if (! std::isfinite (firstTarget) || ! std::isfinite (cycles.back()) || cycles.back() - firstTarget > 64.0)
            return 0.0; // no sane cycle count: noise, not a tone
        for (size_t j = 0; j < 64 * perCycle; ++j) // at most 64 cycles, so the loop always ends
        {
            const auto target = firstTarget + static_cast<double> (j) / static_cast<double> (perCycle);
            while (p + 1 < cycles.size() && cycles[p + 1] <= target)
                ++p;
            if (p + 1 >= cycles.size())
                break;
            const auto position = static_cast<double> (from) + static_cast<double> (p) + (target - cycles[p]) / (cycles[p + 1] - cycles[p]);
            const auto base = static_cast<long> (std::floor (position));
            const auto f = position - static_cast<double> (base);
            const auto at4 = [&] (long i) { return static_cast<double> (x[static_cast<size_t> (std::clamp (i, 0L, size - 1))]); };
            const auto y0 = at4 (base - 1), y1 = at4 (base), y2 = at4 (base + 1), y3 = at4 (base + 2);
            warped.push_back (y1 + 0.5 * f * (y2 - y0 + f * (2.0 * y0 - 5.0 * y1 + 4.0 * y2 - y3 + f * (3.0 * (y1 - y2) + y3 - y0))));
            times.push_back (position / sampleRate);
        }
        const auto count = warped.size();
        if (count < 4 * perCycle)
            return 0.0;

        // 3: the fundamental of the warped signal, and the slope of its phase
        size_t m = 64;
        while (m < 2 * count)
            m <<= 1;
        std::vector<std::complex<double>> b (m);
        const auto taper = std::min<size_t> (2 * perCycle, count / 4); // soft ends keep the Hilbert transform from ringing
        for (size_t j = 0; j < count; ++j)
        {
            const auto edge = first == 0 ? count - 1 - j : std::min (j, count - 1 - j);
            b[j] = edge < taper ? warped[j] * (0.5 - 0.5 * std::cos (std::numbers::pi * static_cast<double> (edge) / static_cast<double> (taper))) : warped[j];
        }
        const auto unit = 1.0 / static_cast<double> (perCycle);
        // The first pass keeps to the fundamental; the second opens the band a little, to the phase wobble
        // the first one left, while the warp keeps the 2nd harmonic out at 2.
        const auto lo0 = pass == 0 ? 0.4 : 0.3, lo1 = pass == 0 ? 0.7 : 0.5, hi1 = pass == 0 ? 1.3 : 1.5, hi0 = pass == 0 ? 1.6 : 1.75;
        analytic (b, [=] (double f) { f /= unit; return f < lo0 || f > hi0 ? 0.0 : f < lo1 ? (f - lo0) / (lo1 - lo0) : f > hi1 ? (hi0 - f) / (hi0 - hi1) : 1.0; });
        std::vector<double> unwrapped (count);
        unwrapped[0] = std::arg (b[0]);
        for (size_t j = 1; j < count; ++j)
        {
            auto step = std::arg (b[j]) - std::arg (b[j - 1]);
            step -= twoPi * std::round (step / twoPi);
            unwrapped[j] = unwrapped[j - 1] + step;
        }
        const auto pitchAtIndex = [&] (size_t j) { return (unwrapped[j + reach] - unwrapped[j - reach]) / twoPi / (times[j + reach] - times[j - reach]); };
        if (seconds < times[reach] || seconds >= times[count - 1 - reach])
            return pass == 0 ? 0.0 : result;
        size_t j = reach;
        while (j + 1 < count - reach && times[j + 1] <= seconds)
            ++j;
        const auto w = (seconds - times[j]) / (times[j + 1] - times[j]);
        result = std::max ((1.0 - w) * pitchAtIndex (j) + w * pitchAtIndex (j + 1), 0.0);

        // The phase of the fundamental is a finer n (t) than the markers: the knots of the next pass.
        knots.clear();
        for (size_t i = perCycle / 2; i + perCycle / 2 < count; i += 4)
            if (knots.empty() || unwrapped[i] / twoPi > knots.back().second)
                knots.push_back ({ times[i], unwrapped[i] / twoPi });
    }
    return result;
}

// The steady pitch at the end of the tail: the median pitch over the stretch just before -40 dB.
inline double tailHz (const std::vector<float>& x, double sampleRate)
{
    const auto end = secondsBelowDb (x, sampleRate, -40.0);
    std::vector<double> found;
    for (auto t = std::max (0.5 * end, end - 0.3); t < end - 0.03; t += 0.01)
        if (const auto hz = pitchAt (x, sampleRate, t); hz > 0.0)
            found.push_back (hz);
    if (found.empty())
        if (const auto hz = pitchAt (x, sampleRate, 0.5 * end); hz > 0.0)
            found.push_back (hz);
    if (found.empty())
        return 0.0;
    std::nth_element (found.begin(), found.begin() + static_cast<std::ptrdiff_t> (found.size() / 2), found.end());
    return found[found.size() / 2];
}

struct TailStretch
{
    double from, to;
};

// From where the pitch stays within 1 semitone of the tail, to where the level is below -40 dB for good.
// ponytail: scans back in 5 ms steps with a pitch that is smeared over two periods, so `from` lands
// up to a period late on a sweep that ends smoothly.
inline TailStretch tailStretch (const std::vector<float>& x, double sampleRate)
{
    const auto tail = tailHz (x, sampleRate);
    const auto to = secondsBelowDb (x, sampleRate, -40.0);
    if (tail <= 0.0)
        return { to, to };
    auto from = to;
    for (auto t = to; t >= 0.0; t -= 0.005)
    {
        const auto hz = pitchAt (x, sampleRate, t);
        if (hz <= 0.0)
            continue;
        if (std::abs (12.0 * std::log2 (hz / tail)) > 1.0)
            break;
        from = t;
    }
    return { from, to };
}

// The levels of harmonics 2 to 8 relative to the fundamental, in dB; empty when the stretch is shorter
// than 8 periods. A harmonic above Nyquist reads -200.
inline std::vector<double> harmonicsDb (const std::vector<float>& x, double sampleRate, double fundamental, double fromSeconds, double toSeconds)
{
    if (fundamental <= 0.0 || (toSeconds - fromSeconds) * fundamental < 8.0)
        return {};
    const auto reference = std::max (toneLevel (x, sampleRate, fundamental, fromSeconds, toSeconds), 1.0e-10);
    std::vector<double> db;
    for (int h = 2; h <= 8; ++h)
    {
        const auto hz = fundamental * h;
        db.push_back (hz >= 0.5 * sampleRate ? -200.0 : 20.0 * std::log10 (std::max (toneLevel (x, sampleRate, hz, fromSeconds, toSeconds), 1.0e-10) / reference));
    }
    return db;
}

// Percent of the energy below 300 Hz, 300 Hz-1 kHz, 1-4 kHz and above 4 kHz in a stretch (Hann window);
// all 0 for an empty or silent one. Energy above `maxHz` is left out, so rates with different Nyquists compare.
inline std::array<double, 4> bandShares (const std::vector<float>& x, double sampleRate, double fromSeconds, double toSeconds,
                                         double maxHz = std::numeric_limits<double>::infinity())
{
    const auto from = std::min (static_cast<size_t> (std::max (0.0, fromSeconds) * sampleRate), x.size());
    const auto to = std::min (static_cast<size_t> (std::max (0.0, toSeconds) * sampleRate), x.size());
    std::array<double, 4> shares {};
    if (to < from + 2)
        return shares;
    const auto count = to - from;
    size_t size = 1024;
    while (size < 2 * count)
        size <<= 1;
    std::vector<std::complex<double>> a (size);
    for (size_t i = 0; i < count; ++i)
        a[i] = static_cast<double> (x[from + i]) * (0.5 - 0.5 * std::cos (2.0 * std::numbers::pi * static_cast<double> (i) / static_cast<double> (count - 1)));
    fft (a);
    double total = 0.0;
    for (size_t k = 0; k <= size / 2; ++k)
    {
        const auto hz = static_cast<double> (k) * sampleRate / static_cast<double> (size);
        if (hz > maxHz)
            break;
        const auto energy = std::norm (a[k]);
        shares[hz < 300.0 ? 0 : hz < 1000.0 ? 1 : hz < 4000.0 ? 2 : 3] += energy;
        total += energy;
    }
    if (total <= 0.0)
        return {};
    for (auto& share : shares)
        share *= 100.0 / total;
    return shares;
}

// BODY's curve (specs/modules/body.md): 1 at t = 0, exactly 0 at t = decay.
inline double curveAt (double t, double decay, double shape)
{
    return t >= decay ? 0.0 : std::pow (1.0 - t / decay, std::pow (3.0, 1.0 + shape / 100.0));
}

// The (decay, shape) that minimises `error (decay, shape)`: a coarse grid, then a shrinking pattern search.
template <typename Error>
inline std::pair<double, double> searchCurve (Error error, double lowDecay, double highDecay)
{
    constexpr int steps = 60;
    double ratio = std::pow (highDecay / lowDecay, 1.0 / (steps - 1.0));
    double bestDecay = lowDecay, bestShape = 0.0, bestError = 1.0e300;
    for (int i = 0; i < steps; ++i)
        for (int s = -100; s <= 100; s += 10)
        {
            const auto decay = lowDecay * std::pow (ratio, i);
            if (const auto e = error (decay, s); e < bestError)
                bestError = e, bestDecay = decay, bestShape = s;
        }

    double stepShape = 10.0;
    for (int round = 0; round < 14; ++round)
    {
        for (int pass = 0; pass < 50; ++pass)
        {
            bool better = false;
            for (const auto& [decay, shape] : { std::pair { bestDecay * ratio, bestShape }, std::pair { bestDecay / ratio, bestShape },
                                                std::pair { bestDecay, bestShape + stepShape }, std::pair { bestDecay, bestShape - stepShape } })
            {
                const auto d = std::clamp (decay, lowDecay, highDecay);
                const auto s = std::clamp (shape, -100.0, 100.0);
                if (const auto e = error (d, s); e < bestError)
                    bestError = e, bestDecay = d, bestShape = s, better = true;
            }
            if (! better)
                break;
        }
        ratio = std::sqrt (ratio);
        stepShape *= 0.5;
    }
    return { bestDecay, bestShape };
}

struct PitchFit
{
    double tuneHz, amount, decaySeconds, shape, errorCents;
    int outliers = 0; // readings left out because they stood far from their neighbours
};

// The nearest BODY pitch curve, offset (t) = amount * curve (t) in semitones above the tail: Tune from
// the tail, then Amount (0..72), Decay (1..1000 ms) and Shape (-100..100) by least squares in cents over
// about 80 times from 5 ms (the first cycle of a file is not reliable) to -40 dB, spaced geometrically.
// ponytail: fits the smeared pitch of pitchAt, so Decay reads a little long on a very fast sweep; and
// Decay and Shape trade off against each other on a noisy track.
inline PitchFit fitPitch (const std::vector<float>& x, double sampleRate)
{
    PitchFit fit { tailHz (x, sampleRate), 0.0, 0.0, 0.0, 0.0 };
    const auto end = secondsBelowDb (x, sampleRate, -40.0);
    if (fit.tuneHz <= 0.0 || end <= 0.005)
        return fit;

    std::vector<double> times, semitones;
    constexpr int points = 80;
    for (int i = 0; i < points; ++i)
    {
        const auto t = 0.005 * std::pow (end / 0.005, i / (points - 1.0));
        if (const auto hz = pitchAt (x, sampleRate, t); hz > 0.0)
        {
            times.push_back (t);
            semitones.push_back (12.0 * std::log2 (hz / fit.tuneHz));
        }
    }

    // Leave out readings far from the median of their neighbours (4 each side, in time): a click or a
    // glitch. "Far" is 3 semitones or 6 times the typical distance from that median, whichever is more.
    // ponytail: a glitch longer than about 4 readings (a third of a decade in time) is not caught.
    std::vector<double> distance (times.size());
    for (size_t i = 0; i < times.size(); ++i)
    {
        std::vector<double> near (semitones.begin() + static_cast<std::ptrdiff_t> (i > 4 ? i - 4 : 0), semitones.begin() + static_cast<std::ptrdiff_t> (std::min (times.size(), i + 5)));
        std::nth_element (near.begin(), near.begin() + static_cast<std::ptrdiff_t> (near.size() / 2), near.end());
        distance[i] = std::abs (semitones[i] - near[near.size() / 2]);
    }
    auto sorted = distance;
    std::sort (sorted.begin(), sorted.end());
    const auto limit = std::max (3.0, sorted.empty() ? 0.0 : 6.0 * sorted[sorted.size() / 2]);
    std::vector<double> keptTimes, keptSemitones;
    for (size_t i = 0; i < times.size(); ++i)
        if (distance[i] <= limit)
        {
            keptTimes.push_back (times[i]);
            keptSemitones.push_back (semitones[i]);
        }
    const auto total0 = times.size();
    times = keptTimes;
    semitones = keptSemitones;

    const auto amountFor = [&] (double decay, double shape)
    {
        double top = 0.0, bottom = 1.0e-12;
        for (size_t i = 0; i < times.size(); ++i)
        {
            const auto g = curveAt (times[i], decay, shape);
            top += semitones[i] * g;
            bottom += g * g;
        }
        return std::clamp (top / bottom, 0.0, 72.0);
    };
    const auto error = [&] (double decay, double shape)
    {
        const auto amount = amountFor (decay, shape);
        double sum = 0.0;
        for (size_t i = 0; i < times.size(); ++i)
            sum += std::pow (semitones[i] - amount * curveAt (times[i], decay, shape), 2.0);
        return std::sqrt (sum / std::max<double> (1.0, static_cast<double> (times.size()))) * 100.0;
    };

    // A click bends the readings after it for a few ms, smoothly, so no neighbour test sees it. Fit,
    // then leave out what stands far from the fit (1 semitone, or 4 typical deviations if that is more) in
    // the first 20 ms only, so a real slow second stage of a sweep stays, and fit again, up to 3 times and never more than a third of the readings in all.
    // ponytail: a kick the BODY curve cannot follow has a large typical deviation and loses little.
    const auto total = times.size();
    auto [decay, shape] = searchCurve (error, 0.001, 1.0);
    for (int round = 0; round < 3 && times.size() > 8; ++round)
    {
        const auto amount = amountFor (decay, shape);
        std::vector<double> miss (times.size());
        for (size_t i = 0; i < times.size(); ++i)
            miss[i] = std::abs (semitones[i] - amount * curveAt (times[i], decay, shape));
        auto sortedMiss = miss;
        std::sort (sortedMiss.begin(), sortedMiss.end());
        const auto bound = std::max (1.0, 4.0 * 1.4826 * sortedMiss[sortedMiss.size() / 2]);
        // A click is one stretch from the start: when a reading in the first 20 ms is bad, so is all before it.
        size_t clickEnd = 0;
        for (size_t i = 0; i < times.size() && times[i] < 0.02; ++i)
            if (miss[i] > bound)
                clickEnd = i + 1;
        std::vector<double> keepT, keepS;
        for (size_t i = 0; i < times.size(); ++i)
            if (i >= clickEnd && (times[i] >= 0.02 || miss[i] <= bound)) // after 20 ms only the neighbour test may drop a reading
            {
                keepT.push_back (times[i]);
                keepS.push_back (semitones[i]);
            }
        if (keepT.size() == times.size() || keepT.size() * 3 < total * 2)
            break;
        times = keepT;
        semitones = keepS;
        std::tie (decay, shape) = searchCurve (error, 0.001, 1.0);
    }
    fit.outliers = static_cast<int> (total0 - times.size());
    fit.amount = amountFor (decay, shape);
    fit.decaySeconds = decay;
    fit.shape = shape;
    fit.errorCents = error (decay, shape);
    return fit;
}

struct AmpFit
{
    double decaySeconds, shape, errorDb;
};

// The nearest BODY amp curve on the level envelope (the peak of each 10 ms window, timed from the
// overall peak): Decay 10..2000 ms and Shape by least squares in dB over what stays above -60 dB.
// ponytail: a window shorter than half a period can miss the lobe, so a tune below 50 Hz makes the
// envelope ragged; the error says so.
inline AmpFit fitAmp (const std::vector<float>& x, double sampleRate)
{
    const auto window = std::max<size_t> (1, static_cast<size_t> (0.01 * sampleRate));
    const auto top = static_cast<double> (peak (x));
    const auto start = peakSeconds (x, sampleRate);
    std::vector<double> times, levels;
    for (size_t from = 0; from < x.size(); from += window)
    {
        size_t at = from;
        for (size_t i = from; i < std::min (from + window, x.size()); ++i)
            if (std::abs (x[i]) > std::abs (x[at]))
                at = i;
        const auto t = static_cast<double> (at) / sampleRate - start;
        const auto level = static_cast<double> (std::abs (x[at]));
        if (t >= 0.0 && level > top * 0.001)
        {
            times.push_back (t);
            levels.push_back (20.0 * std::log10 (level / top));
        }
    }
    if (times.empty())
        return { 0.0, 0.0, 0.0 };

    const auto error = [&] (double decay, double shape)
    {
        double sum = 0.0;
        for (size_t i = 0; i < times.size(); ++i)
            sum += std::pow (levels[i] - 20.0 * std::log10 (std::max (curveAt (times[i], decay, shape), 1.0e-6)), 2.0);
        return std::sqrt (sum / static_cast<double> (times.size()));
    };
    const auto [decay, shape] = searchCurve (error, 0.01, 2.0);
    return { decay, shape, error (decay, shape) };
}

// The RMS of the side (L - R) / 2 relative to the mid (L + R) / 2 over a stretch, in dB; identical
// channels read about -200.
inline double sideToMidDb (const std::vector<float>& left, const std::vector<float>& right, double sampleRate, double fromSeconds, double toSeconds)
{
    const auto size = std::min (left.size(), right.size());
    const auto from = std::min (static_cast<size_t> (std::max (0.0, fromSeconds) * sampleRate), size);
    const auto to = std::min (static_cast<size_t> (std::max (0.0, toSeconds) * sampleRate), size);
    double mid = 0.0, side = 0.0;
    for (size_t i = from; i < to; ++i)
    {
        const auto l = static_cast<double> (left[i]), r = static_cast<double> (right[i]);
        mid += 0.25 * (l + r) * (l + r);
        side += 0.25 * (l - r) * (l - r);
    }
    return 10.0 * std::log10 ((side + 1.0e-20) / (mid + 1.0e-20));
}

// HARN-07a: ITU-R BS.1770 K-weighting, designed for any rate as in libebur128. Coefficients are
// b0, b1, b2, a1, a2 (a0 = 1).
struct KWeighting
{
    std::array<double, 5> shelf, highpass;
};

inline KWeighting kWeighting (double rate)
{
    const auto pi = std::numbers::pi;
    const auto f0 = 1681.974450955533, gain = 3.999843853973347, q = 0.7071752369554196;
    const auto vh = std::pow (10.0, gain / 20.0), vb = std::pow (vh, 0.4996667741545416);
    auto k = std::tan (pi * f0 / rate);
    const auto a0 = 1.0 + k / q + k * k;
    KWeighting w;
    w.shelf = { (vh + vb * k / q + k * k) / a0, 2.0 * (k * k - vh) / a0, (vh - vb * k / q + k * k) / a0, 2.0 * (k * k - 1.0) / a0, (1.0 - k / q + k * k) / a0 };

    const auto f1 = 38.13547087602444, q1 = 0.5003270373238773;
    k = std::tan (pi * f1 / rate);
    const auto d = 1.0 + k / q1 + k * k;
    w.highpass = { 1.0, -2.0, 1.0, 2.0 * (k * k - 1.0) / d, (1.0 - k / q1 + k * k) / d };
    return w;
}

// Integrated loudness in LUFS over the 400 ms from time zero (HARN-05a: the onset of the mid), both
// channels weighted equally. Samples past the end count as silence; silence reads -inf.
inline double loudness (const std::vector<float>& left, const std::vector<float>& right, double rate)
{
    const auto size = std::min (left.size(), right.size());
    std::vector<float> mid (size);
    for (size_t i = 0; i < size; ++i)
        mid[i] = 0.5f * (left[i] + right[i]);
    const auto start = onset (mid);
    if (start < 0)
        return -std::numeric_limits<double>::infinity();

    const auto k = kWeighting (rate);
    const auto window = static_cast<size_t> (std::llround (0.4 * rate));
    const auto meanSquare = [&] (const std::vector<float>& channel) {
        std::array<double, 4> shelfState {}, highState {}; // x1, x2, y1, y2
        const auto run = [] (const std::array<double, 5>& c, std::array<double, 4>& s, double x) {
            const auto y = c[0] * x + c[1] * s[0] + c[2] * s[1] - c[3] * s[2] - c[4] * s[3];
            s = { x, s[0], y, s[2] };
            return y;
        };
        double sum = 0.0;
        for (size_t i = 0; i < window; ++i)
        {
            const auto at = static_cast<size_t> (start) + i;
            const auto y = run (k.highpass, highState, run (k.shelf, shelfState, at < size ? static_cast<double> (channel[at]) : 0.0));
            sum += y * y;
        }
        return sum / static_cast<double> (window);
    };
    return -0.691 + 10.0 * std::log10 (meanSquare (left) + meanSquare (right));
}
}
