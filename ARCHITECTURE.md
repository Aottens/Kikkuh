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
  processor, or directly. `BodyVoice.h` is the kick's voice, `MorphOsc.h` its band-limited oscillator, `BodyDrive.h` its clip and filter. `AuditionLoop.h` times the audition loop.
- `source/PluginProcessor.*`: JUCE glue. Parameters, state, MIDI and play-head reading, calls into
  `dsp/`. Nothing here decides how a kick sounds.
- `tools/`: `kikkuh-render` (HARN-04), a development tool that plays a note through the processor
  with `tests/Render.h` and writes a WAV. `kikkuh-measure` (HARN-05) prints the numbers of `tests/Measure.h`
  for a WAV or AIFF. `kikkuh-round` (HARN-07) turns a round file into blind, level-matched WAVs with the
  sketch voice `tools/Sketch.h` (HARN-08), plain C++ on `dsp::MorphOsc` for listening only. None is shipped.
- `source/ui/`: the panel. `PanelEditor` scales one fixed stage of 1154 x 318 points; `Knob` and `PanelLookAndFeel` draw it, and `PanelColours.h` holds its colours. Reads state, sends parameter changes and sets the pad and loop atomics; no DSP.
- `assets/fonts/`: four cuts of Archivo, compiled in as binary data, with their licence.

## Threads
- Audio thread: `processBlock` only; the rules in `CLAUDE.md` apply.
- Message thread: editor and state load/save. Talks to the audio thread through parameter atomics
  and lock-free snapshots.

## Dependencies
Linked: JUCE 9.0.5 (Starter tier, see `DECISIONS.md`), clap-juce-extensions (MIT, pinned SHA).
Embedded: Archivo (SIL OFL 1.1, see `DECISIONS.md`).
Tests only: Catch2 3.8.1 (BSL-1.0). Tools that run against the binary and are not linked:
pluginval (GPLv3), clap-validator (MIT), auval (Apple).
