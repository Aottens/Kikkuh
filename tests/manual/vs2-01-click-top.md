# Checklist: Click and Top in Bitwig (VS2-01, VS2-04; LAYER-01..03)

Result goes in `tests/manual/runs/<date>-VS2-01.md`, with `## Open points`.
The tests show that the sections add to the body without changing it, start on the note's sample, end exactly at Decay
and do not click on a retrigger or a knob jump. This run is about what only ears and hands can tell: does a click or a top
make the kick better, and does the new band read and play well. CORE-01 itself comes after VS2-02, in VS2-03.

1. **[Claude]** Build and install: `cmake --build build`.
2. **[Jij]** Rescan plugins and put a new Kikkuh (CLAP) on an instrument track, all at their defaults, and loop it. CLICK and
   TOP are Off: is it exactly the kick you know? Then open an older project (the `CORE-01` project): does every track still
   sound as it did?
3. **[Jij]** CLICK: turn Level up from Off on Tone, then try Decay 2-10 ms and Tune. Then Noise with High-pass 2-5 kHz. Does a
   click ever make the default kick better, and which kind? (The tone click of VS1-09 is Tone, Tune 0, Decay 4 ms, Level -6 dB.)
4. **[Jij]** TOP: turn Level up on Noise (High-pass 1 kHz), then Decay from 5 to 200 ms, Low-pass down, Tune down. Is there a
   setting that gives the kick a top you would use?
5. **[Jij]** Beefy SM ELE against the reference clip: the tests found CLICK on Noise, Level +6, Decay 8 ms, High-pass 1500 Hz
   with TOP on Noise, Level +6, Decay 10 ms, High-pass 1000 Hz (on BODY near Beefy) bright enough in its first 5 ms. Does it
   get closer to its "almost acoustic" character, or what is still missing? Not timed: that is VS2-03.
6. **[Jij]** Turn every new knob while the loop runs, Level from Off to +6 and back, and flip Source. Any click, zipper or
   jump? Note: Level +6 on both can push the peak far above 0 dBFS, which nothing limits until the Master section.
7. **[Jij]** The panel: is the second band easy to read, are CLICK and TOP where you expect them, and is a switch that says
   "Tone" or "Noise" clear?
