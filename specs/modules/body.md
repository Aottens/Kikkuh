# Module spec: body (BODY)

Status: **approved by the owner (2026-11-03). Scope: BODY-01..04, built by VS1-01.** BODY-05..06
(filter, drive) come with VS1-03. BODY-07 (tail anchor) comes with VS1-04, approved by the owner (2026-11-04). BODY-05, 06, 08, 09 and 10 are written from the
owner's choices in VS1-09 (`tests/manual/runs/2026-11-04-VS1-09.md`) and approved with that PR. This voice
replaces the fixed test kick of BUILD-06.

Time runs in seconds, so everything below holds from 44.1 to 192 kHz (ENG-04). Unless a criterion
says otherwise, "level" is at velocity 127 with the other knobs at their defaults. Velocity scales
the level linearly until PLAY-01 adds its knob, and every MIDI note plays the same kick (PLAY-02 off).
BODY-01..04 describe the voice itself: since BODY-08 they are measured with Drive 0, Start 0 and Sweep 2 Amount 0, and since BODY-07 with Tail Anchor off.

## Parameters

Ids are permanent once released (ENG-06). Ranges may change until the first release, because
automation is stored relative to the range.

| Id | Knob | Range | Default | Knob feel |
| --- | --- | --- | --- | --- |
| `body_wave` | Wave | 0..3 (0 sine, 1 triangle, 2 saw, 3 square) | 0 | linear, continuous |
| `body_tune` | Tune | C0..C3 (32.7..261.6 Hz), whole notes | A0 (55 Hz) | one step per semitone |
| `body_pitch_amount` | Pitch Amount | 0..72 semitones | 60 | linear |
| `body_pitch_decay` | Pitch Decay | 1..1000 ms | 40 ms | centre at 60 ms |
| `body_pitch_shape` | Pitch Shape | -100..+100 % | +30 | linear |
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

### BODY-05 Filter (VS1-03)

No filter won a round in VS1-09, so its type is a proposal: a 24 dB low-pass, the usual subtractive-synth one, the
second-order sections as in `sketch::Filter`'s `svf24`. It is open by default, so it changes nothing until
it is used, and it runs after drive.

| Id | Knob | Range | Default | Knob feel |
| --- | --- | --- | --- | --- |
| `body_filter_cutoff` | Cutoff | 20..20000 Hz | 20000 Hz | centre at 632 Hz |
| `body_filter_res` | Resonance | 0..100 % | 0 % | linear |
| `body_filter_env` | Env Amount | 0..8 octaves | 0 | linear |

- **BODY-05a** At the defaults a sine sweep to 5 kHz stays within 0.1 dB of a render without the filter.
- **BODY-05b** The filter falls 24 dB per octave within 3 dB from 4 to 8 times the cutoff, and at Resonance 0 the
  -3 dB point is within 10 % of Cutoff, at 44.1, 48 and 192 kHz (ENG-04).
- **BODY-05c** Env Amount opens the cutoff by that many octaves at the note-on and falls along the amp decay's
  curve. On a saw with Cutoff 200 Hz and Env 5, energy above 1 kHz is over 1 % in 0-20 ms and below 0.1 % once Amp
  Decay is half over.
- **BODY-05d** ENG-01 holds for the three knobs: a full-range jump during a tail stays below a step of 0.05.

### BODY-06 Drive (VS1-03)

The owner chose hard clipping at a low amount, after the amp and before the filter. Soft, asymmetric and
before-the-amp lost at 12 dB, and hard at 3 and 6 dB won over everything at 12 dB.

| Id | Knob | Range | Default | Knob feel |
| --- | --- | --- | --- | --- |
| `body_drive` | Drive | 0..24 dB | 6 dB (BODY-08) | linear |

- **BODY-06a** Drive multiplies the signal after the amp and clips it at ±1. At 0 dB a signal below full scale
  stays within 0.01 dB of a render without Drive.
