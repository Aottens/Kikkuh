# Reference kicks: what BODY lacks

Written for VS1-08 (HARN-06). Measured on 2026-11-04 with `kikkuh-measure`; BODY rendered with
`kikkuh-render` at the values in each table. These are numbers. Whether anything sounds good is the
owner's call, in VS1-09.

## The references

The owner chose four by ear from the kicks already on this Mac (2026-11-04). The audio stays outside
the repo.

| Kick | File, under `~/` | Format | Licence | SHA-256 |
| --- | --- | --- | --- | --- |
| KICK09 | `Downloads/Progressive_Techno_A_Force_Truly_Evil/SAMPLES/KICK09.wav` | mono, 16-bit, 48 kHz | unknown, no licence file in the pack | `170ca7667c2317fe1f6227b6b94b294128d67dc52e02c98f2980dc6406856f2f` |
| Beefy SM ELE | `Library/Application Support/Bitwig/Bitwig Studio/installed-packages/5.0/Bitwig/Bitwig Drum Machines/Sample Magic Drums/Kick Beefy SM ELE.wav` | stereo, 24-bit, 44.1 kHz | Bitwig package, terms not read | `7eee9fa24df1faa27abde5518b91df8d1d2abde25a2cc3b5c8b46eed4734cd40` |
| Phat Valid | `Library/Application Support/Bitwig/Bitwig Studio/installed-packages/5.0/Bitwig/Bitwig Drum Machines/Kicks/Kick Phat Valid.wav` | stereo, 24-bit, 44.1 kHz | Bitwig package, terms not read | `7257f61afdcaf3eb4f15f87e3ee65016f438a704eb03261f3fc3641ee24ae16c` |
| Phat Crotchety | `Library/Application Support/Bitwig/Bitwig Studio/installed-packages/5.0/Bitwig/Bitwig Drum Machines/Kicks/Kick Phat Crotchety.wav` | stereo, 24-bit, 44.1 kHz | Bitwig package, terms not read | `390f0e372a099363bc58e4e2bf007b293f4049e42fa24736f469466f8825c2c2` |

## What BODY lacks

BODY here is BODY-01..04, at the values the fit found, with Hold set to the reference's `peak_ms`.

### Missing: no knob of BODY-01..04 gets there

1. **A hard start.** The largest step in the first 1 ms (`step_1ms`, relative to the peak) is -2.4,
   -4.5, -7.3 and -12.7 dB on the references and -25.5, -31.1, -32.0 and -31.4 dB on BODY: 19 to 27 dB
   smaller. Energy above 1 kHz in the first 5 ms is 6.3, 78.6, 37.1 and 0.8 % on the references and 0.0 %
   on BODY.
   Tried on Beefy, which has the most: a saw gives 16.1 % with a step of -36.6 dB, and a square 9.9 % with
   -13.5 dB, but either keeps its harmonics through the whole kick. Pitch Amount 72 with Decay 5 ms gives
   0.1 %. BODY-05 (filter) cannot add what is not there; BODY-06 (drive) might raise the step, not
   measured. LAYER-01 (Click, slice 2) is the planned place.
2. **A sweep in two stages.** KICK09 falls to 51 Hz by 75 ms, then keeps falling slowly to 43 Hz at about
   650 ms. BODY's one curve ends on Tune, so the nearest fit leaves 97 cents, with Shape stuck at its
   limit of +100. BODY reads 172 cents low at 150 ms, 144 at 200 ms and 85 at 300 ms. Phat Valid has a
   milder form: 40 Hz at 96 ms, 34 Hz at 261 ms, BODY 112 to 114 cents low at 100 and 150 ms, fit error 96 cents.
   Phat Crotchety has one stage: the fit leaves 19 cents, and BODY stays within 37 cents from 10 to 500 ms.
   No planned criterion covers this; BODY-05 and 06 do not touch pitch.
3. **Tuning between notes.** The tail sits -42, -41, +44 and -22 cents from the nearest note, and Tune has
   whole notes only (BODY-02). Phat Crotchety's -22 is the one to trust: KICK09 and Phat Valid are still
   falling there, and Beefy's tail is not readable (see below).
