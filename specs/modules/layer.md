# Module spec: layer (LAYER), the CLICK and TOP sections

Status: **proposal, waiting for the owner (gate 1).** Scope: LAYER-01..03 and the panel row, built by VS2-01 and VS2-04;
LAYER-04 and the Sample source come with VS2-02 and get their full criteria when it starts.

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

Each section plays its source from the note-on, shapes it with its Decay envelope, filters it with High-pass and
Low-pass and scales it by Level and the note's level (PLAY-01's formula, the same as BODY's). The two sections are added to
BODY after BODY's drive and filter, so they are never clipped by BODY's Drive, and the sum goes to the output (and to
MASTER when it exists). Mono, as BODY is.

```
note ─► BODY (osc, amp, drive, filter) ──┐
     ─► CLICK (source, decay, HP, LP, level) ─┼─► sum ─► out
     ─► TOP   (source, decay, HP, LP, level) ─┘
```

## Sources (LAYER-01)

Built in, generated, no factory samples (SELL-06 stays simple):

- **Tone**: a sine at `3000 Hz * 2^(Tune/12)`, starting at phase 0, as the VS1-09 sketch played it.
- **Noise**: white noise from a fixed table made at 48 kHz, played at the rate `2^(Tune/12)`, as a sampler plays a
  noise sample: up brightens it, down darkens it. Every note plays the same stretch, so ENG-03 holds.
- **Sample** (VS2-02): your own file, dragged onto the section; played at `2^(Tune/12)`. Until VS2-02 the choice does not
  exist.

## Parameters

Ids are permanent once released (ENG-06). Each id below exists twice: once with `click_` and once with `top_` in front
(`click_level`, `top_level`, ...). Ranges may change until the first release.

| Id (after the prefix) | Knob | Range | Default CLICK | Default TOP | Knob feel |
| --- | --- | --- | --- | --- | --- |
| `source` | Source | Tone, Noise (VS2-02 adds Sample) | Tone | Noise | switch, read at the note-on |
| `level` | Level | Off, -60..+6 dB | Off | Off | linear in dB; the bottom position is Off |
| `tune` | Tune | -24..+24 semitones | 0 | 0 | linear, continuous |
| `decay` | Decay | 1..2000 ms | 4 ms | 30 ms | centre at 50 ms |
| `hp` | High-pass | 20..20000 Hz | 20 Hz | 1000 Hz | centre at 632 Hz |
| `lp` | Low-pass | 20..20000 Hz | 20000 Hz | 20000 Hz | centre at 632 Hz |
| `start` (VS2-02) | Start | 0..1000 ms into the source | 0 | 0 | centre at 50 ms |

Start comes with the Sample source in VS2-02: on a generated source it has nothing worth skipping.

## Curve: Decay

The section's envelope falls from 1 to exactly 0 over Decay: `g(t) = (1 - t/D)^2`, the curve the VS1-09 sketch used.
After Decay the section is silent. A Sample also stops at its own end.

## Acceptance criteria

### LAYER-01 Source

- **LAYER-01a** Tone at Tune 0 measures 3000 Hz within 0.5 %; at Tune +12 and -12, 6000 and 1500 Hz, at 44.1, 48 and
  192 kHz.
- **LAYER-01b** Noise is broadband: with filters open and Tune 0, over a Decay of 200 ms its energy splits over
  `bands` (HARN-05) 300 Hz-1 kHz, 1-4 kHz and 4 kHz up within 3 percentage points of white noise up to 20 kHz.
- **LAYER-01c** Noise follows Tune: at Tune -12 the share of energy above 4 kHz is at most half of that at Tune 0.
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
  the sketch's `0.5 * sin (2 pi 3000 t) * (1 - t / 4 ms)^2` within 0.01 per sample after the filters' effect, and within
  0.5 dB in peak.
- **LAYER-02g** The layers can reach Beefy's first 5 ms (proposed): on BODY at Beefy's fit in VS1-08, there is a setting of
  CLICK and TOP within their ranges that puts at least 60 % of the energy in 0-5 ms above 1 kHz and keeps it below 5 %
  in 5-20 ms. The test holds that setting and writes it into this spec; if no setting reaches it, VS2-01 reports the
  best it reaches and stops for the owner.
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

### LAYER-04 Your own sample (VS2-02)

Criteria come when VS2-02 starts: loading off the audio thread with a snapshot swap, the file saved with the project, a
missing file giving silence and a message, and the Start knob.

## The panel (VS2-04)

FAST-05 holds for the new controls: one control per parameter (FAST-05a), the grid (FAST-05l), section lines (FAST-05m).
The plate grows downward by one row band, and CLICK and TOP stand on it, left to right, with the signal line running
through them to the same arrow. Each section has four columns:

| Column | Top row (L) | Bottom row (S) |
| --- | --- | --- |
| 1 | Level | Source |
| 2 | Decay | Tune |
| 3 | High-pass | |
| 4 | Low-pass | |

Start takes the empty place under High-pass when it arrives in VS2-02. The panel changes, so CORE-01 is run again: VS2-03,
after VS2-02.

## Engineering, as it applies to LAYER

- **ENG-06b** The id test lists the twelve ids of VS2-01, and VS2-02 adds the two Start ids.
- **ENG-03, ENG-04** hold with both layers on, at non-default values.
- The section is plain C++ in `source/dsp/` without JUCE; the processor only reads its knobs and adds its output.
