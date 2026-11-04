#pragma once

#include "dsp/BodyVoice.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <numbers>
#include <random>
#include <string>
#include <vector>

// HARN-08: the sketch voice, for the listening rounds. Plain C++, offline, nothing in source/ uses it.
// It is BODY plus what BODY cannot do yet; what wins a round is rebuilt under the audio-thread rules.
namespace sketch
{
enum class ClickKind { none, tone, noise };
enum class SatKind { none, soft, asym, hard };
enum class FilterKind { none, svf12, svf24, ladder };

// The defaults add nothing: a voice with these is BODY.
struct Params
{
    dsp::BodyParams body;
    double tuneCents = 0.0, sweep2Amount = 0.0, sweep2DecayMs = 100.0, sweep2Shape = 0.0, startPhase = 0.0;
    ClickKind clickKind = ClickKind::none;
    double clickHz = 3000.0, clickMs = 2.0, clickDb = -6.0;
    double dipAt = 0.0, dipWidth = 1.0, dipDb = 0.0, dip2At = 0.0, dip2Width = 1.0, dip2Db = 0.0;
    SatKind satKind = SatKind::none;
    double satDrive = 0.0;
    bool satBeforeAmp = false;
    FilterKind filterKind = FilterKind::none;
    double filterCutoff = 20000.0, filterRes = 0.0, filterEnv = 0.0, filterDecayMs = 200.0;
};

namespace detail
{
// The whole text must be a finite number.
inline bool parseNumber (const std::string& text, double& out)
{
    if (text.empty() || std::isspace (static_cast<unsigned char> (text[0])))
        return false;
    char* end = nullptr;
    out = std::strtod (text.c_str(), &end);
    return *end == '\0' && std::isfinite (out);
}

// "A0", "F#0", "Bb-1", with C3 = 60.
inline bool parseNote (const std::string& text, double& midi)
{
    static const std::string letters = "CDEFGAB";
    static constexpr int semitones[] = { 0, 2, 4, 5, 7, 9, 11 };
    if (text.empty())
        return false;
    const auto letter = letters.find (static_cast<char> (std::toupper (static_cast<unsigned char> (text[0]))));
    if (letter == std::string::npos)
        return false;
    auto note = semitones[letter];
    size_t at = 1;
    if (at < text.size() && (text[at] == '#' || text[at] == 'b'))
        note += text[at++] == '#' ? 1 : -1;
    double octave = 0.0;
    if (! parseNumber (text.substr (at), octave) || octave != std::floor (octave))
        return false;
    midi = (octave + 2.0) * 12.0 + note;
    return true;
}

struct Key
{
    const char* name;
    double lo, hi;
    float dsp::BodyParams::* body;
    double Params::* own;
};

inline const std::vector<Key>& keys()
{
    using B = dsp::BodyParams;
    static const std::vector<Key> table {
        { "body_wave", 0, 3, &B::wave, nullptr }, { "body_tune", 24, 60, &B::tune, nullptr },
        { "body_pitch_amount", 0, 72, &B::pitchAmount, nullptr }, { "body_pitch_decay", 1, 1000, &B::pitchDecayMs, nullptr },
        { "body_pitch_shape", -100, 100, &B::pitchShape, nullptr }, { "body_amp_attack", 0, 50, &B::attackMs, nullptr },
        { "body_amp_hold", 0, 500, &B::holdMs, nullptr }, { "body_amp_decay", 10, 2000, &B::ampDecayMs, nullptr },
        { "body_amp_shape", -100, 100, &B::ampShape, nullptr },
        { "tune_cents", -100, 100, nullptr, &Params::tuneCents }, { "sweep2_amount", 0, 24, nullptr, &Params::sweep2Amount },
        { "sweep2_decay", 1, 2000, nullptr, &Params::sweep2DecayMs }, { "sweep2_shape", -100, 100, nullptr, &Params::sweep2Shape },
        { "start_phase", 0, 90, nullptr, &Params::startPhase },
        { "click_hz", 500, 12000, nullptr, &Params::clickHz }, { "click_ms", 0.5, 20, nullptr, &Params::clickMs },
        { "click_db", -40, 6, nullptr, &Params::clickDb },
        // The spec gives the dents no ranges for position and width: these are wide enough for the first 200 ms.
        { "dip_at", 0, 200, nullptr, &Params::dipAt }, { "dip_width", 0.1, 100, nullptr, &Params::dipWidth },
        { "dip_db", -40, 0, nullptr, &Params::dipDb }, { "dip2_at", 0, 200, nullptr, &Params::dip2At },
        { "dip2_width", 0.1, 100, nullptr, &Params::dip2Width }, { "dip2_db", -40, 0, nullptr, &Params::dip2Db },
        { "sat_drive", 0, 36, nullptr, &Params::satDrive },
        { "filter_cutoff", 20, 20000, nullptr, &Params::filterCutoff }, { "filter_res", 0, 1, nullptr, &Params::filterRes },
        { "filter_env", 0, 8, nullptr, &Params::filterEnv }, { "filter_decay", 1, 2000, nullptr, &Params::filterDecayMs },
    };
    return table;
}
}

// False for an unknown key, a value that does not parse, or one out of range. Nothing changes then.
inline bool set (Params& p, const std::string& key, const std::string& value)
{
    if (key == "click_kind" && (value == "tone" || value == "noise"))
        return p.clickKind = value == "tone" ? ClickKind::tone : ClickKind::noise, true;
    if (key == "sat_kind" && (value == "soft" || value == "asym" || value == "hard"))
        return p.satKind = value == "soft" ? SatKind::soft : value == "asym" ? SatKind::asym : SatKind::hard, true;
    if (key == "sat_place" && (value == "after_amp" || value == "before_amp"))
        return p.satBeforeAmp = value == "before_amp", true;
    if (key == "filter_kind" && (value == "svf12" || value == "svf24" || value == "ladder"))
        return p.filterKind = value == "svf12" ? FilterKind::svf12 : value == "svf24" ? FilterKind::svf24 : FilterKind::ladder, true;

    for (const auto& k : detail::keys())
    {
        if (key != k.name)
            continue;
        double number = 0.0;
        if (! (key == "body_tune" && detail::parseNote (value, number)) && ! detail::parseNumber (value, number))
            return false;
        if (number < k.lo || number > k.hi)
            return false;
        if (k.body != nullptr)
            p.body.*k.body = static_cast<float> (number);
        else
            p.*k.own = number;
        return true;
    }
    return false;
}

namespace detail
{
// tanh, a clip, or tanh around a bias with its DC taken out by a one-pole high-pass at 10 Hz.
struct Saturator
{
    Saturator (SatKind newKind, double drive, double rate) : kind (newKind), gain (std::pow (10.0, drive / 20.0)), blocker (1.0 - 2.0 * std::numbers::pi * 10.0 / rate) {}

