# Checklist: Drive and filter by ear in Bitwig (VS1-03; BODY-05, BODY-06)

Result goes in `tests/manual/runs/<date>-VS1-03.md`, with `## Open points`.
The tests measure the numbers; this run is about what only ears can tell. The filter's type is a proposal, since no
filter won a round in VS1-09, so say whether it is the right one.

1. **[Claude]** Build and install: `cmake --build build`.
2. **[Jij]** Rescan plugins and put Kikkuh (CLAP) on an instrument track with a four-on-the-floor loop. All knobs at their defaults: the kick sounds as it did before this build.
3. **[Jij]** Turn Drive slowly from 0 to 24 dB while it loops. It gets louder and harder without clicks, and the tail stays clean at low Drive. Does 6 dB sound like what you chose in VS1-09?
4. **[Jij]** At Drive 24 dB, listen to the top end: any metallic or inharmonic sound that does not belong to the note? Also try a higher Tune, around C2 to C3.
5. **[Jij]** Keep your monitors low: Resonance 100 % with Cutoff on the note measures 23 dB louder, and nothing limits it yet. Turn Cutoff down from the top while it loops, then Resonance up, then Env Amount up with Cutoff around 200 Hz. They do what their names say, without clicks. Is this filter useful on a kick, and is the useful part of each range easy to reach?
6. **[Jij]** Save, close and reopen the project: the four new knobs come back.
