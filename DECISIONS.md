# Kikkuh — Decision Log

Append-only. Newest at top. Each entry: date, decision, alternatives, why, reversal trigger.

---

## 2026-11-06 — Your own sample is held in the project, not found by its path (VS2-02, owner)

**Decision:** a sample dropped on CLICK or TOP is read on a background thread (WAV, AIFF or FLAC), summed to mono, kept at its
own rate and cut at 3 s, and its audio goes into the saved state as 32-bit float with its name. The audio thread takes a new
sample at the next note-on, and the replaced one is freed off it. It plays through the sinc kernel the noise uses, without
band-limiting when Tune speeds it up. A drop on a section at Level Off sets -6 dB. The state version stays 3.

**Alternatives considered:** keeping a path, which breaks when a project moves to another machine; more formats (MP3, Ogg),
which differ between macOS and Windows; band-limited reading by rate; a longer limit.

**Reasoning:** PRD LAYER-04 says the samples are saved with the project and a missing file never breaks it; holding the audio
makes the file unneeded after the drop. 3 s covers Start plus Decay at their maximum, at most about 3 MB per section.

**Reversal trigger:** states grow too large for a host, or the owner hears aliasing at high Tune.

## 2026-11-05 — A reloaded state is exact, fixed in our code rather than worked around (VS2-01)

**Decision:** three causes made a reloaded parameter come back an ulp off, which clap-validator failed on once slice 2
added knobs and moved its random values: JUCE writes state values rounded ("20.0" for 20.0000019), a skewed range's float
round trip is not exact, and `replaceState` skips a value only approximately equal to the current one. The state now writes
nine significant digits, `ranged` picks the knob position whose value is exactly the one asked for, and
`setStateInformation` sets any value `replaceState` skipped.

**Alternatives considered:** snapping every knob to the precision it shows, which made the wheel stick at the bottom of a
skewed range (20 Hz on a filter); a fixed seed or a skipped test in clap-validator, which hides the fault from hosts.

**Reasoning:** a host that saves and reloads must get the same project back; the fault was in BODY's knobs too, hidden by
which values the validator happened to draw.

**Reversal trigger:** a JUCE update that makes the state round trip exact by itself.

## 2026-11-05 — Click and Top: generated sources, silent by default, after BODY's drive (slice 2, owner)

**Decision:** CLICK and TOP are one section class used twice, with Tone and Noise as generated sources (no factory
samples) and your own sample in VS2-02. Both start at Level Off, so saved projects keep their sound and the state version
stays 3. They are added after BODY's drive and filter, mono, with 12 dB high- and low-pass filters before a Decay
envelope. Start comes with the Sample source in VS2-02, as a time into the file.

**Alternatives considered:** the VS1-09 tone click on by default, which lost to no click in round 8; a third source (a
pulse) or self-made samples with an acoustic character; Start now, as a phase in degrees; 24 dB filters as BODY's; a stereo
layer for Beefy's side.

**Reasoning:** the owner's ears chose the kick without a click, and noise through two filters covers the bright first 5 ms
VS1-08 measured on Beefy (LAYER-02g). Filters before the envelope make a section exactly silent after Decay.

**Reversal trigger:** VS2-03 misses Beefy and names a source, a knob or stereo it looked for.

## 2026-11-05 — CORE-01 runs on four references; Beefy's miss goes to slice 2, not to the panel (VS1-07, owner)

**Decision:** CORE-01 uses the four references of VS1-08 every run. The first run on the VS1-06 panel made three of
them usable within 3 minutes; Beefy SM ELE missed on its almost acoustic character, not on a missing or surprising knob,
and nothing on the panel got in the way. So the panel is not changed and slice 2 starts. Click and Top are the layers
that can carry Beefy, and VS2-03 runs CORE-01 again on all four after them.

**Alternatives considered:** three references, which by dropping Beefy would have passed the test by choosing what passes;
fixing the panel before slice 2, as the CORE-01 rule asks after a fail, with no panel finding to fix.

**Reasoning:** the rule protects the panel, and this miss is a sound source BODY does not have. VS1-08 measured Beefy as
the reference with the most harmonics, the weakest sweep, an unsteady tail and a stereo side.

**Reversal trigger:** VS2-03 still misses Beefy, or names a knob it looked for: then the panel or BODY is fixed first.

## 2026-11-05 — The panel: printed aluminium, one colour for sounding, PITCH before OSC (VS1-06, owner)

