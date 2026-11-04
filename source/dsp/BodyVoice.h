#pragma once

#include "BodyDrive.h"
#include "MorphOsc.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <iterator>
#include <numbers>
#include <string>

namespace dsp
{
// The BODY knobs in the units of specs/modules/body.md. Tune is a MIDI note, C3 = 60.
// The member defaults are the sound before BODY-08 (tools/Sketch.h takes them as "today"); the plugin's own defaults live in createLayout.
struct BodyParams
{
    float wave = 0.0f, tune = 33.0f, pitchAmount = 36.0f, pitchDecayMs = 100.0f, pitchShape = 0.0f,
          attackMs = 0.0f, holdMs = 0.0f, ampDecayMs = 400.0f, ampShape = 0.0f,
          driveDb = 0.0f, filterCutoffHz = 20000.0f, filterResPercent = 0.0f, filterEnvOctaves = 0.0f,
          sweep2Amount = 0.0f, sweep2DecayMs = 300.0f, startDegrees = 0.0f,
          tailAnchor = 0.0f; // BODY-07: off here, so the member defaults stay the sound before the anchor
};

// Falls from 1 to exactly 0 over x = 0 ... 1; shape is in percent.
inline double bodyCurve (double x, double shape)
{
    return std::pow (1.0 - x, std::pow (3.0, 1.0 + shape / 100.0));
}

// BODY-02d: the note the tail lands on, at the moment the amp envelope has fallen to -40 dB.
inline double bodyTailNote (const BodyParams& p)
{
    const auto k = std::pow (3.0, 1.0 + p.ampShape / 100.0);
    const auto t = p.attackMs + p.holdMs + p.ampDecayMs * (1.0 - std::pow (0.01, 1.0 / k));
    // A curve past its end is 0, not NaN: clamp x to 1.
    return p.tune + p.pitchAmount * bodyCurve (std::min (1.0, t / p.pitchDecayMs), p.pitchShape)
           + p.sweep2Amount * bodyCurve (std::min (1.0, t / p.sweep2DecayMs), 0.0);
}

// BODY-07: the window the bend lives in, in seconds: the later of Pitch Decay and 50 ms, or with Pitch Amount 0 of Sweep 2 Decay and 50 ms.
inline double bodyAnchorWindow (const BodyParams& p)
{
    return 0.001 * std::max (p.pitchAmount > 0.0f ? p.pitchDecayMs : p.sweep2DecayMs, 50.0f);
}

// BODY-07e: `e`, so that the pitch times (1 + e * (1 - t/W)) lands the oscillator on Start + Tune Hz * t whatever the sweeps did.
// The voice adds samples rather than integrating, so the integrals get the Euler-Maclaurin terms of that sum
// (ponytail: a Shape near -100 has an end-point singularity that costs the integral some accuracy).
inline double bodyAnchorBend (const BodyParams& p, double sampleRate)
{
    const auto lengthOne = 0.001 * std::max (p.pitchDecayMs, 1.0f), lengthTwo = 0.001 * std::max (p.sweep2DecayMs, 1.0f);
    const auto hzAt = [&] (double t) {
        const auto midi = p.tune + p.pitchAmount * bodyCurve (std::min (1.0, t / lengthOne), p.pitchShape)
                          + p.sweep2Amount * bodyCurve (std::min (1.0, t / lengthTwo), 0.0);
        return 440.0 * std::pow (2.0, (midi - 69.0) / 12.0);
    };
    const auto tuneHz = hzAt (1.0e9);
    const auto window = bodyAnchorWindow (p);
    const auto end = std::max ({ window, p.pitchAmount > 0.0f ? lengthOne : 0.0, p.sweep2Amount > 0.0f ? lengthTwo : 0.0 });

    // Composite 5-point Gauss-Legendre (32 pieces, 160 evaluations per segment), split where the curves have a corner.
    double cuts[5] = { 0.0, window, end, lengthOne, lengthTwo };
    std::sort (std::begin (cuts), std::end (cuts));
    const auto integrate = [&] (auto&& g, double a, double b) {
        constexpr int pieces = 32;
        constexpr double nodes[3] = { 0.0, 0.5384693101056831, 0.9061798459386640 }, weights[3] = { 0.5688888888888889, 0.4786286704993665, 0.2369268850561891 };
        const auto h = (b - a) / pieces;
        double sum = 0.0;
        for (int i = 0; i < pieces; ++i)
        {
            const auto mid = a + (i + 0.5) * h;
            sum += weights[0] * g (mid);
            for (int j = 1; j < 3; ++j)
                sum += weights[j] * (g (mid - 0.5 * h * nodes[j]) + g (mid + 0.5 * h * nodes[j]));
        }
        return sum * 0.5 * h;
    };
    double excess = 0.0, weight = 0.0;
    for (int i = 0; i < 4; ++i)
    {
        const auto a = cuts[i], b = std::min (cuts[i + 1], end);
        if (a >= end || b <= a)
            continue;
        excess += integrate ([&] (double t) { return hzAt (t) - tuneHz; }, a, b);
        if (a < window)
            weight += integrate ([&] (double t) { return hzAt (t) * (1.0 - t / window); }, a, std::min (b, window));
    }

    // The end-point terms of the sum over samples: (h(0) - h(T)) dt / 2 + (h'(T) - h'(0)) dt^2 / 12, h' at 0 from the curves' slopes there.
    const auto dt = 1.0 / sampleRate, start = hzAt (0.0);
    const auto slope = start * std::numbers::ln2 / 12.0
                       * (-p.pitchAmount * std::pow (3.0, 1.0 + p.pitchShape / 100.0) / lengthOne - p.sweep2Amount * 3.0 / lengthTwo);
    excess += (start - tuneHz) * dt / 2.0 - slope * dt * dt / 12.0;
    weight += start * dt / 2.0 + (-hzAt (window) / window - (slope - start / window)) * dt * dt / 12.0;

    const auto lost = -excess;
    return (lost - std::round (lost)) / weight;
}

// "A0", "F#0", with C3 = 60.
inline std::string bodyNoteName (int midi)
{
    static constexpr const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const auto octave = (midi >= 0 ? midi / 12 : -((11 - midi) / 12)) - 2;
    return std::string (names[((midi % 12) + 12) % 12]) + std::to_string (octave);
}

// "C#1 +23 ct": the nearest note, with cents when it is more than one off.
inline std::string bodyNoteReadout (double midi)
{
    const auto nearest = static_cast<int> (std::lround (midi));
    const auto cents = (midi - nearest) * 100.0;
    auto text = bodyNoteName (nearest);
    if (std::abs (cents) > 1.0)
    {
        char buffer[24];
        std::snprintf (buffer, sizeof buffer, " %+d ct", static_cast<int> (std::lround (cents)));
        text += buffer;
    }
    return text;
}

class BodyVoice
{
public:
    void setParams (const BodyParams& p)
    {
        // The cutoff is smoothed in octaves: a jump across ten octaves would otherwise open in the first millisecond.
        targets = { p.wave, p.tune, p.pitchAmount, p.pitchDecayMs, p.pitchShape, p.attackMs, p.holdMs, p.ampDecayMs, p.ampShape,
                    p.driveDb, std::log2 (std::max (p.filterCutoffHz, 1.0f)), p.filterResPercent, p.filterEnvOctaves, p.sweep2Amount, p.sweep2DecayMs };
        startPhase = p.startDegrees / 360.0; // not smoothed: it only matters at the note-on
        anchorOn = p.tailAnchor > 0.5f;      // BODY-07: read at the note-on as well
    }

