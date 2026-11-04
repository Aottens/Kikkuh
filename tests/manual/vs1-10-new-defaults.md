# Checklist: the new defaults by ear in Bitwig (VS1-10; BODY-08, BODY-09, BODY-10)

Result goes in `tests/manual/runs/<date>-VS1-10.md`, with `## Open points`.
The tests measure the numbers against the sketch; this run is about what only ears can tell. VS1-07 hears these
defaults in a track as well (BODY-08e).

1. **[Claude]** Build and install: `cmake --build build`.
2. **[Jij]** Rescan plugins and put a new Kikkuh (CLAP) on an instrument track with a four-on-the-floor loop, all knobs at their defaults. Is this the kick you chose in round 8 of VS1-09? To compare: `afplay` on `B.wav` from `build/kikkuh-round tests/manual/rounds/8-check.txt --out <empty dir> --seed 108`.
3. **[Jij]** Keep your monitors low at first: the kick measures +1.2 dBFS at velocity 127, and nothing limits it until the Master section. Does the track's meter or anything after it distort?
4. **[Jij]** Turn Sweep 2 Amount from 0 to 24 and Sweep 2 Decay from 1 ms to 2 s while it loops. They do what their names say, without clicks. Is the useful part of each range easy to reach?
5. **[Jij]** Turn Start from 90 to 0 while it loops. The attack softens. At 90, is the hit a hit and not a fault? Fast notes that retrigger a ringing kick may click: that is PLAY-03's (VS1-05), not this run's.
6. **[Jij]** Open a project saved before this build, with a Kikkuh in it. It sounds as it did, with Sweep 2 Amount, Start and Drive at 0 (or Drive at what you had set).
