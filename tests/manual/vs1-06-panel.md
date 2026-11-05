# Checklist: the panel in Bitwig (VS1-06; FAST-01, FAST-05)

Result goes in `tests/manual/runs/<date>-VS1-06.md`, with `## Open points`.
The tests show which controls exist, where, and what they do. This run is about how it feels.
Whether you can reach a kick within 3 minutes is CORE-01, the next run (VS1-07), so it is not repeated here.

Done: the run, its re-test and the last check are in `tests/manual/runs/2026-11-05-VS1-06.md`. A new run of this checklist comes with any change to how the panel works:

1. **[Claude]** Build and install: `cmake --build build`.
2. **[Jij]** Transport stopped: hit the pad, then switch Loop on. Does it sound at once, in the project's tempo, with the yellow keeping up?
3. **[Jij]** Transport running against a four-to-the-floor: does the loop sit on it, through an arranger loop and a stop and start?
4. **[Jij]** Drag knobs and Tune, with and without Shift; the wheel with and without Shift; double-click and Cmd-click; type `F#0` on Tune and `1k` on Cutoff.
5. **[Jij]** Resize from small to large. Close and reopen: the size is kept, Loop is off.
6. **[Jij]** Loop on, then bounce offline: only the MIDI notes in the bounce.
7. **[Jij]** Is everything aligned and readable, and is the panel pleasant to work on?
