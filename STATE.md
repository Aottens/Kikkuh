# Kikkuh — State

_Last updated: 2026-11-04 (VS1-05)_

This file is the handoff: where we are, what is next, what waits. It is not a log.

## Current focus
BODY now plays what the owner chose in VS1-09: a sweep in two stages, a start at full level and 6 dB of hard clipping
by default, and it measures as the sketch did. Older projects keep their sound (state version 2). BODY-05's filter type
is still a proposal. The panel is still the generic editor, so the tail readout (BODY-02d) is not visible until VS1-06.
At the defaults a kick peaks at +1.17 dBFS at velocity 127, and nothing limits it until the Master section (VS3-01).
The kick now plays: Velocity and Key Track are knobs, and a new note cuts off a ringing one without a click (3 ms fade,
no knob). With Key Track on, a C3 at the defaults peaks at +3.1 dBFS, also unlimited until VS3-01.
clap-validator warns that scanning takes 410 ms against its 100 ms guideline; not a failure, watch it.

## Next
1. VS1-04 (tail anchor) as its own build, with BODY-07 still to be specified; then VS1-06 (panel). Tests that read the bare voice use `BarePlugin` in
   `tests/Body.cpp`: at the defaults the signal passes the oversampling filters, which delay and ring.

## Waiting for a run in Bitwig (gate 3, owner)
- VS1-03: `tests/manual/vs1-03-drive-filter.md`. Written for Drive default 0: on this build its step 2 no longer holds
  (the defaults changed on purpose) and step 6 has more new knobs; to test it as written, run it with Drive set to 0 first.
- VS1-10: `tests/manual/vs1-10-new-defaults.md`. Its step 5 warns of clicks on fast notes: fixed by VS1-05, so on this build they should be gone. Its step 6 needs an older project, and the owner has none: leave it out, as in the VS1-05 run.

## Blocked
- none

## Owner decisions open
- CORE-01 asks for three reference kicks and VS1-08 chose four: run it with all four, or pick three. Needed by VS1-07.
- Is tuning between notes worth a Fine knob (IDEA-03)? Proposed: after v1.
- Other DAWs for testing (SELL-07, X-01): only Bitwig for now, needed by VS6-03 before release.
