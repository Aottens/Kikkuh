# Module spec: body (BODY)

Status: **approved by the owner (2026-11-03). Scope: BODY-01..04, built by VS1-01.** BODY-05..07
(filter, drive, tail anchor) come with VS1-03 and VS1-04. This voice replaces the fixed test kick of
BUILD-06.

Time runs in seconds, so everything below holds from 44.1 to 192 kHz (ENG-04). Unless a criterion
says otherwise, "level" is at velocity 127 with the other knobs at their defaults. Velocity scales
the level linearly until PLAY-01 adds its knob, and every MIDI note plays the same kick (PLAY-02 off).

## Parameters

Ids are permanent once released (ENG-06). Ranges may change until the first release, because
automation is stored relative to the range.

| Id | Knob | Range | Default | Knob feel |
| --- | --- | --- | --- | --- |
| `body_wave` | Wave | 0..3 (0 sine, 1 triangle, 2 saw, 3 square) | 0 | linear, continuous |
| `body_tune` | Tune | C0..C3 (32.7..261.6 Hz), whole notes | A0 (55 Hz) | one step per semitone |
| `body_pitch_amount` | Pitch Amount | 0..72 semitones | 36 | linear |
| `body_pitch_decay` | Pitch Decay | 1..1000 ms | 100 ms | centre at 60 ms |
| `body_pitch_shape` | Pitch Shape | -100..+100 % | 0 | linear |
| `body_amp_attack` | Attack | 0..50 ms | 0 ms | centre at 5 ms |
| `body_amp_hold` | Hold | 0..500 ms | 0 ms | centre at 50 ms |
| `body_amp_decay` | Amp Decay | 10..2000 ms | 400 ms | centre at 200 ms |
| `body_amp_shape` | Amp Shape | -100..+100 % | 0 | linear |

**Note names:** MIDI note 60 is C3, so MIDI 36 is C1 (65.4 Hz) and A0 is MIDI 33 (55 Hz).

## Curves: Decay and Shape

Pitch and amp share one curve. Over a Decay of `D`, it falls from 1 to exactly 0:

`g(t) = (1 - t/D)^k`, with `k = 3^(1 + Shape/100)`

So Shape -100 gives k = 1 (a straight line), 0 gives k = 3, and +100 gives k = 9 (falls fast, then a
long tail). At half of Decay the curve stands at 0.5, 0.125 and 0.002 respectively. Decay is the whole
length, not a time constant: when Decay has passed, the curve has arrived.

## Acceptance criteria

### BODY-01 Wave

- **BODY-01a** At Wave 0, 1, 2 and 3 with Pitch Amount 0, the tail has the harmonics of that wave,
  within 10 %, as a ratio to the fundamental. Sine: 2nd and 3rd below -60 dB. Triangle: 2nd below
  -60 dB, 3rd 1/9. Saw: 2nd 1/2, 3rd 1/3. Square: 2nd below -60 dB, 3rd 1/3.
- **BODY-01b** Between two waves the knob crossfades: at Wave 2.5, the 2nd harmonic is halfway between
  saw and square, within 10 %.
- **BODY-01c** Every wave has its fundamental in the same phase and starts at zero. Moving Wave never
  cancels the fundamental: at 0, 0.5, 1 ... 3 the fundamental stays above 0.5.
- **BODY-01d** Each whole-numbered wave peaks between 0.9 and 1.0, so no wave is far louder by peak.
- **BODY-01e** Band-limited: at every Wave value of BODY-01c, for fundamentals of 261.6, 4186 and
  12000 Hz at 48 kHz, energy away from the harmonics stays at least 60 dB below the total.
- **BODY-01f** Band-limiting does not dull the low notes: for a saw at 55 Hz at 48 kHz, the harmonic
  nearest 10 kHz is within 3 dB of its 1/n level.
- **BODY-01g** A sweep does not change the level: for every whole-numbered wave, the fundamental stays
  within 5 % from 55 to 3520 Hz at 48 kHz.

