#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <numbers>

namespace dsp
{
// BODY-06: a hard clip at +-1, run at 8x so its harmonics do not fold back, at zero reported latency (ENG-02).
// The up and down filters are minimum-phase IIRs: an inverse Chebyshev low-pass, order 12, 60 dB stopband,
// its stop edge at the base rate's Nyquist. The ratio is fixed, so the coefficients do not depend on the sample rate.
class BodyDrive
{
public:
    BodyDrive()
    {
        constexpr double stopbandDb = 60.0;
        const auto eps = 1.0 / std::sqrt (std::pow (10.0, stopbandDb / 10.0) - 1.0);
        const auto mu = std::asinh (1.0 / eps) / order;
        const auto wa = std::tan (std::numbers::pi * 0.5 / oversampling);
        for (size_t k = 1; k <= sections; ++k)
        {
            const auto th = std::numbers::pi * static_cast<double> (2 * k - 1) / (2.0 * order);
            const auto p1 = std::complex<double> (-std::sinh (mu) * std::sin (th), std::cosh (mu) * std::cos (th));
            const auto p = wa / p1;
            const auto zi = wa / std::cos (th);
            const auto zp = (1.0 + p) / (1.0 - p);
            const auto zz = std::complex<double> (1.0, zi) / std::complex<double> (1.0, -zi);
            auto& c = coeffs[k - 1];
            c.a1 = -2.0 * zp.real();
            c.a2 = std::norm (zp);
            const auto b1 = -2.0 * zz.real();
            const auto g = (1.0 + c.a1 + c.a2) / (2.0 + b1);
            c.b0 = g;
            c.b1 = g * b1;
            c.b2 = g;
        }
    }

    void reset()
    {
        up = {};
        down = {};
    }

    float process (float x, float driveDb)
    {
        const auto m = std::clamp (driveDb / 0.5f, 0.0f, 1.0f);
        if (m == 0.0f)
        {
            if (! clean)
                reset();
            clean = true;
            return x;
        }
        clean = false;

        const auto gain = std::pow (10.0, driveDb / 20.0);
        double first = 0.0;
        for (int i = 0; i < oversampling; ++i)
        {
            auto v = i == 0 ? static_cast<double> (x) * oversampling : 0.0;
            for (size_t s = 0; s < sections; ++s)
                v = run (coeffs[s], up[s], v);
            v = std::clamp (v * gain, -1.0, 1.0);
            for (size_t s = 0; s < sections; ++s)
                v = run (coeffs[s], down[s], v);
            if (i == 0)
                first = v;
        }
        return static_cast<float> ((1.0 - m) * x + m * first);
    }

private:
    static constexpr int order = 12, oversampling = 8;
    static constexpr size_t sections = order / 2;

    struct Biquad { double b0 = 0, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };
    struct State { double s1 = 0, s2 = 0; };

    // Transposed direct form II.
    static double run (const Biquad& c, State& st, double x)
    {
        const auto y = c.b0 * x + st.s1;
        st.s1 = c.b1 * x - c.a1 * y + st.s2;
        st.s2 = c.b2 * x - c.a2 * y;
        return y;
    }

    std::array<Biquad, sections> coeffs {};
    std::array<State, sections> up {}, down {};
    bool clean = true;
};

// BODY-05: a 24 dB/oct state-variable low-pass (two TPT sections). At the top of the knob it is bypassed,
// so it changes nothing until it is used; the blend is complete one tenth of an octave below 20 kHz.
class BodyFilter
{
public:
    void prepare (double newRate)
    {
        rate = newRate;
        st = {};
    }

    // The envelope moves the cutoff only: `knobHz` decides whether the filter is in at all.
    float process (float x, float knobHz, float resonancePercent, float envOctaves = 0.0f, float envCurve = 0.0f)
    {
        // Below 0.001 is rounding noise from a cutoff that went through log2 and back, not a filter in use.
        const auto m = std::clamp (std::log2 (20000.0f / knobHz) * 10.0f, 0.0f, 1.0f);
        if (m < 0.001f)
        {
            st = { 0.0, x, 0.0, x }; // as if x had been steady: opening the filter then starts from the signal, not from silence
            return x;
        }

        const auto fc = std::clamp (static_cast<double> (knobHz) * std::exp2 (static_cast<double> (envOctaves) * envCurve), 1.0, 0.45 * rate);
        const auto g = std::tan (std::numbers::pi * fc / rate);
        const auto qScale = std::pow (20.0, static_cast<double> (resonancePercent) / 100.0);
        const auto wet = section (section (x, g, 1.0 / 0.5412, st[0], st[1]), g, 1.0 / (1.3066 * qScale), st[2], st[3]);
        return static_cast<float> ((1.0 - m) * x + m * wet);
    }

private:
    // Zavalishin's TPT state-variable low-pass.
    static double section (double x, double g, double k, double& s1, double& s2)
    {
        const auto hp = (x - (k + g) * s1 - s2) / (1.0 + g * (g + k));
        const auto bp = g * hp + s1;
        const auto lp = g * bp + s2;
        s1 = g * hp + bp;
        s2 = g * bp + lp;
        return lp;
    }

    double rate = 44100.0;
    std::array<double, 4> st {};
};
}
