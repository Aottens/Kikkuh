# CLAUDE.md — how to work in this repo

Kikkuh by Colourmotion is a kick and rumble instrument plugin, built to be sold.
Read `PROJECT.md`, `STATE.md`, `ARCHITECTURE.md` and `DECISIONS.md` before doing anything.
Talk to the owner in **Dutch**; write code, comments, specs and commits in **English**.

## The loop: spec → test → code → review

1. **Spec first.** Every feature has a spec in `specs/modules/<module>.md` with numbered
   acceptance criteria (`BODY-03`, `RUM-02`, …), growing out of the ids in `specs/PRD.md`.
   No code for a criterion that has no number. If the spec is missing or unclear, write or extend
   it and stop for owner approval (gate 1).
2. **Test second.** Each criterion gets a Catch2 test carrying its id in the test name
   (`TEST_CASE ("rum_04_next_kick_ducks_the_rumble")`) in `tests/`. Write the failing test before
   the implementation.
3. **Code third, by a Sonnet subagent.** The main session writes spec and failing tests; a
   Sonnet subagent (`model: sonnet`) writes the smallest change that turns them green; the main
   session reviews the diff before it goes to the owner. Gate: `cmake --build build` with no
   warnings in our code, `ctest --test-dir build` green and `scripts/validate.sh` green.
4. **Review.** One branch and one PR per build (see *Builds*) on GitHub; the owner reviews and
   merges (gate 2). Anything only a DAW or ears can show ships with a checklist in `tests/manual/`
   that only the owner can tick (gate 3).
5. **Close the loop.** Update `STATE.md`, `BACKLOG.md` and `README.md` in the same PR. The README
   stays a README for users: a new feature goes under *What works today*; `## Status` is rewritten,
   never appended to, at most 8 lines (checked), with no task ids or changelog prose.
   A decision made along the way goes in `DECISIONS.md`.
   **Merged = done:** the PR sets its own `BACKLOG.md` row to `done (date, #PR)`, or
   `gate3: <checklist path>` when only the owner's run is left; never `review`/`merged`.
   Open points found in a run go under `## Open points` in the run file (`tests/manual/runs/`),
   each ending in `→` its backlog row, IDEA or checklist, made in that PR.
   `STATE.md` is the handoff (focus, next, gate 3, blocked, owner decisions), not a log.
   `scripts/check-tracking.py` checks this in CI. **Every PR description has a `## Tracking`
   section** with the output of `scripts/check-tracking.py --summary origin/main`.
6. **Clean up after the merge.** In the owner's checkout on `main`:
   `git fetch --prune && git branch -vv | awk '/: gone]/{print $1}' | xargs git branch -D`.

Use the `project-orchestrator` agent in `.claude/agents/` for pre-flight and post-flight checks.

## Builds: what goes together

A build is one session, one branch and one PR, holding one task or a few that belong together. The
main session proposes the builds in `BACKLOG.md`, in a `Builds:` line above the slice's table, and
the owner approves them with the spec.

- **Together** when the tasks are one behaviour across layers, or one needs the other.
- **Apart** when one may have to be reverted alone, or it holds a gate 3 run that would keep the
  rest from being done.
- **Size:** at most three tasks and at most one of them M. An L or XL is a build of its own.
- **Context:** each task gets its own Sonnet subagent with its own brief: criteria ids, failing
  tests, files to read and what the task before it left.
- **Quality:** one commit per task. The main session runs the gate and reviews each diff before
  the next subagent starts.
- **Stop:** when a task turns out larger than its size or review finds a real defect in the
  approach, ship what is done and move the rest to the next build.

## Sessions

- **One task or build per session.** After its PR is merged, update STATE/BACKLOG and offer the
  next as a new session with a self-contained brief.
- **A session that writes code gets its own worktree** (`use_worktree: true`). Never check out or
  pull in the owner's checkout while another session works there.
- **Session actions stay in this project.** On a bulk action that goes wrong, stop and ask.

## Owner test runs (gate 3)

