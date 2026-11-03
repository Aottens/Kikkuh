# Kikkuh by Colourmotion

A kick and rumble instrument plugin for techno, acid and trance: get from the kick in your head to
the kick in your track with knobs, like on a subtractive synth.

## Status
Very early. Kikkuh loads as an instrument and plays one fixed test kick on any MIDI note. It has no
knobs yet; the sound comes next. Plan in `BACKLOG.md`, handoff in `STATE.md`, scope in `specs/PRD.md`.

## What works today
- Builds as VST3, AU and CLAP and installs itself into `~/Library/Audio/Plug-Ins/`.
- A MIDI note plays a fixed test kick, sample-accurately, at zero latency.

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
