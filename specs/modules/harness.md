# Module spec: harness (HARN)

Status: **HARN-01..03 built with VS0-01. HARN-04 approved by the owner (2026-11-03), built by VS1-02. HARN-05 and HARN-06 approved by the owner (2026-11-03), built by VS1-08; the owner chose four references (2026-11-04). HARN-07..09 approved by the owner (2026-11-04), built by VS1-09.**

## Acceptance criteria

- **HARN-01** `tests/Measure.h` measures peak, first sound, largest step and frequency; each is checked on a known signal.
- **HARN-02** `scripts/validate.sh` runs pluginval at strictness 10, clap-validator, auval and an architecture check on the built plugin; CI and local runs use the same script.
- **HARN-03** `scripts/check-tracking.py` runs in CI and fails on stale tracking, as in `CLAUDE.md`.
- **HARN-04** `kikkuh-render` writes a WAV from given parameter values, without a DAW. It plays one
  note through the same `PluginProcessor` and the same render loop as the tests:

  ```
  kikkuh-render --out kick.wav [--length 3] [--rate 48000] [--note 36] [--velocity 127] [id=value ...]
  ```

  - **HARN-04a** Parameter values: `id=value` uses the ids of `specs/modules/body.md`, in the knob's
    own units (`body_pitch_decay=40` is 40 ms). `body_tune` also takes a note name (`body_tune=F#0`).
    A parameter that is not given stays at its default. A render holds the values in full from its
    first sample, without a glide (ENG-01b).
  - **HARN-04b** Output: the WAV is stereo, as the plugin's output, in 32-bit float, so a peak above
    1.0 is kept and a later level match loses nothing. An existing file is replaced completely.
  - **HARN-04c** Same audio as a host: the file is sample-identical to a render of the same values
    through `tests/Render.h` in a test. The same command twice gives identical samples.
  - **HARN-04d** Length and sample rate: `--length` in seconds (default 3, longer than the longest
    kick) gives exactly `length * rate` samples, rounded down. `--rate` is 44100 to 192000 (default
    48000) and is written in the file's header.
  - **HARN-04e** Note and velocity: the note starts on sample 0. `--velocity` is 1..127 (default 127)
    and scales the level linearly. `--note` is 0..127 (default 36); today every note plays the same
    kick (PLAY-02 off), and it starts to matter with VS1-05.
  - **HARN-04f** Errors: each of these exits non-zero, writes a message naming the bad input to
    stderr and leaves no new file: an unknown parameter id (the message lists the valid ids), a value
    that is not a number or note, a value outside the knob's range (never clamped silently), a length
    of 0 or less, a rate or note or velocity out of range, a missing `--out`, an unknown option, or an
    output path that cannot be written.

  Not in this criterion: level matching and batches, which belong to the listening rounds (VS1-09).
  One render per call in a shell loop is the way until a round shows it is too slow.
