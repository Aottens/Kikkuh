# Kikkuh by Colourmotion

A kick and rumble instrument plugin for techno, acid and trance: get from the kick in your head to
the kick in your track with knobs, like on a subtractive synth.

## Status
Very early. Kikkuh plays a synthesized kick that you shape with wave, tune, pitch and amp knobs in the
host's generic editor. Filter, drive and the real panel come next. Plan in `BACKLOG.md`, handoff in
`STATE.md`, scope in `specs/PRD.md`.

## What works today
- Builds as VST3, AU and CLAP and installs itself into `~/Library/Audio/Plug-Ins/`.
- A MIDI note plays a kick, sample-accurately, at zero latency.
- Wave morphs from sine through triangle and saw to square, band-limited.
- Tune sets the note the kick lands on; Pitch Amount, Decay and Shape set the sweep down to it.
- Attack, Hold, Decay and Shape set the level.
- All knobs are automatable and smoothed, and they come back with your project.
- `kikkuh-render` writes a kick to a WAV without a DAW, for example
  `build/kikkuh-render --out kick.wav body_tune=F#0 body_pitch_amount=60`.
- `kikkuh-measure` prints the sweep, harmonics, first milliseconds and decay of a kick from a WAV or
  AIFF, and the nearest knob values: `build/kikkuh-measure kick.wav`.

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