4. **An uneven level in the first 20 ms.** Phat Valid's peak per window is -0.9, -10.0, -0.5, -4.9 and
   -0.7 dB for 0-1, 1-2, 2-5, 5-10 and 10-20 ms, with its peak at 30 ms. BODY's is -0.7, 0, 0, 0 and 0 dB.
   Attack and Hold make one rise, not a dip.

### Reachable: other knob values get there

5. **The level over time.** The fit leaves 1.1, 2.8, 2.4 and 1.3 dB. With the values in the tables, BODY
   crosses -6, -12, -20 and -40 dB within 23 ms of each reference. Beefy needs Amp Shape at its limit of
   -100, and Hold at the reference's `peak_ms`.
6. **A soft first millisecond.** Phat Crotchety reads -10.2 and -5.9 dB in 0-1 and 1-2 ms. Amp Attack 4 ms
   with Hold 0 gives -11.8 and -8.9 dB. KICK09's -3.6 and -1.2 dB lie near Attack 2 ms with Hold 1 ms,
   which gives -8.8 and -0.1 dB; a shorter Attack was not tried.
7. **The fast part of the sweep.** From 10 to 75 ms BODY stays within 82 cents on KICK09, 105 on Phat Valid and
   32 on Phat Crotchety. On Beefy it stays within 17 cents from 30 to 75 ms, and 20 ms is 167 cents high.

### Not lacking

8. **Tail harmonics.** KICK09's strongest is -67 dB and Phat Crotchety's -85 dB, against -111 and -114 dB
   on BODY. The references hold more, but both are below the -60 dB that BODY-01a calls a clean sine.
   After 5 ms at most 2.1 % of a reference's energy lies above 300 Hz, and BODY reaches 2.7 %. Beefy and
   Phat Valid have tails too short to read. So these four kicks show no measured need for saturation in
   the tail. The owner turned down Hard Relentless C, the one with strong tail harmonics (-16 dB).
9. **Stereo.** Beefy's side is -45 dB, -37 dB in the first 20 ms; the others are below -80 dB. BODY is mono.

### For VS1-09

Items 1 to 4 are what variants could test. Item 8 says saturation variants have the least support in
these numbers; that is not a reason to skip them, because the owner's ears decide.

## How far to trust the numbers

- Pitch was checked with a separate method: intervals between rising zero crossings, with no code shared
  with `tests/Measure.h`. KICK09, Phat Valid and Phat Crotchety agree from 5 ms on, for example KICK09 at
  11 ms: 215 Hz crossings, 229 Hz at 10 ms measured.
- **Beefy's sweep is the weakest.** `pitch_5ms` (1146 Hz) reads the click, and `pitch_10ms` (122 Hz) is
  wrong: zero crossings give 217 Hz at 8 ms and 144 Hz at 14 ms. `pitch_150ms` (133 Hz) reads a real
  glitch of a few ms in the sample. The fit left out 28 of 80 readings, so its 78 cents hold for the rest.
- Beefy's `tail_hz` (56.9 Hz) is not a steady tail. The level drops fast, and zero crossings wander from
  56 to 64 Hz and down to 45 Hz between 100 and 218 ms.
- Tail harmonics are measured over a stretch where the pitch may still move by a semitone, which can read
  high harmonics low. The ruler is the same on both sides.
- Pitch readings assume that the fundamental is the strongest part. After 5 ms at least 97.9 % of each
  reference's energy lies below 300 Hz, so that holds here.

## The numbers

Reading `bands_*`: percent below 300 Hz, 300 Hz-1 kHz, 1-4 kHz and above 4 kHz. Levels are in dB relative
to the peak. Each BODY column is measured from the render by the same command, and its `fit_*` values are
the fit on that render.

### KICK09

```
build/kikkuh-render --out body.wav --length 2 body_tune=F0 body_pitch_amount=38.74 body_pitch_decay=320.4 body_pitch_shape=100.0 body_amp_hold=3.17 body_amp_decay=1841.8 body_amp_shape=84.1
```

