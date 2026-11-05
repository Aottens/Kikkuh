# Kikkuh — State

_Last updated: 2026-11-05 (VS1-07)_

This file is the handoff: where we are, what is next, what waits. It is not a log.

## Current focus
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
1. Slice 2: write the criteria for Click and Top (LAYER-01..04) and the slice's `Builds:` line, for the owner's approval (gate 1), then VS2-01.
   VS2-03 is the CORE-01 re-run that closes the slice; the Bitwig project `CORE-01` (four tracks and the references) is kept for it.
   Tests that read the bare voice use `BarePlugin` in `tests/Body.cpp`, now with the anchor off: at the defaults the signal
   passes the oversampling filters, which delay and ring.

## Waiting for a run in Bitwig (gate 3, owner)
- VS1-04: `tests/manual/vs1-04-tail-anchor.md`. Its step 2 is the main question: does the bend cost the chosen kick anything?
- VS1-03: `tests/manual/vs1-03-drive-filter.md`. Written for Drive default 0: on this build its step 2 no longer holds
  (the defaults changed on purpose) and step 6 has more new knobs; to test it as written, run it with Drive set to 0 first.
- VS1-10: `tests/manual/vs1-10-new-defaults.md`. Its step 5 warns of clicks on fast notes: fixed by VS1-05, so on this build they should be gone. Its step 6 needs an older project, and the owner has none: leave it out, as in the VS1-05 run.

## Blocked
- none

## Owner decisions open
- Is tuning between notes worth a Fine knob (IDEA-03)? Proposed: after v1.
- Other DAWs for testing (SELL-07, X-01): only Bitwig for now, needed by VS6-03 before release.
