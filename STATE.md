# Kikkuh — State

_Last updated: 2026-11-04 (VS1-10)_

This file is the handoff: where we are, what is next, what waits. It is not a log.

## Current focus
BODY now plays what the owner chose in VS1-09: a sweep in two stages, a start at full level and 6 dB of hard clipping
by default, and it measures as the sketch did. Older projects keep their sound (state version 2). BODY-05's filter type
is still a proposal. The panel is still the generic editor, so the tail readout (BODY-02d) is not visible until VS1-06.
At the defaults a kick peaks at +1.17 dBFS at velocity 127, and nothing limits it until the Master section (VS3-01).
clap-validator warns that scanning takes 410 ms against its 100 ms guideline; not a failure, watch it.

## Next
1. Propose the next build from VS1-04, VS1-05 and VS1-06. Tests that read the bare voice use `BarePlugin` in
   `tests/Body.cpp`: at the defaults the signal passes the oversampling filters, which delay and ring.

## Waiting for a run in Bitwig (gate 3, owner)
- VS1-03: `tests/manual/vs1-03-drive-filter.md`. Written for Drive default 0: on this build its step 2 no longer holds
  (the defaults changed on purpose) and step 6 has more new knobs; to test it as written, run it with Drive set to 0 first.
- VS1-10: `tests/manual/vs1-10-new-defaults.md`

## Blocked
- none

## Owner decisions open
- With Drive on below Start 90 the peak moves by up to 0.75 dB between 44.1 and 192 kHz (BODY-10c), while pitch and loudness hold. Accept it, or make the Drive filters' top independent of the rate? Proposed: accept, since the default is not affected.
- CORE-01 asks for three reference kicks and VS1-08 chose four: run it with all four, or pick three. Needed by VS1-07.
- Is tuning between notes worth a Fine knob (IDEA-03)? Proposed: after v1.
- Other DAWs for testing (SELL-07, X-01): only Bitwig for now, needed by VS6-03 before release.
