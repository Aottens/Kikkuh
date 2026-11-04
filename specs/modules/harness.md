# Module spec: harness (HARN)

Status: **HARN-01..03 built with VS0-01. HARN-04 approved by the owner (2026-11-03), built by VS1-02. HARN-05 and HARN-06 approved by the owner (2026-11-03), built by VS1-08; the owner chose four references (2026-11-04).**

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
