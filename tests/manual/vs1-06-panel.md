# Checklist: the panel in Bitwig (VS1-06; FAST-01, FAST-05)

Result goes in `tests/manual/runs/<date>-VS1-06.md`, with `## Open points`.
The tests show which controls exist, where, and what they do. This run is about how it feels.
Whether you can reach a kick within 3 minutes is CORE-01, the next run (VS1-07), so it is not repeated here.

The first run is `tests/manual/runs/2026-11-05-VS1-06.md`: the pad and the loop with the transport stopped and on the grid, resizing and
the material all held. What is left is what changed after it, and the bounce:

1. **[Claude]** Build and install: `cmake --build build`.
2. **[Jij]** Reopen Kikkuh. Is everything aligned now: knobs in a row on one line, labels and values on one line, bottom knobs under the
   top ones? Can you read every label and value at normal monitor distance?
3. **[Jij]** Drag Tune. Is 12 points per semitone right, or still too fast or now too slow?
4. **[Jij]** Scroll over a knob with and without Shift. Is Shift finer now?
5. **[Jij]** Loop on, then bounce a few bars of the track offline. The bounce holds only the MIDI notes, no kick from the loop. If Bitwig's bounce
   turns out to run in real time, report which option you used.
