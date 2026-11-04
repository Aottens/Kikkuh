# Kikkuh — Decision Log

Append-only. Newest at top. Each entry: date, decision, alternatives, why, reversal trigger.

---

## 2026-11-03 — BODY: finite curves, notes with C3 = 60, wavetables (owner approved the spec)

**Decision:** Decay is the whole length of a curve `(1 - x)^k`, which arrives at exactly 0. Note
names use MIDI 60 = C3. The oscillator reads band-limited wavetables built as sums of sines, with
one level constant per wave.

**Alternatives considered:** exponential curves with a time constant; C4 = 60; PolyBLEP.

**Reasoning:** a finite curve lands exactly on Tune, so the tail has a defined start for the anchor
of BODY-07. With C3 = 60, the PRD's "C1, F#0" are 65 and 46 Hz, real kick tails. Sums of sines start
every wave at zero with the fundamental in phase, so morphing cannot cancel it and a sweep keeps its
level. The cost: a square above about 6 kHz peaks up to 8 % over 1.0, reachable only with extreme
Tune and Amount, and MASTER comes after it.

**Reversal trigger:** the owner's Bitwig note names turn out not to be C3 = 60, or CORE-01 shows
that Decay as a whole length feels wrong.

---

## 2026-11-03 — A fixed test kick in the skeleton

**Decision:** the skeleton plays a fixed sine sweep on every MIDI note (BUILD-06), not silence.

**Alternatives considered:** a silent skeleton, as Dukko's passthrough was.

**Reasoning:** an instrument's riskiest path is MIDI to audio. With sound, the first tests and the
first Bitwig run prove sample accuracy, sample-rate independence and the host path.

**Reversal trigger:** none; BODY replaces it in slice 1.

## 2026-11-03 — JUCE 9.0.5, Starter tier

**Decision:** build on JUCE 9.0.5 under the Starter licence.

**Alternatives considered:** JUCE 8.0.12 as in Dukko.

**Reasoning:** a new product should start on the current major. The JUCE 9 EULA caps Starter at
$20,000 revenue or funding over the previous 12 months, counted from the use of the framework for
an individual and across the entity and all affiliates for a company. Above that: Indie ($40 a
month or $800 once, up to $300,000), then Pro. Section 1.9 allows selling products you own. The
EULA was read through a summary tool, not in full.

**Reversal trigger:** revenue nears the limit (move to Indie), or a full reading of the EULA before
release finds a term that blocks this. That reading is a task in slice 6.

## 2026-11-03 — Plugin identity (owner)

**Decision:** "Kikkuh by Colourmotion": product "Kikkuh", company "Colourmotion", manufacturer code
`Cmot`, plugin code `Kik1`, bundle and CLAP id `com.colourmotion.kikkuh`.

**Reasoning:** hosts store these in saved projects. The owner chose the name and brand; the codes
follow from them.

**Reversal trigger:** free to change until the first release, at the cost of the owner's own test
projects. The trademark check (slice 6) is the one known reason it could still change. After release: none.

## 2026-11-03 — Harness carried over from Dukko and CMDMX

**Decision:** the CMDMX loop in the form Dukko already adapted it to JUCE: one `CMakeLists.txt`,
`FetchContent`, Catch2, `BACKLOG`/`STATE`/`IDEAS`/`DECISIONS`, `check-tracking.py`, the
project-orchestrator agent and gate 3 checklists. From CMDMX directly: the *Builds* rules. New for
Kikkuh: render and measure helpers for tests, one `validate.sh` for local and CI, AU and Windows in
CI from the start, selling rules, and the CORE-01 test as a panel gate.

**Alternatives considered:** copying CMDMX's Rust and Swift tooling (does not apply); the Pamplejuce
template (Dukko removed it for carrying helpers nothing used).

**Reasoning:** Dukko already ran this loop on a real plugin in Bitwig.

**Reversal trigger:** a rule costs more than it catches, on the owner's say.

## 2026-11-04 — Reference kicks stay outside the repo (owner)

**Decision:** four references, chosen by the owner by ear from kicks already on this Mac. The repo holds
their names, hashes and numbers, not the audio.

**Alternatives considered:** three, as CORE-01 asks; committing the audio.

**Reasoning:** one pack has no licence file and the Bitwig terms are not read. The numbers are what later
work needs.

**Reversal trigger:** a test in CI needs the audio itself, with a licence that allows it.

## 2026-11-04 — BODY's sound chosen by ear in blind rounds (owner)

**Decision:** new defaults and engine as chosen in VS1-09: a fast sweep with a second stage, a start at full level,
hard clipping at 6 dB after the amp, no filter by default. Saturation and filter variants are compared at -20 LUFS over
400 ms from the first sound.

**Alternatives considered:** defaults fitted to a reference's numbers, which round 1 turned down; soft or asymmetric
saturation; a start at zero, which BODY-01c asked for; matching by peak, which would judge the loudest as best.

**Reasoning:** the owner's ears decide (IDEA-02). Preference was weak in some rounds, so the knobs stay and the choice
is checked in a loop in VS1-07.

**Reversal trigger:** the CORE-01 run in VS1-07 fails on these defaults.
