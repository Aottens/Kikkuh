# Module spec: layer (LAYER), the CLICK and TOP sections

Status: **approved by the owner (2026-11-05), with every recommendation: Tone and Noise only, Start with VS2-02, 12 dB filters, mono.** Scope: LAYER-01..03 and the panel row, built by VS2-01 and VS2-04;
LAYER-04 and the Sample source come with VS2-02. **LAYER-04 is proposed (2026-11-06) and waits for the owner (gate 1).**

CLICK and TOP are two identical sections, one class in `source/dsp/` used twice, that play in parallel over the body.
Only their defaults differ. They are what the owner's runs left to them:

- In VS1-09 (`tests/manual/runs/2026-11-04-VS1-09.md`) a tone click won parts of rounds 3 and 4 and lost in round 8, on
  one setting: 3 kHz, 4 ms, -6 dB. So the owner liked a click, but not that one on top of the chosen kick. That setting is
  reproducible here (LAYER-02f), and the sections start silent (LAYER-03e).
- In CORE-01 (`tests/manual/runs/2026-11-05-CORE-01.md`) Beefy SM ELE missed on its almost acoustic character. VS1-08
  (`docs/research/reference-kicks.md`) measured what BODY cannot make: in its first 5 ms 78.6 % of Beefy's energy lies
  above 1 kHz (52.9 % in 1-4 kHz, 25.7 % above 4 kHz) against 0.0 % on BODY, and after 5 ms that is gone (1.3 %). That is
  a short, bright, noisy hit, which LAYER-02g asks the sections to reach.

Time runs in seconds (ENG-04). Unless a criterion says otherwise, "level" is at velocity 127, BODY at its defaults, and
"the layer" is what a render with the section on adds to a render with it off. Nothing limits the sum until the Master
section (VS3-01): a layer adds to a default kick that already peaks at +1.17 dBFS.

## Signal flow

Each section plays its source from the note-on, filters it with High-pass and Low-pass, shapes it with its Decay
envelope and scales it by Level and the note's level (PLAY-01's formula, the same as BODY's). The two sections are added to
BODY after BODY's drive and filter, so they are never clipped by BODY's Drive, and the sum goes to the output (and to
MASTER when it exists). Mono, as BODY is. The filters come before the envelope, so a section is exactly silent once
Decay has passed; source and filters start afresh at every note-on, so every hit is the same (ENG-03).

```
note ─► BODY (osc, amp, drive, filter) ──┐
     ─► CLICK (source, HP, LP, decay, level) ─┼─► sum ─► out
     ─► TOP   (source, HP, LP, decay, level) ─┘
```

## Sources (LAYER-01)

Built in, generated, no factory samples (SELL-06 stays simple):

- **Tone**: a sine at `3000 Hz * 2^(Tune/12)`, starting at phase 0, as the VS1-09 sketch played it.
- **Noise**: white noise from a fixed table made at 48 kHz, played at the rate `2^(Tune/12)`, as a sampler plays a
  noise sample: up brightens it, down darkens it. Every note plays the same stretch, so ENG-03 holds.
- **Sample** (VS2-02): your own file, dragged onto the section, played from Start at the rate `2^(Tune/12)` (LAYER-04).

## Parameters

Ids are permanent once released (ENG-06). Each id below exists twice: once with `click_` and once with `top_` in front
(`click_level`, `top_level`, ...). Ranges may change until the first release.

| Id (after the prefix) | Knob | Range | Default CLICK | Default TOP | Knob feel |
| --- | --- | --- | --- | --- | --- |
| `source` | Source | Tone, Noise, Sample | Tone | Noise | switch, read at the note-on |
| `level` | Level | Off, -60..+6 dB | Off | Off | linear in dB; the bottom position is Off |
| `tune` | Tune | -24..+24 semitones | 0 | 0 | linear, continuous |
| `decay` | Decay | 1..2000 ms | 4 ms | 30 ms | centre at 50 ms |
| `hp` | High-pass | 20..20000 Hz | 20 Hz | 1000 Hz | centre at 632 Hz |
| `lp` | Low-pass | 20..20000 Hz | 20000 Hz | 20000 Hz | centre at 632 Hz |
| `start` | Start | 0..1000 ms into the sample | 0 | 0 | centre at 50 ms; read at the note-on |

