#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <vector>

namespace dsp
{
// BODY-01: sine, triangle, saw and square as band-limited wavetables, all sums of sines in sine phase,
// so they start at zero and share their fundamental in phase. Mip j holds up to 2^j harmonics.
class MorphOsc
{
public:
    // Builds the shared tables; call it off the audio thread (the processor constructor does).
    static void warmUp() { tables(); }

    void reset() { phase = 0.0; }

    float next (double hz, double sampleRate, float wave)
    {
        const auto& t = tables();

        // Blend the two mips that lie wholly below Nyquist, so a sweep never steps.
        const auto headroom = std::log2 (std::max (0.5 * sampleRate / std::max (hz, 1.0e-3), 1.0));
        int lo = 0, hi = 0;
        double mix = 0.0;
        if (headroom >= static_cast<double> (mips))
            lo = hi = mips - 1;
        else if (headroom >= 1.0)
        {
            hi = static_cast<int> (headroom);
            lo = hi - 1;
            mix = headroom - hi;
        }

        const auto position = phase * tableSize;
        const auto index = static_cast<int> (position);
        const auto frac = static_cast<float> (position - index);
        const auto read = [&] (int shape, int mip) {
            const auto* table = t.table (shape, mip);
            return table[index] + frac * (table[index + 1] - table[index]);
        };
        const auto read2 = [&] (int shape) {
            const auto a = read (shape, lo);
            return lo == hi ? a : a + static_cast<float> (mix) * (read (shape, hi) - a);
        };

        const auto position01 = std::clamp (wave, 0.0f, 3.0f);
        const auto first = std::min (static_cast<int> (position01), 2);
        const auto a = read2 (first);
        const auto out = a + (position01 - static_cast<float> (first)) * (read2 (first + 1) - a);

        phase += hz / sampleRate;
        phase -= std::floor (phase);
        return out;
    }

private:
    static constexpr int tableSize = 4096, mips = 11; // 1 ... 1024 harmonics

    struct Tables
    {
        std::vector<float> data = std::vector<float> (4 * mips * (tableSize + 1));

        const float* table (int shape, int mip) const { return data.data() + (shape * mips + mip) * (tableSize + 1); }

        static double coefficient (int shape, int n)
        {
            const auto odd = n % 2 == 1;
            switch (shape)
            {
                case 0: return n == 1 ? 1.0 : 0.0;
                case 1: return odd ? ((n / 2) % 2 == 0 ? 1.0 : -1.0) / (static_cast<double> (n) * n) : 0.0;
                case 2: return (odd ? 1.0 : -1.0) / n;
                default: return odd ? 1.0 / n : 0.0;
            }
        }

        Tables()
        {
            std::array<double, tableSize> sine {};
            for (int i = 0; i < tableSize; ++i)
                sine[static_cast<size_t> (i)] = std::sin (2.0 * std::numbers::pi * i / tableSize);

            for (int shape = 0; shape < 4; ++shape)
            {
                // Each mip adds only its new partials to the one before it.
                std::array<double, tableSize> sum {};
                double peak = 0.0;
                int n = 1;
                for (int mip = 0; mip < mips; ++mip)
                {
                    for (; n <= (1 << mip); ++n)
                        if (const auto c = coefficient (shape, n); c != 0.0)
                            for (int i = 0; i < tableSize; ++i)
                                sum[static_cast<size_t> (i)] += c * sine[static_cast<size_t> ((n * i) & (tableSize - 1))];

                    auto* out = data.data() + (shape * mips + mip) * (tableSize + 1);
                    for (int i = 0; i < tableSize; ++i)
                    {
                        out[i] = static_cast<float> (sum[static_cast<size_t> (i)]);
                        if (mip == mips - 1)
                            peak = std::max (peak, std::abs (sum[static_cast<size_t> (i)]));
                    }
                }

                // One constant per wave, the peak of its fullest mip, so the fundamental stays level across mips.
                for (int mip = 0; mip < mips; ++mip)
                {
                    auto* out = data.data() + (shape * mips + mip) * (tableSize + 1);
                    for (int i = 0; i < tableSize; ++i)
                        out[i] = static_cast<float> (out[i] / peak);
                    out[tableSize] = out[0];
                }
            }
        }
    };

    static const Tables& tables()
    {
        static const Tables shared;
        return shared;
    }

    double phase = 0.0;
};
}
