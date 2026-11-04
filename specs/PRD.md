# Kikkuh by Colourmotion — PRD

Status: **scope approved by the owner, 2026-11-03.** Research behind it: `docs/research/kick-plugins.md`.
Ids are stable so the owner can comment per line. They become full acceptance criteria in
`specs/modules/` when a slice starts.

## Core value

From the kick in your head to the kick in your track, fast, with knobs like a subtractive synth.
Rumble is part of the instrument. Curves exist for those who want them, and you never need them.

**Success test (CORE-01).** The owner picks three reference kicks and gets each close enough to use
within 3 minutes, knobs only, in the first playable build and again at every UI change. If this
fails, the panel changes before anything else gets built.

## v1: sellable kick and rumble instrument

### Sound sections, one screen, signal flow left to right

**BODY**, the synth voice
- BODY-01 Wave morph sine → triangle → saw → square, band-limited.
- BODY-02 Tune set as a note (C1, F#0, …), with a readout of the note the tail actually lands on.
- BODY-03 Pitch envelope as knobs: Amount (semitones), Decay (ms), Shape.
- BODY-04 Amp envelope as knobs: Attack, Hold, Decay, Shape.
- BODY-05 Filter as a subtractive synth has it: Cutoff, Resonance, Env Amount.
- BODY-06 Drive on the voice, before the filter.
- BODY-07 Tail anchor, on by default: changing pitch knobs before the anchor leaves the phase of
  the tail unchanged, sample for sample. Off means classic behaviour.
- BODY-08 Defaults chosen by ear in VS1-09: a fast sweep, a second stage, a start at full level and a little drive.
- BODY-09 A second sweep stage: Sweep 2 Amount and Sweep 2 Decay.
- BODY-10 Start: the phase of the oscillator at the note-on.

**CLICK** and **TOP**, two identical parallel sections
- LAYER-01 Source is a built-in noise or transient, or your own sample by drag and drop.
- LAYER-02 Knobs: Level, Tune, Start, Decay, High-pass, Low-pass.
- LAYER-03 Runs in parallel over the body for as long as Decay says, from 1 ms up to the full kick.
- LAYER-04 Your samples are saved with the project; a missing file never breaks the project.

**RUMBLE**
- RUM-01 Source: dark reverb fed by the kick, or noise.
- RUM-02 Groove in tempo: Delay after the kick, Hold and Release, in note values.
- RUM-03 Tone: Low-pass with its own envelope in tempo, Drive, Size, Level.
- RUM-04 Reset: the next kick cuts or ducks the rumble, with a knob for how hard.
- RUM-05 Mono below an adjustable frequency.

**MASTER**
- MAST-01 Clipper with Drive, oversampled, with autogain so loudness does not fool you.
- MAST-02 Low and high shelf.
- MAST-03 Output level and a peak and level meter.
- MAST-04 Separate outputs: kick and rumble, plus the sum on the main output.

### Getting to the sound quickly
- FAST-01 Audition: a hit pad, and a loop that retriggers in host tempo without MIDI or transport.
- FAST-02 Starting points: 8–12 archetypes that set every knob (909, Berlin rumble, hardgroove, acid,
  trance, psy, hard). A short list you start from; no sound library.
- FAST-03 Reference slot: drop a WAV, switch level-matched between it and Kikkuh, see its waveform
  over yours. No analysis, no import.
- FAST-04 A/B compare of two settings.
- FAST-05 Everything visible on one screen with no tabs; resizable.

### Curves, the extra
- CURV-01 Live view of pitch and amp on a logarithmic time axis, so the first 10 ms are readable
  without zooming.
- CURV-02 Drag the curve to make your own shape.
- CURV-03 Knobs keep working after that: Amount scales the height, Decay scales the length, and your
  shape stays. No separate mode to fall out of.

### Playing
- PLAY-01 MIDI note triggers sample-accurately; velocity sets level, with an amount knob.
- PLAY-02 Key tracking off by default (every note plays the set kick), on for 808-style playing.
- PLAY-03 Retrigger without clicks, also at high tempo.

### Engineering
- ENG-01 Every parameter is host-automatable and smoothed. No allocation or locks on the audio thread.
- ENG-02 Zero reported latency, so the kick lands on the grid. Oversampling must respect this.
- ENG-03 The same note with the same settings renders identically (checked by test).
- ENG-04 Sounds the same from 44.1 to 192 kHz (checked by test).
- ENG-05 Under 3 % CPU per instance on an M-series Mac at 48 kHz / 256 samples.
- ENG-06 Saved state is versioned and survives updates; parameter ids are permanent.

### Selling
- SELL-01 Formats: VST3, AU, CLAP. macOS universal (arm64 + x86_64) and Windows x64.
- SELL-02 Passes pluginval and clap-validator in CI, plus auval for AU.
- SELL-03 Signed and notarised macOS installer; signed Windows installer.
- SELL-04 Offline licence key: no iLok, no required online account, no telemetry.
- SELL-05 Demo mode.
- SELL-06 Short manual; factory samples and presets all self-made or cleanly licensed.
- SELL-07 Tested by hand in Bitwig first, and before release in at least Ableton Live, Logic Pro, FL Studio.

## Decide before the first release

**X-01 External rumble: send and return.** Kick goes out, your own rumble chain comes back into
Kikkuh, through groove, Reset and Master. The idea is the owner's and it stands out; the research found
no kick synth that has it. It ships after v1, but it changes the plugin's audio buses, and hosts store those
in projects, so the layout must be settled before release.

- Return: a sidechain input as a third rumble source. Simple, and Kikzilla 2 already offers a
  comparable external mode.
- Send: an extra output for the dry kick. The risk is the loop: Kikkuh → reverb track → back into
  Kikkuh is a routing cycle in the host. A forum report says Bitwig allows this with a small delay
  and another that Ableton's delay compensation stops working in such paths. **Both are single user
  reports, so this needs a spike:** a minimal plugin tried in each host, measuring whether the
  routing is allowed and how much delay the return gets.
- Fallback if hosts refuse: a slot inside Kikkuh that loads your reverb plugin directly. No routing
  at all and the best experience, but much more work: plugin scanning, formats, and other people's
  crashes inside our plugin.

## Later

Send and return (X-01); drag the rendered kick into the DAW as WAV; audio trigger to replace or
augment kicks; extra sample layers; randomise; envelopes on drive and harmonics; preset browser with
tags; AAX; analysis import of an existing kick.

## Out of scope

Large sample and preset libraries; an effects rack; a modulation matrix; bass synth or sequencer;
subscription or cloud; AI features.

## Commercial facts (checked 2026-11-03; read the actual licence texts before release)

- [JUCE](https://juce.com/get-juce/): Starter free up to $20k revenue; Indie $40 a month or $800 once,
  up to $300k; Pro above that. The page does not say outright that Starter covers closed-source
  sales, so read the EULA. It also points to a JUCE 9 EULA, while Dukko is on JUCE 8.0.12.
- [VST3](https://rekkerd.org/vst-3-now-available-under-mit-license) has been MIT since 3.8 (October 2025).
- No GPL code may be linked in, or the plugin could not be sold closed-source.
- **Name:** one web search found no audio product called Kikkuh, but that is no trademark search. Check
  BOIP, EUIPO and USPTO before fixing the name: plugin identity is permanent once projects use it.
