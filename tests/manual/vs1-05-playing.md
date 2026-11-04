# Checklist: playing the kick in Bitwig (VS1-05; PLAY-01, PLAY-02, PLAY-03)

Result goes in `tests/manual/runs/<date>-VS1-05.md`, with `## Open points`.
The tests measure the steps and levels; this run is about what only ears can tell. Above all: whether 3 ms is the right
fade, which has no knob.

1. **[Claude]** Build and install: `cmake --build build`.
2. **[Jij]** Rescan plugins and put a new Kikkuh (CLAP) on an instrument track, all knobs at their defaults. Play a run of
   sixteenths at 200 BPM. Do you hear a click, a tick or a gap where a note cuts off the one before it?
3. **[Jij]** Set Amp Decay to 2000 ms and play the same run, then Start to 0. This is the hardest case for the fade: the
   old kick is still loud. Does the old kick stop too abruptly, or does it smear into the new hit?
4. **[Jij]** Back to the defaults. Draw notes with velocities from soft to hard, and move Velocity from 100 % to 0 %. Do
   soft notes feel right? They also clip less, because velocity comes before Drive. Is that what you want?
5. **[Jij]** Turn Key Track on and play a bass line from C1 to C3, monitors low: a keyed C3 peaks at +3.1 dBFS at the
   defaults, and nothing limits it until the Master section. Is it in tune against another instrument, and does it stay one
   kick from low to high? Tune does nothing now: does that get in the way?
6. **[Jij]** Open a project saved before this build. It sounds as it did.