- **HARN-05** `kikkuh-measure` prints the numbers of a kick from a WAV or AIFF, without a DAW, so a
  reference and a BODY render are measured by the same ruler. Each new measurement lives in
  `tests/Measure.h`, with a check on a known signal in `tests/MeasureCheck.cpp`.

  ```
  kikkuh-measure kick.wav [more.wav ...]
  ```

  - **HARN-05a** Time zero: time 0 is the first sample above -60 dB of the file's peak, so silence in
    front changes no number. Levels are in dB relative to that peak; the peak itself is printed in
    dBFS. Measured on the mid `(L + R) / 2`, except for HARN-05f.
  - **HARN-05b** Sweep: the pitch in Hz at 5, 10, 20, 30, 50, 75, 100, 150, 200, 300 and 500 ms, and
    the tail's frequency and note. On a known sine sweep with a 2nd and 3rd harmonic added, each value
    is within 50 cents during the sweep and within 0.5 % on a steady tail. A time where the level has
    fallen below -40 dB prints no pitch.
  - **HARN-05c** Harmonics: the levels of harmonics 2 to 8 relative to the fundamental, in the tail,
    from where the pitch stays within 1 semitone of the tail until -40 dB. If that stretch is shorter
    than 8 periods the tool says so and prints none. On a known tone each level is within 1 dB.
  - **HARN-05d** First milliseconds: the time of the peak, the largest step in the first 1 ms, the
    peak in 0-1, 1-2, 2-5, 5-10 and 10-20 ms, and each window's energy split over the bands below
    300 Hz, 300 Hz-1 kHz, 1-4 kHz and above 4 kHz, in percent, for 0-5, 5-20, 20-50 and 50-200 ms. A
    known 5 ms burst at 2 kHz puts over 90 % in the 1-4 kHz band of its window.
  - **HARN-05e** Decay and nearest BODY: the time at which the level falls below -6, -12, -20, -40 and
    -60 dB for good, and the nearest values of BODY's own curve (`specs/modules/body.md`): Tune, Pitch
    Amount, Pitch Decay and Pitch Shape from the sweep, Amp Decay and Amp Shape from the level, each
    with its remaining error in cents or dB. On a signal made with known values of that curve, the
    fit gives those values back, within 10 % or 1 semitone. A click or a short glitch does not pull the fit: pitch
    readings that stand far from their neighbours are left out, and the tool prints how many
    (`fit_pitch_outliers`). The same values come back, within the same limits, from that signal with a
    2 ms noise click louder than the kick and a 4 ms glitch in the tail. Found measuring the owner's
    references: one click moved a fit by 14 semitones of Amount.
  - **HARN-05f** Stereo: the side level relative to the mid, over the whole kick and over 0-20 ms. A
    dual-mono file reads below -100 dB.
  - **HARN-05g** Errors: a missing or unreadable file, or no file named, exits non-zero with a message
    naming the input on stderr.

- **HARN-06** The comparison: `docs/research/reference-kicks.md` holds, for each of the owner's four
  references, its name, source, licence as far as known and SHA-256, the numbers of HARN-05 next to
  those of a `kikkuh-render` of the nearest BODY values, and the exact command of that render. A
  closing list says what BODY lacks. Every item has a measured number and is marked **reachable**
  (other knob values get there; say which) or **missing** (no knob of BODY-01..04 gets there; BODY-05
  and 06 named where they might). Its claims are about numbers; whether it sounds good stays the
  owner's, in VS1-09. Reference audio stays outside the repo unless the owner agrees and its licence
  is clear.

- **HARN-07** `kikkuh-round` turns one round file into blind, level-matched, repeatable audio, so the
  owner can choose by ear with `afplay`, without a DAW. A development tool, not shipped.

  ```
  kikkuh-round tests/manual/rounds/1-start.txt --out build/rounds/1 --seed 7
  ```

  A round file is plain text, in the repo, so a round can be rendered again from it and its seed:

  ```
  # comment
  base body_tune=A0 body_amp_decay=400          # applies to every option; an option overrides it
  option plain
  option soft sat_kind=soft sat_drive=12
  reference KICK09 ~/Downloads/.../KICK09.wav   # optional, may repeat
  ```

  - **HARN-07a** Level match: every option and reference is scaled to -20 LUFS, measured by a new
    `measure::loudness` in `tests/Measure.h`: ITU-R BS.1770 K-weighting over the 400 ms from time zero
    (HARN-05a), on both channels. So what is compared is the loudness of the hit, not its peak. Read back
    from the written files, all of a round lie within 0.1 LU of -20. An option that would peak above
    -1 dBFS is an error naming it, never limited, because a limiter would change the sound being judged.
    `MeasureCheck.cpp` checks the measure on known signals: a 997 Hz sine at 0 dBFS on both channels
    reads 0.0 LUFS within 0.1, the K filter's coefficients at 48 kHz match those printed in BS.1770
    within 0.0001, and the same 100 Hz sine reads the same within 0.1 at 44.1 and 192 kHz.
  - **HARN-07b** Unnamed: options are written as `A.wav`, `B.wav` ... in shuffled order. Files hold no
    option name, all files of a round have the same length (the longest kick until it falls below
    -90 dBFS, plus 50 ms), and stdout names no option. The letter of each option, with its loudness
    before the match and its gain, is only in `key.txt`, which Claude reads after the owner has answered.
  - **HARN-07c** Repeatable: `--seed` is required. The same file and seed give byte-identical output,
    and the seed used is written in the run file. An option's audio does not depend on the seed: under
    two seeds it is sample-identical before the letters are assigned. The shuffle is written out by
    hand on `std::mt19937`, because `std::shuffle` differs between standard libraries.
  - **HARN-07d** Output: 48 kHz, stereo, 32-bit float. A reference is written as `ref-<name>.wav`, named
    and at its own rate, level-matched but not shuffled: the owner may listen to it knowingly. The
    audio of a reference stays outside the repo.
  - **HARN-07e** Errors: each exits non-zero with a message naming the input on stderr and writes no
    audio: a missing file, seed or `--out`; an `--out` that exists and is not empty (never overwrite a
    round); an unknown key or a value out of range; a line that is none of the above; fewer than 2 or
    more than 6 options; a reference that cannot be read.