| Measure | Reference | Nearest BODY |
| --- | --- | --- |
| `peak_dbfs` | -0.60 | 0.00 |
| `tail_hz` | 42.60 | 43.65 |
| `tail_note` | F0 | F0 |
| `pitch_5ms` | 298.33 | 304.79 |
| `pitch_10ms` | 229.37 | 234.96 |
| `pitch_20ms` | 149.24 | 152.88 |
| `pitch_30ms` | 108.32 | 109.96 |
| `pitch_50ms` | 67.70 | 70.97 |
| `pitch_75ms` | 51.13 | 53.45 |
| `pitch_100ms` | 50.16 | 47.18 |
| `pitch_150ms` | 48.58 | 43.98 |
| `pitch_200ms` | 47.46 | 43.66 |
| `pitch_300ms` | 45.85 | 43.65 |
| `pitch_500ms` | 43.83 | 43.64 |
| `harmonic_2` | -67.3 | -111.3 |
| `harmonic_3` | -71.5 | -112.7 |
| `harmonic_4` | -90.9 | -115.3 |
| `harmonic_5` | -80.9 | -117.3 |
| `harmonic_6` | -99.1 | -119.0 |
| `harmonic_7` | -85.3 | -120.4 |
| `harmonic_8` | -105.4 | -121.6 |
| `peak_ms` | 3.17 | 0.60 |
| `step_1ms` | -2.4 | -25.5 |
| `peak_0_1ms` | -3.6 | 0.0 |
| `peak_1_2ms` | -1.2 | 0.0 |
| `peak_2_5ms` | 0.0 | 0.0 |
| `peak_5_10ms` | -1.9 | -0.1 |
| `peak_10_20ms` | -1.9 | -0.3 |
| `bands_0_5ms` | 6.6 87.1 6.2 0.1 | 35.1 64.9 0.0 0.0 |
| `bands_5_20ms` | 97.9 2.1 0.0 0.0 | 97.3 2.7 0.0 0.0 |
| `bands_20_50ms` | 100.0 0.0 0.0 0.0 | 100.0 0.0 0.0 0.0 |
| `bands_50_200ms` | 100.0 0.0 0.0 0.0 | 100.0 0.0 0.0 0.0 |
| `below_6db_ms` | 162.79 | 155.52 |
| `below_12db_ms` | 312.02 | 304.27 |
| `below_20db_ms` | 478.94 | 486.75 |
| `below_40db_ms` | 853.48 | 842.19 |
| `below_60db_ms` | 967.96 | 1105.60 |
| `fit_tune` | F0 | F0 |
| `fit_tune_error_cents` | -42.1 | -0.3 |
| `fit_pitch_amount` | 38.74 | 38.37 |
| `fit_pitch_decay_ms` | 320.4 | 233.3 |
| `fit_pitch_shape` | 100.0 | 67.7 |
| `fit_pitch_error_cents` | 97.4 | 9.9 |
| `fit_pitch_outliers` | 0 | 0 |
| `fit_amp_decay_ms` | 1841.8 | 1751.1 |
| `fit_amp_shape` | 84.1 | 77.1 |
| `fit_amp_error_db` | 1.06 | 0.23 |
| `side_db` | -234.0 | -234.5 |
| `side_0_20ms_db` | -224.7 | -226.6 |

### Beefy SM ELE

```
build/kikkuh-render --out body.wav --length 2 body_tune=A#0 body_pitch_amount=22.42 body_pitch_decay=139.7 body_pitch_shape=13.6 body_amp_hold=4.10 body_amp_decay=234.3 body_amp_shape=-100.0
```