### BODY-02 Tune

- **BODY-02a** With Pitch Amount 0, the tail of each note from C0 to C3 lands on
  `440 * 2^((midi - 69) / 12)` Hz, within 0.5 %.
- **BODY-02b** The host shows Tune as a note name: "A0", "F#0", "C1". Typing such a name sets the knob.
- **BODY-02c** Tune is where the pitch envelope ends, and Pitch Amount goes up from there. The kick
  starts at Tune + Amount and falls to Tune. Moving Tune moves the whole sweep.
- **BODY-02d** The readout gives the note the tail actually lands on, taken at the moment the amp
  envelope has fallen to -40 dB. If the pitch envelope is done by then, it reads Tune exactly ("A0").
  If not, it reads the nearest note with cents ("C#1 +23 ct"), and a measured render agrees within
  10 cents. The processor offers the readout now; VS1-06 puts it on the panel, because the generic
  editor cannot show it.

### BODY-03 Pitch envelope

- **BODY-03a** Pitch Amount: the sweep starts at Tune + Amount, within 25 cents, measured on a slow
  sweep (Decay 1000 ms). Amount 0 gives no sweep at all.
- **BODY-03b** Pitch Decay: from Decay onward the pitch is Tune, within 0.5 %, at 10, 100 and 1000 ms.
- **BODY-03c** Pitch Shape: halfway through a Decay of 1000 ms with Amount 24, the offset is
  `24 * 0.5^k` semitones (12, 3, 0.05) at Shape -100, 0, +100, within 0.5 semitone.
- **BODY-03d** The pitch envelope starts at the note-on, whatever Attack and Hold are set to.

### BODY-04 Amp envelope

- **BODY-04a** Attack: the level rises as a straight line from 0 to 1. Halfway through a 40 ms Attack
  the level is 0.5, within 10 %. At Attack 0, the first sample after the note-on starts at zero,
  on the note's exact sample (the behaviour of BUILD-06).
- **BODY-04b** Hold: the level stays above 0.95 from the end of Attack to the end of Hold, at 100 ms.
- **BODY-04c** Amp Decay: output is exactly 0 from Attack + Hold + Decay onward, within one sample, at
  10, 400 and 2000 ms, and never before.
- **BODY-04d** Amp Shape: halfway through a 1000 ms Decay, the level is `0.5^k` (0.5, 0.125, 0.002) at
  Shape -100, 0, +100, within 10 %, or within 0.005 for the last.
- **BODY-04e** A kick ends without a click: the largest step in the last 5 ms of a kick stays below
  that of the sine at full level, at every Decay and Shape value tested.

### Engineering, as it applies to BODY

- **ENG-01a** All nine parameters are automatable by the host.
- **ENG-01b** Every parameter is smoothed per sample. A knob jump settles within 1 % of its range within
  50 ms, at every sample rate. A knob moved while no kick rings holds in full from the next kick's
  first sample, without a glide.
- **ENG-01c** A full-range jump of any knob during a ringing tail makes no click: the largest step
  stays below 0.05 (a sine at C3 has 0.034 of its own). Wave is tested from sine to triangle, because a
  saw and a square have large steps by nature. Pitch knobs are tested with the sweep still running at
  a low pitch.
- **ENG-01d** `processBlock` does not allocate while parameters change.
- **ENG-01e** Each knob's text survives a round trip, as clap-validator demands: value to text, back to
  a value and to text again gives the same text. Times and semitones show one decimal, Shape whole
  percents and Wave two decimals.
- **ENG-06a** The state restores all nine parameters. A state that lacks a parameter (an older version,
  or a VS0-01 project) gives that parameter its default. The state keeps its version, and
  `setStateInformation` runs saved states through a migration step that later versions can extend.
- **ENG-06b** A test lists the nine ids as written above, so renaming one fails.
- **ENG-03, ENG-04** hold with the new voice, also at non-default knob values.