**Decision:** the panel the owner approved in a clickable prototype after research (`docs/research/panel-design.md`). A flat
aluminium plate with ink print, and a printed line in the real order of the signal, so PITCH comes before OSC. Yellow means only that a
kick sounds. Two knob sizes, 1144 x 318 points, 80 to 200 %. The grafiet variant of the prototype is not built. After the owner's runs everything stands on one grid with two knob sizes, centred titles and light section lines: the first prototype's knobs were not aligned, which the owner rejected.

**Alternatives considered:** one long row of equal knobs at 1240 x 300; OSC first, as most synths have it; a dark panel with an accent colour.

**Reasoning:** unreadable text and poor scaling are the most reported complaints, and an oversized window hides the arrangement. A
panel gets a face from one rule applied everywhere. The evidence for the looks is thin and mostly taste; no source was found against
neon or LED rings on plugins as such, so that part is the owner's call.

**Reversal trigger:** the VS1-06 or CORE-01 run finds it hard to read or tiring, or PITCH before OSC gets in the way.

## 2026-11-05 — Archivo as the panel's typeface (owner)

**Decision:** four static cuts of Archivo (Omnibus-Type, SIL OFL 1.1, no reserved font name), compiled into the plugin, with
`assets/fonts/OFL.txt` alongside. Made with fontTools (instanced, subset to Latin, renamed "Kikkuh Archivo") from
`ofl/archivo/Archivo[wdth,wght].ttf` in github.com/google/fonts, sha256 `0e094a7d...b05053`.

**Alternatives considered:** the system font, which differs per platform; the variable font, because JUCE has no way to set its axes.

**Reasoning:** the OFL allows embedding in software that is sold; it requires the licence to go with it and forbids selling the font alone.
The installer (VS6-04) must ship `OFL.txt`.

**Reversal trigger:** the owner picks another typeface.

## 2026-11-05 — Audition: not parameters, not in a bounce, free-running when stopped (VS1-06, owner)

**Decision:** the pad and the loop are not host parameters, are not saved and are silent offline. The loop stops when the editor closes. It
plays quarter notes, at full level on Tune's note. On a running transport it follows the PPQ, and a first hit waits for the next quarter note.
On a stopped one there is no PPQ, so it counts the interval at the host's tempo: the one place tempo is counted rather than derived.

**Alternatives considered:** a host parameter, which a project could store as on, so that it plays with nobody asking; a choice of note values; the last
MIDI note on the pad, so what you hear depends on what you played before.

**Reasoning:** a bounce can never hold a kick that is not in the piano roll, and a loop never runs out of sight.

**Reversal trigger:** the VS1-06 run wants the loop in other note values, or Bitwig does not process audio with the transport stopped.

---

## 2026-11-04 — Tail anchor: Start holds, the sweep bends, each stage locks on its own (VS1-04, owner)

**Decision:** with the anchor on, the phase after the sweeps is `Start + Tune Hz * t`, taken from a sample counter and not
added up. Start also holds, so the sweep's frequency is multiplied by `1 + e * (1 - t/W)`, with `e` set at the note-on so
that at most half a cycle is gained or lost. Stage 1 can lock while the second stage runs. On by default; a project saved
earlier keeps it off (state version 3). The note-on's calculation is 5-point Gauss-Legendre at about 10 us, because Simpson
missed the landing by 1e-5 cycle and failed BODY-07c.

**Alternatives considered:** the phase backward from the anchor, so that Start floats, which loses the full hit of round 8;
locking only when all sweeps are done, at 350 ms, where the amp already stands at -54 dB; sliding the phase after the sweep,
which puts the correction in the tail and moves its pitch by up to 75 cents over 200 ms.

**Reasoning:** where the pitch is highest, half a cycle costs the fewest cents; whether it can be heard is for the VS1-04 run. The
cost: the default kick bends by -62 cents at the note-on and is no longer sample-identical to the one chosen in VS1-09, and a
short sweep on a low note bends by up to 534 cents at the note-on.

**Reversal trigger:** the VS1-04 run hears the bend as a change for the worse, or CORE-01 fails on it.

## 2026-11-04 — PLAY-03b holds for every gap, with a limit from what Drive does (VS1-04, owner)

**Decision:** a second note 50 to 310 ms after the first stays below a step of 1.15 at the defaults, and 0.75 at Start 0 with
Amp Decay 2000 ms.

**Alternatives considered:** keeping "within 0.05 of a hit from silence" at 75 and 300 ms, which the anchor breaks at 75 ms
(0.86 against 0.78); testing it with the anchor off, which hides the matter.

**Reasoning:** the old limit held by chance. Anchor on or off, 13 of 104 gaps exceed it, up to 1.13, because Drive clips the old
kick's negative swing just before the new hit. The new limits still fail without the fade (1.17 and 1.05).

**Reversal trigger:** the owner hears a click in step 6 of the VS1-04 run.

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
