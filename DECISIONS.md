# Kikkuh — Decision Log

Append-only. Newest at top. Each entry: date, decision, alternatives, why, reversal trigger.

---

## 2026-11-04 — Drive: 8x minimum-phase oversampling, and a stricter alias test (VS1-03)

**Decision:** the clip runs at 8x between inverse Chebyshev low-passes (order 12, 60 dB, stop edge at the base Nyquist),
blended in over the first 0.5 dB, so Drive 0 is sample-identical to the input. BODY-06d is also tested on every note
from A#1 to D3, because 200 Hz alone cannot fail at 48 kHz: its aliases land on its harmonics. The filter is likewise
out of the path at the top of Cutoff.

**Alternatives considered:** first-order anti-derivative anti-aliasing (measured -89 dB on 196 Hz but -56 dB on 262 Hz);
4x (-76 dB worst case); a steeper or higher stop edge (flatter or shorter, but below 80 dB or duller above 15 kHz).

**Reasoning:** measured worst case -84 dB from A#1 to D3 at Drive 24, -0.14 dB at 18 kHz, against a bare clip's -74 dB on
G2. The cost is that what passes the clip arrives 3 samples late at 48 kHz, about 0.06 ms, while the reported latency
stays 0 (ENG-02). The 80 dB limit is still a proposal; higher tones alias more (-68 dB at C5), as BODY-06d expects.

**Reversal trigger:** the owner hears aliasing or a late attack in the VS1-03 or CORE-01 run, or CPU becomes a problem.

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

## 2026-11-04 — Old projects migrate to the sound they had; a VS0-01 project takes the defaults (VS1-10)

**Decision:** state version 2. A version 1 state that holds knobs gets Sweep 2 Amount 0, Start 0 and Drive 0 where it
lacks them, and keeps what it has. A VS0-01 state holds no knobs and takes the new defaults. `dsp::BodyParams` keeps
its member defaults at the sound before BODY-08; the plugin's defaults live in `createLayout`.

**Alternatives considered:** migrating a VS0-01 state as well, which gives a mix nobody chose while its fixed kick is
gone anyway; moving `BodyParams` to the new defaults, which changes the "today" option of the VS1-09 rounds.

**Reasoning:** BODY-08d, and the VS1-09 rounds stay repeatable from their seed.

**Reversal trigger:** the sketch and its rounds are deleted, or the owner wants a VS0-01 project treated otherwise.

## 2026-11-04 — The peak may move with the sample rate below Start 90 (owner)

**Decision:** with Drive on below Start 90, the peak may move by up to 0.75 dB between 44.1 and 192 kHz (BODY-10c). The
default Start 90 stays held to 1 %, the other Starts to 10 %, and pitch and loudness hold at every Start.

**Alternatives considered:** making the top of the Drive filters independent of the sample rate.

**Reasoning:** the default is not affected, and pitch and loudness hold.

**Reversal trigger:** the owner hears a difference between sample rates, or a limiter in the Master section depends on the peak.

## 2026-11-04 — Playing: the note replaces Tune, velocity before Drive, a retrigger fades the old value (VS1-05, owner)

**Decision:** with Key Track on the played note replaces Tune, clamped to C0..C3. Velocity scales the level before Drive.
Kikkuh plays one kick at a time. On a retrigger the old kick's last value fades with a 3 ms time constant before Drive, and
the filter's envelope moves to its new start over the same fade. There is no knob for it.

**Alternatives considered:** Tune as a transposition around C1, which keeps the knob alive but plays out of key at its
default; velocity after Drive, which keeps the tone the same at every velocity; a second voice that rings on and fades,
which costs an oscillator per overlapping note and still needs a rule for notes that pile up.

**Reasoning:** what is in the piano roll is what you hear. Velocity before Drive is today's behaviour, so projects keep
their sound. One fading value needs no extra voice and holds at any note rate.

**Reversal trigger:** the VS1-05 run hears the fade as abrupt or smeared, or a dead Tune gets in the way in VS1-07.