- **HARN-08** A sketch voice renders what BODY cannot do yet, in `tools/Sketch.h`, as plain C++ on
  `dsp::MorphOsc` and `dsp::bodyCurve`. It is for listening only: nothing in `source/` changes, and the
  plugin does not use it. What wins a round is built properly later, under the audio-thread rules.

  It takes BODY's nine ids, with the same meaning and ranges (`specs/modules/body.md`), and these keys:

  | Keys | Does |
  | --- | --- |
  | `tune_cents` (-100..100) | moves the whole pitch, between notes |
  | `sweep2_amount` (0..24 st), `sweep2_decay` (1..2000 ms), `sweep2_shape` (-100..100) | adds a second curve to the sweep; the sweep is `Tune + Amount * g1 + sweep2_amount * g2` |
  | `start_phase` (0..90 degrees) | the phase the oscillator starts at: 90 starts at full level |
  | `click_kind` (`tone`, `noise`), `click_hz` (500..12000), `click_ms` (0.5..20), `click_db` (-40..6) | a burst added to the voice, falling as `(1 - t / click_ms)^2`, silent from `click_ms`; noise comes from a fixed seed |
  | `dip_at`, `dip_width` (ms), `dip_db` (-40..0), and `dip2_at`, `dip2_width`, `dip2_db` | a raised-cosine dent in the level |
  | `sat_kind` (`soft`, `asym`, `hard`), `sat_drive` (0..36 dB), `sat_place` (`after_amp`, `before_amp`) | saturation, at 4 times the running rate when `after_amp`; `soft` is tanh, `asym` tanh with a bias whose DC is removed, `hard` a clip |
  | `filter_kind` (`svf12`, `svf24`, `ladder`), `filter_cutoff` (20..20000 Hz), `filter_res` (0..1), `filter_env` (0..8 octaves), `filter_decay` (1..2000 ms) | a low-pass; the cutoff starts `filter_env` octaves up and falls along BODY's curve at Shape 0 |

  Signal order: oscillator, amp (with `sat_place=before_amp` the saturation comes first), dents, click,
  saturation, filter. It runs at 192 kHz and is decimated to 48 kHz by a linear-phase filter with its
  delay removed, so saturation can be judged without aliasing. Linear phase and 4x are allowed here
  because this is offline. They are not an answer to ENG-02: VS1-03 must reach its choice at zero latency.

  - **HARN-08a** Plain means BODY: with none of the keys above, at 48 kHz without decimation, the sketch
    differs from `BodyVoice` by less than -90 dB relative to its peak, at the defaults and at values
    away from them.
  - **HARN-08b** Decimation: a 1 kHz sine passes within 0.1 dB, a 30 kHz sine falls at least 90 dB, and
    the output starts on the same sample as a kick rendered at 48 kHz, within one sample.
  - **HARN-08c** Sweep: `tune_cents=-22` lands within 3 cents of the note minus 22 cents. With
    `sweep2_amount`, the pitch follows the sum above within 50 cents at 10, 75, 300 and 500 ms, and ends on Tune.
  - **HARN-08d** Hard start: `start_phase=90` has a `step_1ms` above -10 dB, inside the references' -2.4 to -12.7 dB, and the plain kick stays below -20 dB. A `tone` click at 3 kHz and a
    `noise` click each raise the energy above 1 kHz in the first 5 ms to over 5 %, and leave the band
    shares of 50-200 ms unchanged within 1 percentage point.
  - **HARN-08e** Dents: the kick with a dent of -10 dB at 1.5 ms, 1 ms wide, divided by the same kick
    without it, reads -10 dB within 1 dB at 1.5 ms and 0 dB within 1 dB from 5 to 10 ms.
  - **HARN-08f** Saturation: on a steady sine `soft` and `hard` have a 3rd harmonic above -40 dB and a 2nd
    below -60 dB, and `asym` has a 2nd above -40 dB with DC below -60 dB, at drive 12 dB.
    With `sat_place=before_amp` the 3rd harmonic relative to the fundamental in the tail stays within
    3 dB of its level at the start; with `after_amp` it falls by at least 10 dB. Aliasing at drive 24 dB stays at least 80 dB
    below the fundamental, on a 5 kHz sine for `soft` and `asym`, and on a 200 Hz sine for `hard`: a clip
    has corners, whose harmonics fold back at any oversampling for a high tone.
  - **HARN-08g** Filter: on low-level sines with no env, `svf12` falls 12 dB per octave and `svf24` and
    `ladder` 24 dB per octave, within 3 dB, from 4 to 8 times the cutoff. At `filter_res=0` the -3 dB point
    of both SVFs is within 10 % of `filter_cutoff`. `filter_env` opens the filter early and not late: on a
    saw with cutoff 200 Hz, env 5 octaves and decay 100 ms, energy above 1 kHz is over 1 % in 0-20 ms and
    below 0.1 % in 200-400 ms.

