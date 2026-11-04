# Checklist: the tail anchor in Bitwig (VS1-04; BODY-07, PLAY-03b)

Result goes in `tests/manual/runs/<date>-VS1-04.md`, with `## Open points`.
The tests show that the tail is identical; this run is about what only ears can tell. Above all: whether the bend of the
sweep costs anything, because the anchor is on by default and bends the kick chosen in VS1-09 a little (-62 cents at the
note-on, gone at 50 ms).

1. **[Claude]** Build and install: `cmake --build build`.
2. **[Jij]** Rescan plugins and put a new Kikkuh (CLAP) on an instrument track, all knobs at their defaults. Loop a kick and
   flip Tail Anchor off and on. Is it still the kick you chose? Which do you prefer, or is there no difference?
3. **[Jij]** Anchor on, Amp Decay 2000 ms, against a steady sine bass on A0 or a second kick. Turn Pitch Amount, Decay and
   Shape while it loops. Does the low end stay where it was? Then the same with the anchor off. Is the difference worth having?
4. **[Jij]** The worst case of the bend: Tune A0, Pitch Amount 12, Pitch Decay 5 ms, Sweep 2 Amount 0; then Pitch Decay 10 ms
   and Amount 24. Flip the anchor. Does the attack change in a way you do not want?
5. **[Jij]** Anchor on, Pitch Decay 400 ms, Amp Decay 2000 ms. Move Pitch Amount while a kick plays. That kick misses its landing
   and slides back over 50 ms. Do you hear a wobble or a click?
6. **[Jij]** PLAY-03b: all defaults, a run of sixteenths at 280 BPM (54 ms apart), where a retrigger's step is largest. Do you
   hear a click, with the anchor on or off? This is what decides the new limit of 1.15.