Start comes with the Sample source in VS2-02 and acts on it only: on a generated source it has nothing worth skipping.

## Curve: Decay

The section's envelope falls from 1 to exactly 0 over Decay: `g(t) = (1 - t/D)^2`, the curve the VS1-09 sketch used.
After Decay the section is silent. A Sample also stops at its own end.

## Acceptance criteria

### LAYER-01 Source

- **LAYER-01a** Tone at Tune 0 measures 3000 Hz within 0.5 %; at Tune +12 and -12, 6000 and 1500 Hz, at 44.1, 48 and
  192 kHz.
- **LAYER-01b** Noise is broadband: with filters open and Tune 0, over a Decay of 200 ms its energy splits over
  `bands` (HARN-05) 300 Hz-1 kHz, 1-4 kHz and 4 kHz up within 3 percentage points of white noise up to 20 kHz.
- **LAYER-01c** Noise follows Tune: at Tune -12 it plays at half speed, so it holds nothing above 12 kHz. At 48 kHz,
  leaving out 12 kHz up changes its share of energy above 4 kHz by less than 1 percentage point, where at Tune 0 it
  changes it by more than 10 (white noise: 83 % against 67 %). (Corrected in VS2-01: the first wording, half the share
  above 4 kHz, cannot hold for noise that is slowed down, which keeps 67 % there.)
- **LAYER-01d** ENG-03 holds for both sources: the same note renders identically twice, across block sizes.
- **LAYER-01e** ENG-04 holds for both sources: the layer's peak and its `bands` stay within 1 dB and 3 percentage points
  between 44.1, 48, 96 and 192 kHz (energy above 20 kHz left out).
- **LAYER-01f** Source is read at the note-on: moving it while a layer rings leaves that layer sample-identical, and
  the next note follows it.

### LAYER-02 Knobs

- **LAYER-02a** Level: the layer's peak follows Level within 0.5 dB at -40, -12, 0 and +6 dB, on Tone with Decay 2000 ms.
  At Off the render is sample-identical to a render without the section.
- **LAYER-02b** Velocity scales the layer as it scales BODY: at MIDI velocity 64 and Velocity 100 % the layer's peak is
  0.504 of that at 127, within 1 %.
- **LAYER-02c** Decay: on Tone with Decay 10, 100 and 1000 ms, the level halfway through Decay is 0.25 within 10 %, and
  the layer is exactly 0 from Decay onward, within one sample.
- **LAYER-02d** High-pass and Low-pass are 12 dB per octave each: from 4 to 8 times past the cutoff they fall 12 dB per
  octave within 2 dB, and the -3 dB point is within 10 % of the cutoff, at 44.1, 48 and 192 kHz. At 20 Hz and
  20000 Hz they change Noise's level by less than 0.1 dB from 100 Hz to 10 kHz.
- **LAYER-02e** Tune is the pitch of the source (LAYER-01a, c). It does not follow Key Track or the note: layers are the
  same on every note.
- **LAYER-02f** The VS1-09 tone click is a setting: CLICK on Tone, Tune 0, Decay 4 ms, Level -6 dB, filters open, follows
  the sketch's `0.5 * sin (2 pi 3000 t) * (1 - t / 4 ms)^2` within 0.05 per sample (the open filters shift its phase a
  little) and within 0.5 dB in peak.
- **LAYER-02g** The layers can reach Beefy's first 5 ms (proposed): on BODY at Beefy's fit in VS1-08, there is a setting of
  CLICK and TOP within their ranges that puts at least 60 % of the energy in 0-5 ms above 1 kHz and keeps it below 5 %
  in 5-20 ms. The test holds that setting and writes it into this spec; if no setting reaches it, VS2-01 reports the
  best it reaches and stops for the owner.
  VS2-01 found it: CLICK on Noise, Level +6 dB, Decay 8 ms, High-pass 1500 Hz, and TOP on Noise, Level +6 dB, Decay 10 ms,
  High-pass 1000 Hz, both Tune 0 and Low-pass open, which puts 75.6 % above 1 kHz in 0-5 ms and 0.03 % in 5-20 ms.