- **BODY-06b** On a steady A0 sine at Drive 12 dB the 3rd harmonic is above -40 dB and the 2nd below -60 dB.
- **BODY-06c** Drive leaves a low-level tail clean, because it comes after the amp: on the BODY-08 values, the tail's
  3rd harmonic stays below -60 dB, where the sketch measured -117 dB.
- **BODY-06d** Alias limit, proposed: on a 200 Hz sine at Drive 24 dB at 48 kHz, energy away from the harmonics
  stays at least 80 dB below the total. A clip's corners fold back at any oversampling for a high tone, so this limit
  is for the low tones a body plays. 200 Hz alone cannot fail: it repeats every 240 samples at 48 kHz, so its aliases
  land on its harmonics, and a bare clip passes. So the limit also holds for each note from A#1 to D3; a bare clip
  reaches -74 dB on G2. Below about 120 Hz `inharmonicDb`'s mask covers the whole spectrum and it reads nothing.
- **BODY-06e** Zero latency (ENG-02): the reported latency stays 0 and the first sound stays on the note's sample.
  The sketch's linear-phase filters are not an answer to this: use minimum-phase oversampling or anti-derivative
  anti-aliasing, chosen in VS1-03. If 80 dB cannot be reached that way, VS1-03 reports the best it reaches and stops for the owner.
  The output is exactly 0 before the note's sample and starts within two samples of it. What goes through the clip is
  delayed by less than 0.1 ms (5 samples at 48 kHz) against Drive 0.
- **BODY-06f** ENG-01 holds: a full-range jump during a tail stays below a step of 0.05 at a level below clipping.

### BODY-07 Tail anchor (VS1-04)

**What can stay identical.** The oscillator's phase is Start plus every cycle played since the note-on. A pitch knob
changes how many cycles the sweep plays, so in the classic behaviour the tail lands on a different phase with every
move: from the defaults at A0 the tail stands at 0.32 of a cycle, at Amount 48 at 0.04, at Pitch Decay 20 ms at 0.49
(a model of the voice, not a measured render). The sweep itself cannot stay identical: it is what the knob changes. So the anchor holds the part after the
sweep: its phase becomes `Start + Tune Hz * t`, as if the kick had played Tune from the note-on, whatever the sweep did.

**Why the sweep bends.** Start also fixes the phase at the note-on (BODY-10), and the sweep plays a number of cycles
that is rarely whole. Both ends can hold only if the sweep gains or loses up to half a cycle. The anchor multiplies its
frequency by `1 + e * (1 - t/W)`, with `e` set per note so that the kick lands on the anchored phase. The bend is largest at the note-on,
where the pitch is high, and gone at `W`. `W` is the later of Pitch Decay and 50 ms, or with Pitch Amount 0 the later of
Sweep 2 Decay and 50 ms. A second stage that outlasts `W` keeps its own pitch: its remaining cycles are counted in `e`.

| Id | Knob | Range | Default | Knob feel |
| --- | --- | --- | --- | --- |
| `body_tail_anchor` | Tail Anchor | off, on | on | switch, read at the note-on as Start is |

"Pitch knobs" are Pitch Amount, Pitch Decay, Pitch Shape, Sweep 2 Amount and Sweep 2 Decay. Tune and Start move the
tail on purpose. Wave and the amp knobs do not touch the phase, with the anchor on or off. In BODY-07b, c and e "the
bare voice" is Drive 0 with the filter open, because Drive and the filter have a memory (BODY-07g).

- **BODY-07a** Off is classic: with Tail Anchor off the bare voice is sample-identical to an oscillator fed the pitch of
  BODY-09a from Start, for several settings of the pitch knobs.
- **BODY-07b** All sweeps done: two renders of the bare voice with Tail Anchor on, differing in any pitch knobs, are
  sample-identical (exactly equal floats) from `T + 50 ms`, where `T` is the latest `W` and Sweep 2 Decay of the two.