    // Snaps the smoothers to the targets: hosts and tests set parameters before this.
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        smoothing = 1.0 - std::exp (-1.0 / (0.01 * sampleRate));
        values = targets;
        active = false;
        tailLeft = 0;
        drive.reset();
        filter.prepare (newSampleRate);
        residualDecay = static_cast<float> (std::exp (-1.0 / (residualSeconds * sampleRate)));
        residual = envResidual = lastDry = lastEnv = 0.0f;
    }

    // `level` is the note's whole gain (PLAY-01); `keyedNote` >= 0 replaces Tune for this note (PLAY-02).
    // PLAY-03: the last value of the kick it replaces fades away on top of the new note, in `dry` and in the filter envelope.
    void trigger (float level, int keyedNote = -1)
    {
        if (! active)
            values = targets; // next() skips smoothing while idle, so the knobs are stale
        residual = lastDry;
        envResidual = active ? lastEnv - 1.0f : 0.0f; // a new note starts its envelope at 1; from silence it opens at once
        velocity = level;
        keyNote = keyedNote;
        osc.reset (startPhase);
        anchored = anchorOn;
        clockPart = false;
        samples = 0;
        if (anchored)
        {
            BodyParams p;
            p.tune = keyNote >= 0 ? static_cast<float> (keyNote) : targets[1];
            p.pitchAmount = targets[2];
            p.pitchDecayMs = targets[3];
            p.pitchShape = targets[4];
            p.sweep2Amount = targets[13];
            p.sweep2DecayMs = targets[14];
            anchorBend = bodyAnchorBend (p, sampleRate);
            anchorWindow = bodyAnchorWindow (p);
        }
        pitchProgress = 0.0;
        pitch2Progress = 0.0;
        ampProgress = 0.0;
        stage = Stage::attack;
        active = true;
        tailLeft = 0;
    }

    bool isActive() const { return active; }

