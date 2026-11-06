# Kikkuh by Colourmotion

A kick and rumble instrument plugin for techno, acid and trance: get from the kick in your head to
the kick in your track with knobs, like on a subtractive synth.

## Status
Very early. Kikkuh plays a synthesized kick that you shape on one printed panel of knobs, in the order of the signal. You can
hear it without MIDI, with a hit pad and a loop in your host's tempo. Its defaults are the sound chosen by ear in blind rounds.
Click and Top layer a tone or noise over it; your own samples, Master and Rumble come next.
Plan in `BACKLOG.md`, handoff in `STATE.md`, scope in `specs/PRD.md`.

## What works today
- Builds as VST3, AU and CLAP and installs itself into `~/Library/Audio/Plug-Ins/`.
- One panel on one screen, resizable from 80 to 200 %: drag a knob, Shift for fine, double-click or Cmd-click for its default, click its value to type (`F#0`, `1k`).
- The hit pad plays the kick, and Loop plays it on quarter notes: on your host's grid while the transport runs, free at its tempo while it stands still. Neither ends up in a bounce or in your project.
- The panel shows the note the tail actually lands on.
- A MIDI note plays a kick, sample-accurately, at zero latency. A new note cuts off the one that rings without a click.
- Velocity sets how much the note's velocity changes its level; Key Track makes the kick play the note you play.
- Wave morphs from sine through triangle and saw to square, band-limited.
- Tune sets the note the kick lands on; Pitch Amount, Decay and Shape set the sweep down to it, and Sweep 2 Amount and
  Decay add a slower second stage.
- Start sets the phase at the note-on: at 90 degrees the kick starts at full level.
- Tail Anchor, on by default, keeps the tail in place while you turn the pitch knobs: the sweep changes and what follows it does not.
- Attack, Hold, Decay and Shape set the level.
- Drive clips the kick hard after the amp, oversampled and still at zero reported latency.
- A 24 dB low-pass with Cutoff, Resonance and Env Amount follows the drive.
- Click and Top, two equal sections below the body, layer a tone or noise over the kick, each with Level, Decay, Tune,
  High-pass and Low-pass. Both start Off, so the kick and your older projects sound as before.
- All knobs are automatable and smoothed, and they come back with your project; a project saved before the new
  defaults keeps its sound.
- `kikkuh-render` writes a kick to a WAV without a DAW, for example
  `build/kikkuh-render --out kick.wav body_tune=F#0 body_pitch_amount=60`.
- `kikkuh-measure` prints the sweep, harmonics, first milliseconds and decay of a kick from a WAV or
  AIFF, and the nearest knob values: `build/kikkuh-measure kick.wav`.
- `kikkuh-round` renders a round of variants as unnamed, equally loud WAVs for choosing by ear:
  `build/kikkuh-round tests/manual/rounds/8-check.txt --out rounds/8 --seed 108`.

## Build and run

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build && ctest --test-dir build
```

```bash
scripts/validate.sh
```

Rescan plugins in Bitwig to pick up the new build.

## How work happens
Spec → test → code → review, one build per PR; see `CLAUDE.md`.

## Licence
All rights reserved. See `LICENSE`.