- **BODY-07c** Second stage still running: if the two differ only in Pitch Amount, Decay or Shape, they also agree from
  the later `W + 50 ms` until BODY-07b takes over, with the largest difference below -80 dB (measured in VS1-04: one float step, about -144 dB). At the defaults that is the audible tail, from 100 ms, where BODY-07b alone starts at 350 ms, where the amp stands at -54 dB.
- **BODY-07d** Start holds: with Tail Anchor on the first sample is the same as with it off, for any pitch knobs. BODY-10a
  holds at Start 0 and 90.
- **BODY-07e** The bend is limited: the phase gained or lost by the bend is at most half a cycle. Pitch follows BODY-09a from
  `W` on within 1 cent, and before it within `1200 * log2 (1 + e)` cents. Without a sweep (both Amounts 0) `e` is 0
  and on matches off within -120 dB: on takes its tail's phase from the clock, as BODY-07b needs, and off adds it up. Limit, proposed: at A0, for Amount 12..72, Decay 5..160 ms, Shape -50..+60, Sweep 2
  Amount 0 or 6, the bend at the note-on stays below 600 cents. The model gives a median of 82; measured, the maximum is 534.
- **BODY-07f** The defaults keep their numbers: BODY-08a, b and c hold with Tail Anchor on. BODY-02d, BODY-03 and
  BODY-09 are measured with it off; the readout does not count the bend. The default kick's bend is -62 cents at the note-on, measured, and -38 cents at 20 ms in
  the model.
- **BODY-07g** Drive and the filter remember what went in, so after them the tails converge instead of being identical. At the
  defaults the difference between two renders of BODY-07b falls below -80 dB within 50 ms of `T + 50 ms` (measured in VS1-04: exactly 0
  from `T + 50 ms`, with the kick held at full level through the clip). With the filter closed it rings for as long as the filter rings; no limit.
- **BODY-07h** Key Track: the note that plays takes Tune's place. A note with Key Track on is sample-identical to the same
  note on Tune with it off. Two different notes have different pitches, so there is nothing to compare.
- **BODY-07i** ENG-04: the tail's phase against `Start + Tune Hz * t` stays within 0.01 cycle at 44.1, 48, 96 and 192 kHz.
- **BODY-07j** A knob moved during the sweep cannot be foreseen, so that kick misses its landing. What it misses slides away
  within 50 ms, and ENG-01c holds: no step above 0.05. ENG-01d holds: the calculation at the note-on has a fixed cost and does not
  allocate. The switch moved during a kick works from the next note, without a click.
- **BODY-07k** A project saved before this change keeps its sound (ENG-06a): state version 3, and a state with knobs but
  without this one gets Tail Anchor off. A VS0-01 state holds no knobs and takes the default, on, as BODY-08d says.

### BODY-08 New defaults (VS1-10)

The owner's choice in round 8. The note and the amp stay as they are.

| Knob | Today | New |
| --- | --- | --- |
| Pitch Amount | 36 | 60 |
| Pitch Decay | 100 ms | 40 ms |
| Pitch Shape | 0 | +30 |
| Sweep 2 Amount (BODY-09) | | 6 st |
| Sweep 2 Decay (BODY-09) | | 300 ms |
| Start (BODY-10) | | 90 degrees |
| Drive | 0 dB | 6 dB |

- **BODY-08a** A kick at the defaults measures as the sketch's `chain-hard6` did, within 50 cents or 10 %: 89 Hz at 20
  ms, 61 Hz at 100 ms, 56 Hz at 200 ms; -6 dB at 134 ms and -20 dB at 241 ms; 6.9 % of its energy between 300 Hz and
  1 kHz in 5-20 ms, within 2 percentage points.
- **BODY-08b** Its loudness (HARN-07a) is -6.6 LUFS at velocity 127, within 0.5 LU, and `step_1ms` above -10 dB.
- **BODY-08c** It ends without a click, with the sweep on: the largest step in its last 5 ms stays below that of a
  sine at full level.