    float next()
    {
        if (! active && tailLeft <= 0)
            return 0.0f;

        for (size_t i = 0; i < values.size(); ++i)
            values[i] += static_cast<float> (smoothing) * (targets[i] - values[i]);
        const auto wave = values[0], tune = keyNote >= 0 ? static_cast<float> (keyNote) : values[1], amount = values[2], pitchMs = values[3], pitchShape = values[4],
                   attackMs = values[5], holdMs = values[6], decayMs = values[7], ampShape = values[8];

        float dry = 0.0f;
        double envCurve = 0.0; // 1 while the amp rises or holds, then the amp's own fall, 0 after the end
        if (active)
        {
            // Progress integrates dt / length, so a time knob changes the rate and never jumps the level.
            const auto dt = 1.0 / sampleRate;
            const auto midi = tune + amount * bodyCurve (pitchProgress, pitchShape) + values[13] * bodyCurve (pitch2Progress, 0.0);
            pitchProgress = std::min (1.0, pitchProgress + dt / (0.001 * std::max (pitchMs, 1.0f)));
            pitch2Progress = std::min (1.0, pitch2Progress + dt / (0.001 * std::max (values[14], 1.0f)));

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
                level = bodyCurve (ampProgress, ampShape);
                envCurve = level;
                ampProgress += dt / (0.001 * std::max (decayMs, 1.0f));
                if (ampProgress >= 1.0)
                {
                    active = false;
                    tailLeft = static_cast<long> (tailSeconds * sampleRate);
                }
            }
            if (stage != Stage::decay)
                envCurve = 1.0;

            auto hz = 440.0 * std::pow (2.0, (midi - 69.0) / 12.0);
            if (anchored)
                hz = anchorHz (hz, tune, dt, amount, values[13]);
            dry = osc.next (hz, sampleRate, wave) * static_cast<float> (level) * velocity;
        }
        else
            --tailLeft; // drive and filter ring out on silence

        dry += residual;
        envCurve += envResidual;
        lastDry = dry;
        lastEnv = static_cast<float> (envCurve);
        residual *= residualDecay;
        envResidual *= residualDecay;
        if (std::abs (residual) < 1e-7f)
            residual = 0.0f;
        if (std::abs (envResidual) < 1e-7f)
            envResidual = 0.0f;

        // ponytail: the tail runs a fixed 5 s after the kick; a resonant filter with a long ring needs a level gate.
        return filter.process (drive.process (dry, values[9]), std::exp2 (values[10]), values[11], values[12], static_cast<float> (envCurve));
    }

private:
    // BODY-07: the bent sweep, then the clock. From the handover the phase is Start + Tune Hz * t, not integrated, so two kicks that
    // differ only in their sweeps are the same floats. What the integrated phase misses falls to 0 over 50 ms (BODY-07b, j).
    double anchorHz (double sweepHz, float tune, double dt, float amount, float amountTwo)
    {
        const auto seconds = static_cast<double> (samples++) * dt;
        if (! clockPart)
        {
            if (seconds < anchorWindow || (pitch2Progress < 1.0 && amountTwo > 0.0f) || (pitchProgress < 1.0 && amount > 0.0f))
                return sweepHz * (1.0 + anchorBend * std::max (0.0, 1.0 - seconds / anchorWindow));
            clockPart = true;
            clockHz = 440.0 * std::pow (2.0, (tune - 69.0) / 12.0);
            clockOffset = 0.0;
            const auto miss = osc.getPhase() - (startPhase + clockHz * static_cast<double> (samples - 1) / sampleRate);
            missed = miss - std::round (miss);
            slideLeft = slideLength = static_cast<long> (std::ceil (slideSeconds * sampleRate));
        }
        const auto hz = 440.0 * std::pow (2.0, (tune - 69.0) / 12.0);
        const auto n = static_cast<double> (samples - 1);
        if (std::abs (hz - clockHz) > 0.0)
        {
            clockOffset += (clockHz - hz) * n / sampleRate; // a Tune move keeps the phase continuous
            clockHz = hz;
        }
        const auto slide = slideLeft > 0 ? missed * static_cast<double> (slideLeft--) / static_cast<double> (slideLength) : 0.0;
        const auto phase = startPhase + hz * n / sampleRate + clockOffset + slide;
        osc.reset (phase - std::floor (phase));
        return hz;
    }

    enum class Stage { attack, hold, decay };

    void enter (Stage newStage)
    {
        stage = newStage;
        ampProgress = 0.0;
    }

    static constexpr double tailSeconds = 5.0, residualSeconds = 0.003, slideSeconds = 0.05; // PLAY-03: the retrigger fade; BODY-07: the slide to the clock

    MorphOsc osc;
    BodyDrive drive;
    BodyFilter filter;
    long tailLeft = 0;
    std::array<float, 15> targets {}, values {};
    double sampleRate = 44100.0, smoothing = 0.0, pitchProgress = 0.0, pitch2Progress = 0.0, ampProgress = 0.0, startPhase = 0.0;
    double anchorBend = 0.0, anchorWindow = 0.0, clockHz = 0.0, clockOffset = 0.0, missed = 0.0;
    long samples = 0, slideLeft = 0, slideLength = 1;
    bool anchorOn = false, anchored = false, clockPart = false;
    float velocity = 0.0f, residual = 0.0f, envResidual = 0.0f, residualDecay = 0.0f, lastDry = 0.0f, lastEnv = 0.0f;
    int keyNote = -1;
    Stage stage = Stage::attack;
    bool active = false;
};
}
