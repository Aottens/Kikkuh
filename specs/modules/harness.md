# Module spec: harness (HARN)

Status: **HARN-01..03 built with VS0-01. HARN-04 approved by the owner (2026-11-03), built by VS1-02.**

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