| Measure | Reference | Nearest BODY |
| --- | --- | --- |
| `peak_dbfs` | -1.00 | 0.00 |
| `tail_hz` | 56.92 | 58.28 |
| `tail_note` | A#0 | A#0 |
| `pitch_5ms` | 1145.85 | 182.90 |
| `pitch_10ms` | 121.95 | 158.50 |
| `pitch_20ms` | 112.90 | 124.33 |
| `pitch_30ms` | 102.37 | 101.68 |
| `pitch_50ms` | 76.14 | 76.90 |
| `pitch_75ms` | 63.31 | 63.69 |
| `pitch_100ms` | 55.65 | 59.21 |
| `pitch_150ms` | 132.55 | 58.23 |
| `pitch_200ms` | 50.74 | 58.27 |
| `pitch_300ms` | - | - |
| `pitch_500ms` | - | - |
| `harmonics` | tail too short | tail too short |
| `peak_ms` | 4.10 | 3.71 |
| `step_1ms` | -4.5 | -31.1 |
| `peak_0_1ms` | -1.8 | -0.3 |
| `peak_1_2ms` | -2.3 | 0.0 |
| `peak_2_5ms` | 0.0 | 0.0 |
| `peak_5_10ms` | 0.0 | -0.1 |
| `peak_10_20ms` | 0.0 | -0.3 |
| `bands_0_5ms` | 0.2 21.2 52.9 25.7 | 77.5 22.5 0.0 0.0 |
| `bands_5_20ms` | 98.5 0.3 0.6 0.7 | 100.0 0.0 0.0 0.0 |
| `bands_20_50ms` | 99.8 0.0 0.0 0.1 | 100.0 0.0 0.0 0.0 |
| `bands_50_200ms` | 100.0 0.0 0.0 0.0 | 100.0 0.0 0.0 0.0 |
| `below_6db_ms` | 118.28 | 115.81 |
| `below_12db_ms` | 195.28 | 175.98 |
| `below_20db_ms` | 214.72 | 210.90 |
| `below_40db_ms` | 235.24 | 235.92 |
| `below_60db_ms` | 237.44 | 237.92 |
| `fit_tune` | A#0 | A#0 |
| `fit_tune_error_cents` | -40.6 | 0.3 |
| `fit_pitch_amount` | 22.42 | 22.48 |
| `fit_pitch_decay_ms` | 139.7 | 139.8 |
| `fit_pitch_shape` | 13.6 | 14.1 |
| `fit_pitch_error_cents` | 77.8 | 3.4 |
| `fit_pitch_outliers` | 28 | 0 |
| `fit_amp_decay_ms` | 234.3 | 234.3 |
| `fit_amp_shape` | -100.0 | -98.0 |
| `fit_amp_error_db` | 2.82 | 0.45 |
| `side_db` | -44.8 | -233.0 |
| `side_0_20ms_db` | -37.1 | -226.6 |

### Phat Valid

```
build/kikkuh-render --out body.wav --length 2 body_tune=C#0 body_pitch_amount=29.57 body_pitch_decay=188.8 body_pitch_shape=37.5 body_amp_hold=30.45 body_amp_decay=317.0 body_amp_shape=-30.4
```

| Measure | Reference | Nearest BODY |
| --- | --- | --- |
| `peak_dbfs` | -0.01 | 0.00 |
| `tail_hz` | 35.54 | 34.67 |
| `tail_note` | C#0 | C#0 |
| `pitch_5ms` | 164.57 | 158.73 |
| `pitch_10ms` | 139.80 | 131.59 |
| `pitch_20ms` | 97.65 | 96.77 |
| `pitch_30ms` | 79.67 | 75.56 |
| `pitch_50ms` | 51.28 | 52.98 |
| `pitch_75ms` | 42.21 | 41.18 |
| `pitch_100ms` | 39.07 | 36.59 |
| `pitch_150ms` | 37.01 | 34.70 |
| `pitch_200ms` | 35.62 | 34.65 |
| `pitch_300ms` | - | - |
| `pitch_500ms` | - | - |
| `harmonics` | tail too short | tail too short |
| `peak_ms` | 30.45 | 15.48 |
| `step_1ms` | -7.3 | -32.0 |
| `peak_0_1ms` | -0.9 | -0.7 |
| `peak_1_2ms` | -10.0 | 0.0 |
| `peak_2_5ms` | -0.5 | 0.0 |
| `peak_5_10ms` | -4.9 | 0.0 |
| `peak_10_20ms` | -0.7 | 0.0 |
| `bands_0_5ms` | 23.1 39.7 36.4 0.7 | 82.1 17.9 0.0 0.0 |
| `bands_5_20ms` | 99.8 0.2 0.0 0.0 | 100.0 0.0 0.0 0.0 |
| `bands_20_50ms` | 100.0 0.0 0.0 0.0 | 100.0 0.0 0.0 0.0 |
| `bands_50_200ms` | 100.0 0.0 0.0 0.0 | 100.0 0.0 0.0 0.0 |
| `below_6db_ms` | 104.35 | 110.10 |
| `below_12db_ms` | 157.78 | 180.52 |
| `below_20db_ms` | 242.09 | 238.46 |
| `below_40db_ms` | 305.51 | 310.25 |
| `below_60db_ms` | 333.33 | 329.38 |
| `fit_tune` | C#0 | C#0 |
| `fit_tune_error_cents` | 44.1 | 1.3 |
| `fit_pitch_amount` | 29.57 | 29.70 |
| `fit_pitch_decay_ms` | 188.8 | 194.4 |
| `fit_pitch_shape` | 37.5 | 41.2 |
| `fit_pitch_error_cents` | 95.5 | 4.6 |
| `fit_pitch_outliers` | 1 | 0 |
| `fit_amp_decay_ms` | 317.0 | 328.3 |
| `fit_amp_shape` | -30.4 | -35.4 |
| `fit_amp_error_db` | 2.39 | 0.62 |
| `side_db` | -81.6 | -233.4 |
| `side_0_20ms_db` | -82.8 | -226.7 |

