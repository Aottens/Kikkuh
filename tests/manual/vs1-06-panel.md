# Checklist: the panel in Bitwig (VS1-06; FAST-01, FAST-05)

Result goes in `tests/manual/runs/<date>-VS1-06.md`, with `## Open points`.
The tests show which controls exist, where, and what they do. This run is about how it feels, and about one thing only a
host can show: whether the pad and the loop sound while the transport stands still.
Whether you can reach a kick within 3 minutes is CORE-01, the next run (VS1-07), so it is not repeated here.

1. **[Claude]** Build and install: `cmake --build build`.
2. **[Jij]** Rescan plugins and put a new Kikkuh (CLAP) on an instrument track. Is this the panel of the prototype, and can you
   read every label and value at normal monitor distance?
3. **[Jij]** Transport stopped: hit the pad, then switch Loop on. Does it sound, right away, in the project's tempo? The pad and
   the lamp light yellow on each hit: does that keep up with what you hear?
4. **[Jij]** Loop on, transport running with a four-to-the-floor on another track. Does the loop sit exactly on it? Then set the
   arranger to loop one bar, and stop and start the transport. Does it stay on the grid without a double hit?
5. **[Jij]** Drag knobs with the loop on, with and without Shift. Double-click and Cmd-click a knob. Click a value and type: `F#0` on
   Tune, `1k` on Cutoff. Does each do what you expect, and do the drag distance and the Shift ratio feel right?
6. **[Jij]** Drag the window's corner from small to large. Is the smallest size still readable? Close and reopen the editor: the size
   is kept and Loop is off.
7. **[Jij]** Loop on, then bounce the track offline. The bounce holds no kick from the loop, only MIDI notes. Is that what you want?
8. **[Jij]** Is the light aluminium panel pleasant to work on for a while, also in your studio's light? Anything you would change now?