- **Ask explicitly.** A run the owner must do is its own section in the message ("Test nodig"),
  with numbered steps, what to listen for and how to report back. It is also the question of the turn.
- **Few steps for the owner.** Tag steps **[Claude]** (building, installing, resets, anything a
  script or computer use can do) or **[Jij]** (the 5–8 steps that need ears).
- **Progress counter.** Every step message shows where the run stands: "Stap 3/5".
- The owner answers per step; Claude writes the run file in `tests/manual/runs/` with `## Open points`.
- **Host:** Bitwig Studio first. Other hosts come with SELL-07, before release.

## Claude cannot hear

- Every claim about behaviour is a number in a test: use `tests/Render.h` to play notes offline and
  `tests/Measure.h` to measure. Extend `Measure.h`, with a check on a known signal in
  `tests/MeasureCheck.cpp`, rather than writing a one-off measurement in a test.
- Say "the tests show X", never "it sounds good". Whether it sounds good is the owner's call.
- A test may not pass because of what the DSP happens to output today: derive expected values from
  the spec.

## The product rule (CORE-01)

The panel is the product. Knobs use subtractive-synth words, everything is on one screen, there are
no tabs, and a curve is never required. Any change that alters how the panel works comes with a
re-run of the CORE-01 test (`tests/manual/core-01-reference-kicks.md`). If it fails, the panel is
fixed before new features land.

## Ideas intake

The owner drops ideas in chat. Record each one in `IDEAS.md` (id, intent, proposed phase, module,
open questions) and ask the owner to confirm the phase. No criteria or code for an idea until its
phase is planned. v1 is `specs/PRD.md`; new ideas default to later.

## Audio-thread rules (non-negotiable in `processBlock` and everything it calls)

- No allocation, no locks, no syscalls, no logging, no `std::function` construction, no
  exceptions. Data from the UI arrives through atomics or a lock-free snapshot swap.
- Samples load on a background thread and reach the audio thread by snapshot swap; the old one is
  freed off the audio thread.
- `juce::ScopedNoDenormals` at the top of `processBlock`.
- Every audible parameter is smoothed; a retrigger or a switch never clicks.
- MIDI is handled at its sample offset, never at the block start.
- Zero reported latency (ENG-02): oversampling uses minimum-phase filters or nothing.
- Time-based DSP works in seconds, never in samples, so that it sounds the same at every sample
  rate (ENG-04). Tempo values are derived from the host's PPQ every block, never accumulated;
  guard against `bpm < 1` and a missing play head.
- DSP math lives in `source/dsp/` as plain C++ with no JUCE dependency; the processor is thin glue.
  `source/ui/` holds no DSP.

## Selling rules

- **Identity is permanent from the first release:** manufacturer code `Cmot`, plugin code `Kik1`,
  bundle and CLAP id `com.colourmotion.kikkuh`, company "Colourmotion", product "Kikkuh". Parameter
  ids are strings and are permanent too. The audio bus layout is frozen at release (PRD X-01).
- **No GPL or AGPL code linked in.** Tools that only run against the binary may be GPL. Every new
  dependency is a decision in `DECISIONS.md` with its licence, listed in `ARCHITECTURE.md`.
- Factory samples and presets are self-made or cleanly licensed, with the source written down.
- No telemetry and no required online account.

## Toolchain notes

- C++20, JUCE 9, clap-juce-extensions, Catch2 v3, all via `FetchContent` in `CMakeLists.txt`.
- Build: `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build`.
  `COPY_PLUGIN_AFTER_BUILD` installs into `~/Library/Audio/Plug-Ins/{VST3,Components,CLAP}/`.
- Tests: `ctest --test-dir build`. Validators: `scripts/validate.sh` (pluginval strictness 10,
  clap-validator, auval, architecture check), the same script locally and in CI.
- Sanitizer: add `-DCMAKE_BUILD_TYPE=Debug -DKIKKUH_ASAN=ON` in a separate build directory.

## Commit style

`<area>: <imperative summary>` — e.g. `dsp: add body pitch envelope (BODY-03)`.
Reference criterion ids in the body. One commit per task, one build per PR.
