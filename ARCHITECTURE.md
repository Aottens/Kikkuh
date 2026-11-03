# Kikkuh — Architecture

## Data flow

```
 host                                   Kikkuh
 ────                                   ──────
 MIDI, at sample offsets ─────► PluginProcessor::processBlock ──► stereo out
 play head (PPQ, bpm) ────────►   renders between MIDI events
 parameter changes ──► atomics ─►  dsp voice
```

Planned signal flow, from `specs/PRD.md`: BODY, CLICK and TOP in parallel feed MASTER, and the kick
feeds RUMBLE, whose output joins before MASTER. The bus layout is decided before release (X-01).

## Layers
- `source/dsp/`: plain C++, no JUCE, no I/O. Deterministic; tested with `tests/Render.h` through the
  processor, or directly. `BodyVoice.h` is the kick's voice, `MorphOsc.h` its band-limited oscillator.
- `source/PluginProcessor.*`: JUCE glue. Parameters, state, MIDI and play-head reading, calls into
  `dsp/`. Nothing here decides how a kick sounds.
- `source/ui/` (from the first panel task): the editor. Reads state and sends parameter changes; no DSP.

## Threads
- Audio thread: `processBlock` only; the rules in `CLAUDE.md` apply.
- Message thread: editor and state load/save. Talks to the audio thread through parameter atomics
  and lock-free snapshots.

## Dependencies
Linked: JUCE 9.0.5 (Starter tier, see `DECISIONS.md`), clap-juce-extensions (MIT, pinned SHA).
Tests only: Catch2 3.8.1 (BSL-1.0). Tools that run against the binary and are not linked:
pluginval (GPLv3), clap-validator (MIT), auval (Apple).