### Phat Crotchety

```
build/kikkuh-render --out body.wav --length 2 body_tune=A0 body_pitch_amount=22.88 body_pitch_decay=482.8 body_pitch_shape=28.0 body_amp_hold=4.04 body_amp_decay=1298.5 body_amp_shape=51.1
```

| Measure | Reference | Nearest BODY |
| --- | --- | --- |
| `peak_dbfs` | -0.01 | 0.00 |
| `tail_hz` | 54.32 | 55.00 |
| `tail_note` | A0 | A0 |
| `pitch_5ms` | 180.23 | 194.57 |
| `pitch_10ms` | 182.11 | 185.14 |
| `pitch_20ms` | 166.13 | 167.17 |
| `pitch_30ms` | 151.78 | 152.05 |
| `pitch_50ms` | 127.41 | 128.07 |
| `pitch_75ms` | 105.25 | 106.74 |
| `pitch_100ms` | 90.15 | 91.84 |
| `pitch_150ms` | 71.89 | 73.46 |
| `pitch_200ms` | 62.90 | 63.84 |
| `pitch_300ms` | 56.36 | 56.40 |
| `pitch_500ms` | 54.41 | 55.00 |
| `harmonic_2` | -85.1 | -114.4 |
| `harmonic_3` | -88.2 | -114.5 |
| `harmonic_4` | -96.2 | -116.3 |
| `harmonic_5` | -109.5 | -119.0 |
| `harmonic_6` | -116.7 | -119.9 |
| `harmonic_7` | -114.6 | -122.0 |
| `harmonic_8` | -110.5 | -122.5 |
| `peak_ms` | 4.04 | 3.69 |
| `step_1ms` | -12.7 | -31.4 |
| `peak_0_1ms` | -10.2 | -0.4 |
| `peak_1_2ms` | -5.9 | 0.0 |
| `peak_2_5ms` | 0.0 | 0.0 |
| `peak_5_10ms` | -2.4 | -0.1 |
| `peak_10_20ms` | -1.7 | -0.3 |
| `bands_0_5ms` | 66.5 32.7 0.3 0.5 | 76.8 23.2 0.0 0.0 |
| `bands_5_20ms` | 99.7 0.3 0.0 0.0 | 99.9 0.1 0.0 0.0 |
| `bands_20_50ms` | 99.7 0.3 0.0 0.0 | 100.0 0.0 0.0 0.0 |
| `bands_50_200ms` | 100.0 0.0 0.0 0.0 | 100.0 0.0 0.0 0.0 |
| `below_6db_ms` | 179.25 | 159.75 |
| `below_12db_ms` | 287.32 | 297.90 |
| `below_20db_ms` | 440.50 | 460.46 |
| `below_40db_ms` | 781.47 | 760.29 |
| `below_60db_ms` | 909.73 | 951.44 |
| `fit_tune` | A0 | A0 |
| `fit_tune_error_cents` | -21.7 | -0.1 |
| `fit_pitch_amount` | 22.88 | 22.92 |
| `fit_pitch_decay_ms` | 482.8 | 493.6 |
| `fit_pitch_shape` | 28.0 | 30.6 |
| `fit_pitch_error_cents` | 18.5 | 4.0 |
| `fit_pitch_outliers` | 1 | 0 |
| `fit_amp_decay_ms` | 1298.5 | 1292.9 |
| `fit_amp_shape` | 51.1 | 50.4 |
| `fit_amp_error_db` | 1.29 | 0.09 |
| `side_db` | -85.7 | -234.5 |
| `side_0_20ms_db` | -90.8 | -226.6 |

