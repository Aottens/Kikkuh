# Kikkuh by Colourmotion

A kick and rumble instrument plugin for techno, acid and trance. From the kick in your head to the
kick in your track, fast, with knobs like a subtractive synth. Rumble is part of the instrument.
Reference products: Kick 3, Kick Ninja, Audija KickDrum, PunchBox 2, Kikzilla 2
(`docs/research/kick-plugins.md`).

## Owner
Aernout Ottens: product owner, reviewer, tests in Bitwig. Claude builds.

## Core value
The owner gets a usable approximation of a reference kick within 3 minutes, knobs only (CORE-01).
If everything else fails, this must work.

## Scope
- v1, later and out of scope: `specs/PRD.md`. Criteria in `specs/modules/`.
- Unplaced ideas: `IDEAS.md`.

## Constraints
- To be sold: VST3, AU, CLAP on macOS universal and Windows x64.
- Bitwig on Apple Silicon is the first host and the strict acceptance host.
- Zero reported latency. Under 3 % CPU per instance on an M-series Mac at 48 kHz / 256 samples.
- Offline: no telemetry, no required account.
- Closed source: no GPL or AGPL code linked in.

## Stack
C++20, JUCE 9 (commercial licence tier, see `DECISIONS.md`), clap-juce-extensions, Catch2, CMake +
Ninja, GitHub Actions.
