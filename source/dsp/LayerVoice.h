#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <random>
#include <vector>

namespace dsp
{
// The CLICK / TOP knobs in the units of specs/modules/layer.md.
struct LayerParams
{
    float source = 0.0f; // 0 Tone, 1 Noise, 2 Sample; read at the note-on
    float levelDb = -60.0f, tuneSemitones = 0.0f, decayMs = 4.0f, highPassHz = 20.0f, lowPassHz = 20000.0f;
    float startMs = 0.0f; // into the sample, in the file's own time; read at the note-on
};

// LAYER-04: your own file, summed to mono, at its own rate.
struct LayerSample
{
    std::vector<float> data;
    double rate = 48000.0;
};

// LAYER-01..03: one section (CLICK and TOP are two of these). Source, 12 dB/oct high-pass and low-pass, then the envelope (1 - t/D)^2.
class LayerVoice
{
public:
    static constexpr float levelOffDb = -60.0f;

    // Builds the noise table; call it off the audio thread (the processor constructor does).
    static void warmUp() { noise(); }

    void setParams (const LayerParams& p)
    {
        // The filters are smoothed in octaves, as BODY's cutoff is.
        targets = { p.levelDb, p.tuneSemitones, p.decayMs, std::log2 (std::max (p.highPassHz, 1.0f)), std::log2 (std::max (p.lowPassHz, 1.0f)) };
        pendingSource = p.source > 1.5f ? 2 : p.source > 0.5f ? 1 : 0;
        pendingStartMs = p.startMs;
    }

    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        smoothing = 1.0 - std::exp (-1.0 / (0.01 * sampleRate));
        residualDecay = std::exp (-1.0 / (residualSeconds * sampleRate));
        values = targets;
        active = false;
        residual = lastOut = 0.0;
    }

    // `level` is the note's whole gain (PLAY-01). PLAY-03: the last value of the layer it replaces fades away on top of the new note.
    // `file` is the section's sample at this note-on (null for none); it must stay alive while the layer rings.
    void trigger (float level, const LayerSample* file)
    {
        if (! active)
            values = targets; // next() skips smoothing while idle, so the knobs are stale
        residual = lastOut;
        velocity = level;
        source = pendingSource;
        sample = file;
        phase = 0.0;
        position = source == 2 && file != nullptr ? pendingStartMs * 0.001 * file->rate : 0.0;
        progress = 0.0;
        highPass = {};
        lowPass = {};
        coefficientsFor = { -1.0, -1.0 };
        if (source == 0)
        {
            // The tone enters the filters as if it had been running for a few samples, so their start-up does not smear its first cycle (LAYER-02f).
            const auto step = 3000.0 * std::exp2 (values[1] / 12.0) / sampleRate;
            for (int k = 16; k >= 1; --k)
                filter (std::sin (-2.0 * std::numbers::pi * step * k), values[3], values[4]);
        }
        // Sample without a sample, or a Start past its end: silence.
        active = source != 2 || (file != nullptr && position < static_cast<double> (file->data.size()));
    }

    float next()
    {
        if (! active && residual == 0.0)
            return 0.0f;

        double out = 0.0;
        if (active && progress >= 1.0)
            active = false;
        if (active)
        {
            for (size_t i = 0; i < values.size(); ++i)
                values[i] += smoothing * (targets[i] - values[i]);

            const auto ratio = std::exp2 (values[1] / 12.0);
            double x = 0.0;
            if (source == 2 && position >= static_cast<double> (sample->data.size()))
            {
                // The file has ended: its last value fades like a retrigger's (LAYER-04f).
                active = false;
                residual = lastOut;
            }
            else if (source == 2)
            {
                x = read (sample->data.data(), sample->data.size(), position, false);
                position += ratio * sample->rate / sampleRate;
            }
            else if (source == 1)
            {
                x = read (noise().table.data(), noiseSize, position, true);
                position += ratio * 48000.0 / sampleRate;
                if (position >= noiseSize)
                    position -= noiseSize;
            }
            else
            {
                x = std::sin (2.0 * std::numbers::pi * phase);
                phase += 3000.0 * ratio / sampleRate;
                phase -= std::floor (phase);
            }
            x = filter (x, values[3], values[4]);

            const auto gain = targets[0] <= levelOffDb && values[0] < levelOffDb + 0.5 ? 0.0 : std::pow (10.0, values[0] / 20.0);
            const auto g = (1.0 - progress) * (1.0 - progress);
            progress += 1.0 / (sampleRate * 0.001 * std::max (values[2], 1.0));
            out = active ? x * g * gain * velocity : 0.0;
        }

        out += residual;
        lastOut = out;
        residual *= residualDecay;
        if (std::abs (residual) < 1e-7)
            residual = 0.0;
        return static_cast<float> (out);
    }

