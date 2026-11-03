# Module spec: harness (HARN)

Status: **built with VS0-01, except HARN-04 (VS1-02).**

## Acceptance criteria

- **HARN-01** `tests/Measure.h` measures peak, first sound, largest step and frequency; each is checked on a known signal.
- **HARN-02** `scripts/validate.sh` runs pluginval at strictness 10, clap-validator, auval and an architecture check on the built plugin; CI and local runs use the same script.
- **HARN-03** `scripts/check-tracking.py` runs in CI and fails on stale tracking, as in `CLAUDE.md`.
- **HARN-04** `kikkuh-render` writes a WAV from given parameter values, without a DAW.
