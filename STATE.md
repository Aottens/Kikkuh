# Kikkuh — State

_Last updated: 2026-11-06 (VS2-02)_

This file is the handoff: where we are, what is next, what waits. It is not a log.

## Current focus
VS2-02 is built: drag your own WAV, AIFF or FLAC onto CLICK or TOP (LAYER-04, approved with every recommendation). It is
read off the audio thread, played through the noise's sinc kernel with Start and Tune, and held inside the saved state, so
the file is not needed after the drop; the state version stays 3. It waits for the owner's run (`tests/manual/vs2-02-own-sample.md`).
Slice 2 has started: Click and Top (`specs/modules/layer.md`, approved with every recommendation) play a tone or noise in
parallel over the body, through a 12 dB high- and low-pass and a Decay envelope, on a second band of the panel. Both start
Off, so the kick and saved projects are unchanged and the state version stays 3. The tests found a setting that makes
Beefy's bright first 5 ms (LAYER-02g). The owner's run passed: the defaults and an older project are unchanged, the knobs
are clean and a click can be made with Tone or Noise; TOP on a built-in source does not feel usable yet, and the Beefy
setting does not get there and clips (`tests/manual/runs/2026-11-06-VS2-01.md`). A reloaded state is now exact for
every knob (ENG-06a), which clap-validator had started to fail on.
BODY now plays what the owner chose in VS1-09: a sweep in two stages, a start at full level and 6 dB of hard clipping
by default, and it measures as the sketch did. Older projects keep their sound (state version 2). BODY-05's filter type
is still a proposal.
The panel is there: one printed plate in signal order, with the tail readout, a hit pad and a loop. The owner approved it in Bitwig "for now"; what is left to polish is IDEA-05.
CORE-01 on this panel: three of four references usable within 3 minutes. Beefy SM ELE missed on its almost acoustic character, not on the panel, so slice 2 starts and VS2-03 runs CORE-01 again after Click and Top.
At the defaults a kick peaks at +1.17 dBFS at velocity 127, and nothing limits it until the Master section (VS3-01).
The kick now plays: Velocity and Key Track are knobs, and a new note cuts off a ringing one without a click (3 ms fade,
no knob). With Key Track on, a C3 at the defaults peaks at +3.1 dBFS, also unlimited until VS3-01.
The tail anchor is on by default: pitch knobs leave the tail sample-identical, Start holds and the sweep bends by up to half a cycle, so the default kick is a little different from the one chosen in VS1-09 (-62 cents at the note-on, gone at 50 ms). Older projects keep it off (state version 3).
clap-validator warns that scanning takes 410 ms against its 100 ms guideline; not a failure, watch it.

## Next
1. The owner's VS2-02 run in Bitwig (gate 3, below).
2. VS2-03 after it: the owner's CORE-01 re-run on all four references; the Bitwig project `CORE-01` is kept for it.
   Tests that read the bare voice use `BarePlugin` in `tests/Body.cpp`; tests of a layer alone subtract a render with both
   sections Off (`layer` in `tests/Layer.cpp`).

## Waiting for a run in Bitwig (gate 3, owner)
- VS2-02: `tests/manual/vs2-02-own-sample.md`. Its step 7 is the question VS2-01 left: does your own sample give TOP a use?
- VS1-04: `tests/manual/vs1-04-tail-anchor.md`. Its step 2 is the main question: does the bend cost the chosen kick anything?
- VS1-03: `tests/manual/vs1-03-drive-filter.md`. Written for Drive default 0: on this build its step 2 no longer holds
  (the defaults changed on purpose) and step 6 has more new knobs; to test it as written, run it with Drive set to 0 first.
- VS1-10: `tests/manual/vs1-10-new-defaults.md`. Its step 5 warns of clicks on fast notes: fixed by VS1-05, so on this build they should be gone. Its step 6 needs an older project, and the owner has none: leave it out, as in the VS1-05 run.

## Blocked
- none

## Owner decisions open
- Is tuning between notes worth a Fine knob (IDEA-03)? Proposed: after v1.
- Other DAWs for testing (SELL-07, X-01): only Bitwig for now, needed by VS6-03 before release.