- **HARN-09** The rounds: the owner chooses by ear, in at most 8 rounds, each one question. Every round
  file is in `tests/manual/rounds/`, and "no difference" and "none of these" are answers that count. A
  round's base is the winner so far, so round files after the first are written as the rounds go.
  Options in each round, always including the one without the addition:

  | # | Round | Options | Why |
  | --- | --- | --- | --- |
  | 1 | Starting point, all on A0 | today's default; "more tick" (Amount 60, Decay 40, Shape +30, from the VS1-01 run); the curve of each of the four references, moved to A0 | new defaults |
  | 2 | Sweep in stages | one stage; a second stage of 3 st over 650 ms (KICK09); 6 st over 300 ms | comparison item 2 |
  | 3 | Hard start | none; `start_phase=90`; a tone click; a noise click | item 1 |
  | 4 | First 20 ms | even; the two dents of Phat Valid; one dent | item 4 |
  | 5 | Saturation | none; soft after amp; soft before amp; asymmetric; hard | IDEA-02, item 8 |
  | 6 | Filter | off; `svf12`; `svf24`; `ladder`. If round 5 chose none, this round runs on soft saturation and says so, because a sine leaves a filter nothing to do | IDEA-02, BODY-05 |
  | 7 | Tuning between notes | on A0; -22 cents; +44 cents. The question is whether a difference is heard at all | item 3 |
  | 8 | Check | all choices together; the same without the last addition; the same without the saturation or filter chosen; today's default | shows that the choices hold together |

  - **HARN-09a** Before each round the owner gets the `afplay` commands, one per file, and one question.
    After the answer Claude reads `key.txt` and reports what was chosen with its numbers from
    `kikkuh-measure`, not a judgement of the sound.
  - **HARN-09b** The run file `tests/manual/runs/<date>-VS1-09.md` lists each round: its file, seed,
    letters, key, the owner's answer and the measured numbers of the choice, with `## Open points`.
  - **HARN-09c** A choice in round 8 is what counts: an addition that the owner drops there is not a choice.
  - **HARN-09d** Each choice becomes a numbered criterion in `specs/modules/body.md`, marked for VS1-03,
    with its numbers. That includes BODY-08, the new defaults, with a check that a kick at those values
    ends without a click with the sweep on (the open point of the VS1-01 run). A choice that belongs
    elsewhere, as a click does in LAYER-01, is written there or as a backlog row, and a choice that makes
    VS1-03 larger than M is split into its own row.