- **LAYER-02h** ENG-01 holds for Level, Tune, Decay, High-pass and Low-pass: smoothed per sample, settling within 50 ms,
  and a full-range jump during a ringing layer (Tone, Decay 2000 ms, Level -6 dB) stays below a step of 0.05 beyond the
  tone's own step. ENG-01e: Level shows one decimal or "Off", Tune one decimal, Decay one decimal, the filters whole Hz
  and from 1000 Hz up in kHz, as Cutoff does.

### LAYER-03 In parallel, for as long as Decay says

- **LAYER-03a** The layers add: a render with CLICK and TOP on equals BODY alone plus each section alone, within
  1e-6 per sample. BODY sounds exactly as before, whatever the layers are set to.
- **LAYER-03b** A layer starts on the note's exact sample, with uneven host blocks, as BODY does (PLAY-01a). Latency stays
  0 (ENG-02).
- **LAYER-03c** From 1 ms up to the full kick: at Decay 1 ms the layer is silent from 1 ms on; at Decay 2000 ms it still
  sounds when BODY at Amp Decay 2000 ms does.
- **LAYER-03d** A retrigger does not click: a new note restarts both layers on its own sample, and the old layer's last
  value fades with PLAY-03's 3 ms time constant. On Tone with Decay 2000 ms and Level -6 dB, a second note at 16 points
  across one period keeps the largest step around it below 0.05 beyond the tone's own step.
- **LAYER-03e** Saved projects keep their sound (ENG-06a): both sections default to Level Off, so a state without these
  knobs, any version, renders sample-identically to before, and the state version stays 3.
- **LAYER-03f** ENG-01d: `processBlock` does not allocate with both layers on, in a run of retriggers, while every layer
  knob changes. The noise table is built off the audio thread, as the wavetables are.

### LAYER-04 Your own sample (VS2-02, proposed)

A sample is a file of the owner's, held whole in the project: once dropped, the file itself is no longer needed. It is
summed to mono (the mean of its channels), kept at its own sample rate and cut at 3 s, which covers Start (up to 1 s)
plus Decay (up to 2 s). It is not normalised: Level works on the file as it is.

- **LAYER-04a** Drag and drop: a WAV, AIFF or FLAC file dropped on the CLICK or TOP part of the panel's second band
  loads into that section and sets its Source to Sample; if its Level is Off, Level goes to -6 dB, so a drop is heard.
  Other files are refused while they are dragged. Of several files the first readable one loads. The panel shows the
  file's name in the empty place under Low-pass (LAYER-04l).
- **LAYER-04b** Loading is off the audio thread: reading, summing and cutting run on a background thread, and the finished
  sample reaches the audio thread by a snapshot swap, taken at the next note-on, so a ringing layer finishes on the sample
  it started with. The replaced sample is freed off the audio thread. ENG-01d: `processBlock` neither allocates nor frees
  (the allocation guard counts frees too) while samples are dropped and replaced 20 times during a run of retriggers.
- **LAYER-04c** Playback: on Sample, Tune 0, Start 0, Decay 2000 ms, filters open and Level 0 dB, a 1 kHz sine file at the
  host's rate plays at 1 kHz within 0.5 % with its peak within 0.5 dB of the file's, and a stereo file plays the mean of
  its channels within 1e-4 per sample. The file is read with the sinc kernel the noise uses.
- **LAYER-04d** Tune is the playback rate: a 1 kHz sine file at 48 kHz measures 1000, 2000 and 500 Hz at Tune 0, +12 and
  -12, within 0.5 %, at host rates of 44.1, 48 and 192 kHz; the same sine in a 44.1 kHz file measures the same. At +12
  the file ends in half its length, at -12 in twice it.
