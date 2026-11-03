#pragma once

#include "MorphOsc.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <string>

namespace dsp
{
// The nine BODY knobs in the units of specs/modules/body.md. Tune is a MIDI note, C3 = 60.
struct BodyParams
{
    float wave = 0.0f, tune = 33.0f, pitchAmount = 36.0f, pitchDecayMs = 100.0f, pitchShape = 0.0f,
          attackMs = 0.0f, holdMs = 0.0f, ampDecayMs = 400.0f, ampShape = 0.0f;
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
    if (t >= p.pitchDecayMs)
        return p.tune;
    return p.tune + p.pitchAmount * bodyCurve (t / p.pitchDecayMs, p.pitchShape);
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
        targets = { p.wave, p.tune, p.pitchAmount, p.pitchDecayMs, p.pitchShape, p.attackMs, p.holdMs, p.ampDecayMs, p.ampShape };
    }

    // Snaps the smoothers to the targets: hosts and tests set parameters before this.
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        smoothing = 1.0 - std::exp (-1.0 / (0.01 * sampleRate));
        values = targets;
        active = false;
    }

    // ponytail: a hard restart clicks when a kick is still ringing; PLAY-03 replaces this.
    void trigger (float newVelocity)
    {
        if (! active)
            values = targets; // next() skips smoothing while idle, so the knobs are stale
        velocity = newVelocity;
        osc.reset();
        pitchProgress = 0.0;
        ampProgress = 0.0;
        stage = Stage::attack;
        active = true;
    }

    bool isActive() const { return active; }

    float next()
    {
        if (! active)
            return 0.0f;

        for (size_t i = 0; i < values.size(); ++i)
            values[i] += static_cast<float> (smoothing) * (targets[i] - values[i]);
        const auto [wave, tune, amount, pitchMs, pitchShape, attackMs, holdMs, decayMs, ampShape] = values;

        // Progress integrates dt / length, so a time knob changes the rate and never jumps the level.
        const auto dt = 1.0 / sampleRate;
        const auto midi = tune + amount * bodyCurve (pitchProgress, pitchShape);
        pitchProgress = std::min (1.0, pitchProgress + dt / (0.001 * std::max (pitchMs, 1.0f)));

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
            ampProgress += dt / (0.001 * std::max (decayMs, 1.0f));
            if (ampProgress >= 1.0)
                active = false;
        }

        const auto hz = 440.0 * std::pow (2.0, (midi - 69.0) / 12.0);
        return osc.next (hz, sampleRate, wave) * static_cast<float> (level) * velocity;
    }

private:
    enum class Stage { attack, hold, decay };

    void enter (Stage newStage)
    {
        stage = newStage;
        ampProgress = 0.0;
    }

    MorphOsc osc;
    std::array<float, 9> targets {}, values {};
    double sampleRate = 44100.0, smoothing = 0.0, pitchProgress = 0.0, ampProgress = 0.0;
    float velocity = 0.0f;
    Stage stage = Stage::attack;
    bool active = false;
};
}