    bool isActive() const { return active; }

private:
    // Two TPT state-variable filters (Q 0.707, prewarped), high-pass then low-pass.
    struct Svf
    {
        double a1 = 0.0, a2 = 0.0, a3 = 0.0, s1 = 0.0, s2 = 0.0;
        void set (double g)
        {
            a1 = 1.0 / (1.0 + g * (g + std::numbers::sqrt2));
            a2 = g * a1;
            a3 = g * a2;
        }
        // Returns {low-pass, band-pass}.
        std::array<double, 2> run (double x)
        {
            const auto v3 = x - s2;
            const auto v1 = a1 * s1 + a2 * v3;
            const auto v2 = s2 + a2 * s1 + a3 * v3;
            s1 = 2.0 * v1 - s1;
            s2 = 2.0 * v2 - s2;
            return { v2, v1 };
        }
    };

    double filter (double x, double highOct, double lowOct)
    {
        const auto limit = 0.49 * sampleRate;
        if (std::abs (highOct - coefficientsFor[0]) > 0.0)
            highPass.set (std::tan (std::numbers::pi * std::min (std::exp2 (highOct), limit) / sampleRate));
        if (std::abs (lowOct - coefficientsFor[1]) > 0.0)
            lowPass.set (std::tan (std::numbers::pi * std::min (std::exp2 (lowOct), limit) / sampleRate));
        coefficientsFor = { highOct, lowOct };
        const auto [low, band] = highPass.run (x);
        x = x - std::numbers::sqrt2 * band - low;
        return lowPass.run (x)[0];
    }

    // A fixed white-noise table made at 48 kHz, so every note plays the same stretch.
    static constexpr size_t noiseSize = size_t { 1 } << 18, taps = 32, phases = 512;

    struct Noise
    {
        std::vector<float> table, kernel; // kernel: a Kaiser-windowed sinc per fractional position, so a slower or faster rate keeps the colour
        Noise() : table (noiseSize), kernel (taps * (phases + 1))
        {
            std::mt19937 random (noiseSeed);
            std::uniform_real_distribution<float> uniform (-1.0f, 1.0f);
            for (auto& v : table)
                v = uniform (random);
            const auto bessel = [] (double x) {
                double sum = 1.0, term = 1.0;
                for (int k = 1; k < 30; ++k)
                {
                    term *= (x / (2.0 * k)) * (x / (2.0 * k));
                    sum += term;
                }
                return sum;
            };
            constexpr double beta = 5.0, half = taps / 2;
            for (size_t p = 0; p <= phases; ++p)
                for (size_t k = 0; k < taps; ++k)
                {
                    const auto d = static_cast<double> (k) - (half - 1.0) - static_cast<double> (p) / phases; // tap k sits at i - 15 + k
                    const auto x = std::numbers::pi * d;
                    const auto window = bessel (beta * std::sqrt (std::max (0.0, 1.0 - (d / half) * (d / half)))) / bessel (beta);
                    kernel[p * taps + k] = static_cast<float> (d == 0.0 ? 1.0 : std::sin (x) / x * window);
                }
        }
    };

    static const Noise& noise()
    {
        static const Noise shared;
        return shared;
    }

    // The kernel read at `at`; the noise table wraps (its size is a power of two), a file is zero outside itself.
    static double read (const float* data, size_t size, double at, bool wrap)
    {
        const auto i = static_cast<long long> (at);
        const auto p = static_cast<size_t> ((at - static_cast<double> (i)) * phases + 0.5);
        const float* k = &noise().kernel[p * taps];
        double sum = 0.0;
        for (size_t j = 0; j < taps; ++j)
        {
            const auto n = i + static_cast<long long> (j) - static_cast<long long> (taps / 2 - 1);
            if (wrap)
                sum += k[j] * data[n & static_cast<long long> (size - 1)];
            else if (n >= 0 && n < static_cast<long long> (size))
                sum += k[j] * data[n];
        }
        return sum;
    }

    static constexpr unsigned noiseSeed = 20260511u;
    static constexpr double residualSeconds = 0.003; // PLAY-03: the retrigger fade

    Svf highPass, lowPass;
    std::array<double, 5> targets { levelOffDb, 0.0, 4.0, std::log2 (20.0), std::log2 (20000.0) }, values = targets; // level, tune, decay, hp, lp
    std::array<double, 2> coefficientsFor { -1.0, -1.0 };
    double sampleRate = 44100.0, smoothing = 0.0, residualDecay = 0.0, phase = 0.0, position = 0.0, progress = 0.0;
    double velocity = 0.0, residual = 0.0, lastOut = 0.0;
    double pendingStartMs = 0.0;
    int pendingSource = 0, source = 0; // 0 Tone, 1 Noise, 2 Sample
    const LayerSample* sample = nullptr;
    bool active = false;
};
}