    double process (double x)
    {
        switch (kind)
        {
            case SatKind::soft: return std::tanh (gain * x);
            case SatKind::hard: return std::clamp (gain * x, -1.0, 1.0);
            case SatKind::asym:
            {
                const auto v = std::tanh (gain * x + bias) - std::tanh (bias);
                lastOut = v - lastIn + blocker * lastOut;
                lastIn = v;
                return lastOut;
            }
            case SatKind::none: break;
        }
        return x;
    }

    SatKind kind;
    double gain, blocker, lastIn = 0.0, lastOut = 0.0;
    static constexpr double bias = 0.5;
};

// Zavalishin's TPT state-variable low-pass.
inline double svf (double x, double g, double k, double& s1, double& s2)
{
    const auto hp = (x - (k + g) * s1 - s2) / (1.0 + g * (g + k));
    const auto bp = g * hp + s1;
    const auto lp = g * bp + s2;
    s1 = g * hp + bp;
    s2 = g * bp + lp;
    return lp;
}

struct Filter
{
    Filter (FilterKind newKind, double newRate) : kind (newKind), rate (newRate) {}

    double process (double x, double cutoff, double res)
    {
        const auto fc = std::clamp (cutoff, 1.0, 0.45 * rate);
        const auto g = std::tan (std::numbers::pi * fc / rate);
        const auto qScale = std::pow (20.0, res);
        switch (kind)
        {
            case FilterKind::svf12: return svf (x, g, 1.0 / (0.7071 * qScale), st[0], st[1]);
            case FilterKind::svf24: return svf (svf (x, g, 1.0 / 0.5412, st[0], st[1]), g, 1.0 / (1.3066 * qScale), st[2], st[3]);
            case FilterKind::ladder:
            {
                // Four one-poles with tanh in the stages and the feedback; the input is scaled so the passband stays near 1.
                const auto a = 1.0 - std::exp (-2.0 * std::numbers::pi * fc / rate);
                const auto k = 4.0 * res;
                auto in = std::tanh (x * (1.0 + k) - k * st[3]);
                for (auto& s : st)
                {
                    s += a * (in - std::tanh (s));
                    in = std::tanh (s);
                }
                return st[3];
            }
            case FilterKind::none: break;
        }
        return x;
    }