- **BODY-08d** A project saved before this change keeps the values it was saved with (ENG-06a), including the new
  knobs, which take the value that leaves its sound as it was: Sweep 2 Amount 0, Start 0, Drive 0. A knob the project
  did save keeps its value, so VS1-03's Drive stays. A VS0-01 project saved no knobs and has no sound of its own to keep,
  so it takes the defaults, as ENG-06a says.
- **BODY-08e** With the panel from VS1-06 the CORE-01 test runs on these defaults (VS1-07).

### BODY-09 A sweep in two stages (VS1-10)

| Id | Knob | Range | Default | Knob feel |
| --- | --- | --- | --- | --- |
| `body_sweep2_amount` | Sweep 2 Amount | 0..24 semitones | 6 (BODY-08) | linear |
| `body_sweep2_decay` | Sweep 2 Decay | 1..2000 ms | 300 ms | centre at 200 ms |

- **BODY-09a** The pitch is `Tune + Amount * g(pitch) + Sweep 2 Amount * g(sweep 2)`, with `g` the curve of this module
  at Shape 0 for Sweep 2. With Amount 12, Decay 50 ms, Sweep 2 3 st and 650 ms, it follows that within 50 cents at
  10, 75, 300 and 500 ms.
- **BODY-09b** The kick ends on Tune, within 0.5 %, once both have passed. Sweep 2 Amount 0 is sample-identical to
  a render without this stage.
- **BODY-09c** BODY-02d's readout takes the second stage into account, within 10 cents of a measured render.
- **BODY-09d** ENG-01 holds for both knobs.

### BODY-10 Start phase (VS1-10)

| Id | Knob | Range | Default | Knob feel |
| --- | --- | --- | --- | --- |
| `body_start` | Start | 0..90 degrees | 90 (BODY-08) | linear |

- **BODY-10a** Start is the oscillator's phase at the note-on. At 90 with Attack 0 the first sample is at full level,
  and `step_1ms` is above -10 dB; at 0 it behaves as BODY-01c and BODY-04a say, which hold only for Start 0.
- **BODY-10b** The step at the note-on is the sound, not a fault: at 90 it is a single hit from silence on the note's
  exact sample. Retriggering a ringing kick is PLAY-03's to settle (VS1-05).
- **BODY-10c** ENG-03 and ENG-04 hold at every Start. With Drive on below Start 90, measured: pitch and loudness hold,
  and the peak moves by up to 0.75 dB between 44.1 and 192 kHz (+0.89 against +0.14 dBFS at Start 0, Drive 6), because
  the clip's first corners fall high in the sweep. At the default Start 90 it stays within 0.01 dB of +1.17 dBFS. The owner accepted
  this (2026-11-04).

### Engineering, as it applies to BODY

- **ENG-01a** All parameters are automatable by the host.
- **ENG-01b** Every parameter is smoothed per sample. A knob jump settles within 1 % of its range within
  50 ms, at every sample rate. A knob moved while no kick rings holds in full from the next kick's
  first sample, without a glide.
- **ENG-01c** A full-range jump of any knob during a ringing tail makes no click: the largest step
  stays below 0.05 (a sine at C3 has 0.034 of its own). Wave is tested from sine to triangle, because a
  saw and a square have large steps by nature. Pitch knobs are tested with the sweep still running at
  a low pitch.
- **ENG-01d** `processBlock` does not allocate while parameters change.
- **ENG-01e** Each knob's text survives a round trip, as clap-validator demands: value to text, back to
  a value and to text again gives the same text. Times, semitones, Drive and Env Amount show one decimal,
  Shape and Resonance whole percents, Cutoff whole Hz, Start whole degrees and Wave two decimals.
- **ENG-06a** The state restores all parameters. A state that lacks a parameter (an older version,
  or a VS0-01 project) gives that parameter its default, except where BODY-08d says otherwise. The state keeps its version, and
  `setStateInformation` runs saved states through a migration step that later versions can extend.
- **ENG-06b** A test lists the ids as written above, so renaming one fails.
- **ENG-03, ENG-04** hold with the new voice, also at non-default knob values.