- **LAYER-04e** Start: the layer begins Start ms into the file, counted in the file's own time, whatever Tune says. A
  file with a one-sample impulse at 100 ms and Start 100 ms puts the impulse's peak on the note's own sample, within one
  sample. A Start past the file's end gives silence. Start is read at the note-on, as Source (LAYER-01f), so it is not
  smoothed: moving it while a layer rings leaves that layer sample-identical, and the next note follows it. On Tone and
  Noise it changes nothing: Start 0 and 1000 ms render sample-identically. ENG-01e: Start shows one decimal, in ms.
- **LAYER-04f** The end does not click: the layer stops at Decay or at the end of the file, whichever comes first, and at
  the file's end its last value fades with PLAY-03's 3 ms time constant. A file that ends at full scale keeps the largest
  step there below 0.05.
- **LAYER-04g** Saved with the project: the state holds each section's sample (its mono audio as 32-bit float, its rate
  and its file name). A reloaded state renders sample-identically to before saving, also after the file is deleted.
  At most 3 s at 192 kHz, about 3 MB of state per section.
- **LAYER-04h** A missing sample never breaks the project: a file that cannot be read (damaged, empty, not audio) leaves
  the section as it was, and the panel shows "Cannot read" with the name. Source Sample without a sample (switched to
  before any drop, or a state whose sample is damaged or absent) renders silence, sample-identical to Level Off, and the
  panel shows "Drop a sample" or "Missing:" with the saved name. The rest of the state always loads.
- **LAYER-04i** ENG-03 and ENG-04 hold on Sample: the same note renders identically twice, across block sizes; with a
  48 kHz file of white noise the layer's peak and `bands` stay within 1 dB and 3 percentage points between host rates of
  44.1, 48, 96 and 192 kHz (energy above 20 kHz left out).
- **LAYER-04j** Saved projects keep their sound (ENG-06a), and the state version stays 3: a state without the Start ids
  or a sample, any version, renders sample-identically to before, because Start defaults to 0 and no older state can
  choose Sample. Source stores its choice's index, so Tone and Noise come back as they were; a host's automation of
  Source, which works on 0..1, moves Noise from 1 to 0.5, which is allowed before the first release.
- **LAYER-04k** ENG-06b: the id test lists the two Start ids, `click_start` and `top_start`.
- **LAYER-04l** The panel (FAST-05 holds): Start is an S knob in the empty place under High-pass (CLICK x 420, TOP x 776,
  the bottom row), the sample's name or message is printed under Low-pass (x 504 and 860, one line, cut with an ellipsis),
  and the Source switch steps Tone, Noise, Sample. The panel changes, so CORE-01 runs again: VS2-03.

## The panel (VS2-04)

FAST-05 holds for the new controls: one control per parameter (FAST-05a), the grid (FAST-05l), section lines (FAST-05m).
The plate grows downward by a second band of 280 points, to **1144 x 598**: the first band's geometry again, 280 points
lower, with its own signal line and the titles CLICK and TOP on it. CLICK stands under PITCH and TOP to its right, with a
section line between them at x = 556 (two tones, as FAST-05m, from below the second signal line to its bottom value line).
Each section has four columns 84 points apart, L knobs on top and the bottom row below, as in the first band:

| Column | CLICK x | TOP x | Top row (L) | Bottom row |
| --- | --- | --- | --- | --- |
| 1 | 252 | 608 | Level | Source (switch, its text the source's name: "Tone" or "Noise"; never filled, it is a choice, not on or off) |
| 2 | 336 | 692 | Decay | Tune (S) |
| 3 | 420 | 776 | High-pass | |
| 4 | 504 | 860 | Low-pass | |

Labels: Level, Decay, High-pass, Low-pass, Source, Tune. Values as FAST-05j: dB with "Off" at the bottom, ms, Hz and from
1000 Hz up kHz as Cutoff, semitones. Start takes the empty place under High-pass in VS2-02, and the sample's name the place under Low-pass (LAYER-04l). The panel
changes, so CORE-01 is run again: VS2-03, after VS2-02.

## Engineering, as it applies to LAYER

- **ENG-06b** The id test lists the twelve ids of VS2-01, and VS2-02 adds the two Start ids (LAYER-04k).
- **ENG-03, ENG-04** hold with both layers on, at non-default values.
- The section is plain C++ in `source/dsp/` without JUCE; the processor only reads its knobs and adds its output.
