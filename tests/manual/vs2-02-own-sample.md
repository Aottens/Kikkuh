# Checklist: your own sample in Bitwig (VS2-02; LAYER-04)

Result goes in `tests/manual/runs/<date>-VS2-02.md`, with `## Open points`.
The tests show that a dropped file loads into its section off the audio thread, plays at its pitch and rate with Tune and
Start, ends without a click, comes back with the project after the file is deleted, and that a damaged or absent sample
gives silence and a message. This run is about what only a host, hands and ears can tell: does dragging a file in from
Bitwig's browser or Finder work, and does your own sample give TOP (and Beefy) what the built-in sources did not. CORE-01
itself is VS2-03.

1. **[Claude]** Build and install: `cmake --build build`.
2. **[Jij]** Rescan plugins and open the `CORE-01` project: does every track still sound as it did? Then put a new Kikkuh
   (CLAP) on a track and loop it.
3. **[Jij]** Drag a short one-shot (a hat, a snare top, a click) from Bitwig's browser onto TOP, and one from Finder onto
   CLICK. Does each land in the section you dropped it on, does Source say "Sample", is it heard at once (-6 dB), and does
   the file name show under Low-pass? Try a WAV, an AIFF and a FLAC if you have them.
4. **[Jij]** With the loop running: turn Start from 0 to a few hundred ms, and Tune up and down. Does Start skip into the
   file as you expect, and does Tune change its pitch and length like a sampler? Any click when a file ends, or when you
   drop a new one while it plays?
5. **[Jij]** Save the project, close it, rename or move the files you dropped, and open the project again. Does it sound
   the same, with the names still shown?
6. **[Jij]** Drop something that is not audio, or a damaged file, on a section with a sample: the sample should keep
   playing and the panel say "Cannot read". Step Source to Sample on a section without a file: silence and "Drop a sample".
7. **[Jij]** TOP with your own sample: is there now a top you would use? And Beefy SM ELE: does a sample of your own in
   CLICK or TOP get closer to its almost acoustic character? Not timed: that is VS2-03.
8. **[Jij]** The panel: are Start and the file name where you expect them, and is the three-step Source switch clear?
