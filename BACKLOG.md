# Kikkuh — Backlog

One sentence per task with a verifiable done criterion; T-shirt size (S ≤ 2 h, M ≤ half day,
L ≤ day, XL = split it); the owner reviews every PR.
Status (first word of the Status cell, checked by `scripts/check-tracking.py` in CI):
`todo` · `doing` · `done` · `gate3` · `blocked`, details in brackets or after a colon.
- A task PR sets its own row to `done (date, #PR)` in the PR itself: **merged = done**.
- `gate3: <checklist path>` = merged, only the owner's run in Bitwig is left; the path must exist.
- Open points found in a run become their own row (or an IDEA) in the same PR.

Slices below 0 are a proposal: their `Builds:` lines and full criteria come when each slice starts.

## Slice 0 — harness and skeleton

| Id | Task (one sentence, verifiable) | Spec ids | Size | Status |
| --- | --- | --- | --- | --- |
| VS0-01 | The skeleton builds VST3, AU and CLAP and passes `ctest` and `validate.sh` locally and in CI, and the owner hears the test kick from a MIDI note in Bitwig. | BUILD-01..06, ENG-01..04, 06, HARN-01..03 | M | done (2026-11-03, #1; run `tests/manual/runs/2026-11-03-VS0-01.md`) |

## Slice 1 — a kick you can shape with knobs

Builds: VS1-01 alone (L). Order after it: VS1-02, VS1-08, VS1-09, VS1-03, VS1-10 (owner, 2026-11-03; VS1-10 from VS1-09). Then, one build each: VS1-05 (owner, 2026-11-04), VS1-04 (owner, 2026-11-04), and VS1-06 (L; owner, 2026-11-05). The rest are proposed when they start.

| Id | Task (one sentence, verifiable) | Spec ids | Size | Status |
| --- | --- | --- | --- | --- |
| VS1-01 | BODY's wave, tune, pitch and amp knobs are host parameters on a smoothed voice that replaces the test kick, and tests measure each one's effect. | BODY-01..04, ENG-01, 06 | L | done (2026-11-03, #2) |
| VS1-02 | A `kikkuh-render` command writes a WAV for given parameter values, so a sound can be measured and shared without a DAW. | HARN-04 | S | done (2026-11-03, #3) |
| VS1-08 | Reference kicks chosen by the owner (four) are measured (sweep, harmonics, first milliseconds), and a written comparison says what BODY lacks. | IDEA-02, CORE-01, HARN-05, 06 | M | done (2026-11-04, #4) |
| VS1-09 | The owner chooses saturation, filter and curve variants and new defaults in blind, level-matched listening rounds, and the choices are written down as criteria for VS1-03. | IDEA-02, HARN-07..09 | L | done (2026-11-04, #5; run `tests/manual/runs/2026-11-04-VS1-09.md`) |
| VS1-03 | BODY's filter and drive work as VS1-09 chose, and a test shows drive at full stays below the alias limit of BODY-06d, or the owner agrees another. | BODY-05, 06 | M | gate3: `tests/manual/vs1-03-drive-filter.md` |
| VS1-10 | The second sweep stage and Start work and the defaults are the ones chosen in VS1-09, and a test measures them against its numbers. | BODY-08, 09, 10 | M | gate3: `tests/manual/vs1-10-new-defaults.md` |
| VS1-04 | With the tail anchor on, changing pitch knobs leaves the tail sample-identical in a test; off restores classic behaviour. | BODY-07 | M | gate3: `tests/manual/vs1-04-tail-anchor.md` |
| VS1-05 | Velocity, key tracking and retrigger work, and a test shows no step above an agreed size when a kick retriggers at 200 BPM. | PLAY-01..03 | M | done (2026-11-04, #8; run `tests/manual/runs/2026-11-04-VS1-05.md`) |
| VS1-06 | One panel of real knobs in signal-flow order replaces the generic editor, with the audition pad and loop. | FAST-01, 05 | L | done (2026-11-05, #10; run `tests/manual/runs/2026-11-05-VS1-06.md`) |
| VS1-07 | Owner run in Bitwig: the CORE-01 test passes, or its findings become rows before slice 2 starts. | CORE-01 | S | done (2026-11-05, #11; run `tests/manual/runs/2026-11-05-CORE-01.md`) |

## Slice 2 — Click and Top

| Id | Task (one sentence, verifiable) | Spec ids | Size | Status |
| --- | --- | --- | --- | --- |
| VS2-01 | Click and Top run in parallel over the body with built-in sources and their knobs, from one shared section class. | LAYER-01..03 | M | todo |
| VS2-02 | Dragging in your own sample works, loads off the audio thread and comes back with the project; a missing file gives silence and a message. | LAYER-01, 04 | L | todo |
| VS2-03 | Owner run in Bitwig: CORE-01 on all four references with Click and Top, and Beefy SM ELE, missed in VS1-07, is usable within 3 minutes or its findings become rows. | CORE-01 | S | todo |

## Slice 3 — Master

| Id | Task (one sentence, verifiable) | Spec ids | Size | Status |
| --- | --- | --- | --- | --- |
| VS3-01 | The oversampled clipper with autogain works at zero latency, and a test measures its aliasing. | MAST-01, ENG-02 | L | todo |
| VS3-02 | Shelves, output and meters work, and a test holds the whole kick under the CPU limit. | MAST-02, 03, ENG-05 | M | todo |

## Slice 4 — Rumble

| Id | Task (one sentence, verifiable) | Spec ids | Size | Status |
| --- | --- | --- | --- | --- |
| VS4-01 | A spike plugin answers in Bitwig whether a send to a track and a return by sidechain can run as a loop, and with how much delay; the result is written to `DECISIONS.md`. | X-01 | S | todo |
| VS4-02 | The reverb and noise sources with tone, drive and size feed the sum. | RUM-01, 03 | L | todo |
| VS4-03 | Groove follows the host tempo in note values and survives loops and tempo changes in tests. | RUM-02 | M | todo |
| VS4-04 | The next kick resets the rumble as set by a knob, and the rumble is mono below an adjustable frequency, both measured in tests. | RUM-04, 05 | M | todo |
| VS4-05 | Kick and rumble come out on their own outputs, with the bus layout decided against the spike result. | MAST-04, X-01 | M | todo |

## Slice 5 — fast to the sound, and curves

| Id | Task (one sentence, verifiable) | Spec ids | Size | Status |
| --- | --- | --- | --- | --- |
| VS5-01 | A live log-time view of pitch and amp sits on the panel. | CURV-01 | M | todo |
| VS5-02 | Dragging the curve makes your own shape, and knobs still scale it afterwards, shown by a test. | CURV-02, 03 | L | todo |
| VS5-03 | A dropped WAV can be compared level-matched with Kikkuh, with its waveform overlaid. | FAST-03 | M | todo |
| VS5-04 | A/B compare and the 8–12 starting points work, and the owner re-runs CORE-01 with them. | FAST-02, 04, CORE-01 | M | todo |

## Slice 6 — sellable

| Id | Task (one sentence, verifiable) | Spec ids | Size | Status |
| --- | --- | --- | --- | --- |
| VS6-01 | Owner checks "Kikkuh" and "Colourmotion" in BOIP, EUIPO and USPTO, and the full JUCE EULA is read; the outcome is in `DECISIONS.md`. | SELL-01 | S | todo |
| VS6-02 | Universal macOS and Windows builds pass the validators, and the Windows build runs on a real Windows machine. | SELL-01, 02 | L | todo |
| VS6-03 | Owner runs on Ableton Live, Logic Pro and FL Studio pass, including the send and return result from VS4-01. | SELL-07, X-01 | M | todo |
| VS6-04 | Signed, notarised macOS installer and signed Windows installer come out of CI. | SELL-03 | L | todo |
| VS6-05 | Offline licence keys and the demo mode work without any network access. | SELL-04, 05 | L | todo |
| VS6-06 | Short manual and factory content are done, with every source written down. | SELL-06 | M | todo |
