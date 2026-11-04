#pragma once

#include "BodyDrive.h"
#include "MorphOsc.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
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
          sweep2Amount = 0.0f, sweep2DecayMs = 300.0f, startDegrees = 0.0f;
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

            const auto hz = 440.0 * std::pow (2.0, (midi - 69.0) / 12.0);
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
    enum class Stage { attack, hold, decay };

    void enter (Stage newStage)
    {
        stage = newStage;
        ampProgress = 0.0;
    }

    static constexpr double tailSeconds = 5.0, residualSeconds = 0.003; // PLAY-03: the retrigger fade

    MorphOsc osc;
    BodyDrive drive;
    BodyFilter filter;
    long tailLeft = 0;
    std::array<float, 15> targets {}, values {};
    double sampleRate = 44100.0, smoothing = 0.0, pitchProgress = 0.0, pitch2Progress = 0.0, ampProgress = 0.0, startPhase = 0.0;
    float velocity = 0.0f, residual = 0.0f, envResidual = 0.0f, residualDecay = 0.0f, lastDry = 0.0f, lastEnv = 0.0f;
    int keyNote = -1;
    Stage stage = Stage::attack;
    bool active = false;
};
}