    FilterKind kind;
    double rate;
    std::array<double, 4> st {};
};

// Kaiser-windowed sinc low-pass, odd number of taps, cutoff in cycles per sample, unit gain at DC.
inline std::vector<double> lowpass (int taps, double cutoff, double beta)
{
    const auto bessel = [] (double v) {
        double sum = 1.0, term = 1.0;
        for (int k = 1; k < 60; ++k)
        {
            term *= (v / (2.0 * k)) * (v / (2.0 * k));
            sum += term;
        }
        return sum;
    };
    const auto centre = taps / 2;
    std::vector<double> h (static_cast<size_t> (taps));
    double total = 0.0;
    for (int k = 0; k < taps; ++k)
    {
        const auto n = static_cast<double> (k - centre), r = n / centre;
        const auto sinc = n == 0.0 ? 2.0 * cutoff : std::sin (2.0 * std::numbers::pi * cutoff * n) / (std::numbers::pi * n);
        h[static_cast<size_t> (k)] = sinc * bessel (beta * std::sqrt (1.0 - r * r)) / bessel (beta);
        total += h[static_cast<size_t> (k)];
    }
    for (auto& v : h)
        v /= total;
    return h;
}

// Output j is the zero-phase filter centred on input sample factor * j; outside the input is zero.
inline std::vector<float> firDecimate (const std::vector<float>& x, const std::vector<double>& h, long factor)
{
    const auto taps = static_cast<long> (h.size()), centre = taps / 2, size = static_cast<long> (x.size());
    std::vector<float> y (x.size() / static_cast<size_t> (factor));
    for (size_t j = 0; j < y.size(); ++j)
    {
        double sum = 0.0;
        const auto middle = factor * static_cast<long> (j);
        for (long k = std::max (0L, middle + centre - size + 1); k < taps && k <= middle + centre; ++k)
            sum += h[static_cast<size_t> (k)] * static_cast<double> (x[static_cast<size_t> (middle + centre - k)]);
        y[j] = static_cast<float> (sum);
    }
    return y;
}

// Saturation at 4x the running rate, so its harmonics fold back far less (HARN-08f): zero-phase interpolation
// with a 129-tap filter, the curve, the same filter down. What lies between 24 kHz and half the running rate
// is left to the final decimation.
// ponytail: sat_place=before_amp saturates at the running rate instead; a low sine leaves little to fold back.
inline std::vector<float> saturateOversampled (const Params& p, const std::vector<float>& x, double rate)
{
    static const auto h = lowpass (129, 96000.0 / 768000.0, 10.0);
    const auto taps = static_cast<long> (h.size()), centre = taps / 2, size = static_cast<long> (x.size());
    std::vector<float> up (x.size() * 4);
    Saturator sat (p.satKind, p.satDrive, 4.0 * rate);
    for (long m = 0; m < static_cast<long> (up.size()); ++m)
    {
        double sum = 0.0;
        for (auto i = std::max (0L, (m + centre - taps + 1 + 3) / 4); i < size && 4 * i <= m + centre; ++i)
            sum += 4.0 * h[static_cast<size_t> (m + centre - 4 * i)] * static_cast<double> (x[static_cast<size_t> (i)]);
        up[static_cast<size_t> (m)] = static_cast<float> (sat.process (sum));
    }
    return firDecimate (up, h, 4);
}

// Saturation (when asked) and the filter, on a signal at `rate`, time zero at its first sample.
inline std::vector<float> shapeStage (const Params& p, const std::vector<float>& x, double rate, bool saturate)
{
    const auto saturated = saturate && p.satKind != SatKind::none ? saturateOversampled (p, x, rate) : x;
    Filter filter (p.filterKind, rate);
    std::vector<float> y (x.size());
    for (size_t i = 0; i < x.size(); ++i)
    {
        auto v = static_cast<double> (saturated[i]);
        if (p.filterKind != FilterKind::none)
        {
            const auto progress = std::min (1.0, 1000.0 * static_cast<double> (i) / rate / p.filterDecayMs);
            const auto cutoff = p.filterCutoff * std::pow (2.0, p.filterEnv * dsp::bodyCurve (progress, 0.0));
            v = filter.process (v, cutoff, p.filterRes);
        }
        y[i] = static_cast<float> (v);
    }
    return y;
}

inline double dentFactor (double ms, double at, double width, double db)
{
    if (db == 0.0 || std::abs (ms - at) >= 0.5 * width)
        return 1.0;
    return 1.0 - (1.0 - std::pow (10.0, db / 20.0)) * (0.5 + 0.5 * std::cos (2.0 * std::numbers::pi * (ms - at) / width));
}
}

// The saturation and filter stage on a signal at 192 kHz (what the render does at that rate).
inline std::vector<float> shape (const Params& p, const std::vector<float>& x)
{
    return detail::shapeStage (p, x, 192000.0, ! p.satBeforeAmp);
}

// The voice alone at `rate`, through the whole signal order. The envelopes mirror dsp::BodyVoice.
inline std::vector<float> renderRaw (const Params& p, double rate, double seconds)
{
    const auto &wave = p.body.wave, &tune = p.body.tune, &amount = p.body.pitchAmount, &pitchMs = p.body.pitchDecayMs,
               &pitchShape = p.body.pitchShape, &attackMs = p.body.attackMs, &holdMs = p.body.holdMs, &decayMs = p.body.ampDecayMs,
               &ampShape = p.body.ampShape;
    std::vector<float> y (static_cast<size_t> (std::llround (seconds * rate)));
    const auto dt = 1.0 / rate;

    dsp::MorphOsc osc;
    // phase += hz / rate, so one discarded call at hz = startPhase * rate moves the phase by startPhase (a fraction of a cycle).
    if (p.startPhase > 0.0)
        osc.next (p.startPhase / 360.0 * rate, rate, wave);

    detail::Saturator before (p.satBeforeAmp ? p.satKind : SatKind::none, p.satDrive, rate);
    std::mt19937 noise (12345u);
    enum class Stage { attack, hold, decay };
    auto stage = Stage::attack;
    bool active = true;
    double pitchProgress = 0.0, pitch2Progress = 0.0, ampProgress = 0.0;
    const auto enter = [&] (Stage next) {
        stage = next;
        ampProgress = 0.0;
    };

    for (size_t i = 0; i < y.size(); ++i)
    {
        const auto t = static_cast<double> (i) * dt, ms = 1000.0 * t;
        float voice = 0.0f;
        if (active)
        {
            auto midi = tune + amount * dsp::bodyCurve (pitchProgress, pitchShape);
            midi += p.tuneCents / 100.0 + p.sweep2Amount * dsp::bodyCurve (pitch2Progress, p.sweep2Shape);
            pitchProgress = std::min (1.0, pitchProgress + dt / (0.001 * std::max (pitchMs, 1.0f)));
            pitch2Progress = std::min (1.0, pitch2Progress + dt / (0.001 * std::max (p.sweep2DecayMs, 1.0)));

            if (stage == Stage::attack && attackMs <= 0.0f)
                enter (Stage::hold);
            if (stage == Stage::hold && holdMs <= 0.0f)
                enter (Stage::decay);

            double level = 1.0;
            if (stage == Stage::attack)
            {
                level = ampProgress;
                ampProgress += dt / (0.001 * attackMs);
                if (ampProgress >= 1.0)
                    enter (Stage::hold);
            }
            else if (stage == Stage::hold)
            {
                ampProgress += dt / (0.001 * holdMs);
                if (ampProgress >= 1.0)
                    enter (Stage::decay);
            }
            else
            {
                level = dsp::bodyCurve (ampProgress, ampShape);
                ampProgress += dt / (0.001 * std::max (decayMs, 1.0f));
                if (ampProgress >= 1.0)
                    active = false;
            }

            auto o = osc.next (440.0 * std::pow (2.0, (midi - 69.0) / 12.0), rate, wave);
            if (p.satBeforeAmp)
                o = static_cast<float> (before.process (static_cast<double> (o)));
            voice = o * static_cast<float> (level);
        }

        auto d = static_cast<double> (voice);
        d *= detail::dentFactor (ms, p.dipAt, p.dipWidth, p.dipDb) * detail::dentFactor (ms, p.dip2At, p.dip2Width, p.dip2Db);
        if (p.clickKind != ClickKind::none && ms < p.clickMs)
        {
            // Falls to silence at click_ms, so it ends without a step.
            const auto envelope = std::pow (10.0, p.clickDb / 20.0) * std::pow (1.0 - ms / p.clickMs, 2.0);
            // Hand-written uniform mapping: std::uniform_real_distribution differs between standard libraries.
            const auto white = static_cast<double> (noise() & 0xFFFFFFu) / 8388608.0 - 1.0;
            d += envelope * (p.clickKind == ClickKind::tone ? std::sin (2.0 * std::numbers::pi * p.clickHz * t) : white);
        }
        y[i] = static_cast<float> (d);
    }
    return detail::shapeStage (p, y, rate, ! p.satBeforeAmp);
}

// 4:1, linear phase (Kaiser-windowed sinc, 511 taps, passband to ~21 kHz, stopband from ~24 kHz), with its
// delay removed, so time zero stays on the first sample. Before the start counts as zero.
inline std::vector<float> decimate (const std::vector<float>& x)
{
    static const auto h = detail::lowpass (511, 22500.0 / 192000.0, 11.0);
    return detail::firDecimate (x, h, 4);
}

// The voice at 192 kHz, decimated: `seconds` of audio at 48 kHz, time zero on the first sample.
inline std::vector<float> render48k (const Params& p, double seconds)
{
    return decimate (renderRaw (p, 192000.0, static_cast<double> (std::llround (seconds * 48000.0)) / 48000.0));
}
}
