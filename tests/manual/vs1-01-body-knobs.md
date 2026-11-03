# Checklist: BODY's knobs by ear in Bitwig (VS1-01; BODY-01..04, ENG-01, ENG-06)

Result goes in `tests/manual/runs/<date>-VS1-01.md`, with `## Open points`.

1. **[Claude]** Build and install: `cmake --build build`.
2. **[Jij]** Rescan plugins. Put Kikkuh (CLAP) on an instrument track with a four-on-the-floor loop and open its generic editor. You hear a kick falling to A0 (55 Hz). Note what you would want to change first.
3. **[Jij]** Check the note names: does MIDI 60 read C3 in your piano roll, so that Tune's names match Bitwig's?
4. **[Jij]** Wave from left to right while it loops: sine, triangle, saw, square. It changes smoothly, without clicks, and the low end does not drop away.
5. **[Jij]** Tune up and down: the whole kick moves by notes. Then Pitch Amount, Pitch Decay and Pitch Shape: do they do what their names say, and is the useful part of each range easy to reach?
6. **[Jij]** Attack, Hold, Amp Decay and Amp Shape: the same two questions. Listen for a click at the end of a kick, also with a short Amp Decay.
7. **[Jij]** Automate Tune or Pitch Amount in a clip: no clicks. A click on very fast repeats is known at this stage (PLAY-03).
8. **[Jij]** Save, close and reopen the project: the knobs come back. A kick with a long Amp Decay on the last note of a bounce rings out in full.
