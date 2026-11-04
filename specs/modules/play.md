# Module spec: play (PLAY)

Status: **approved by the owner (2026-11-04). Scope: PLAY-01..03, built by VS1-05.**

Kikkuh plays one kick at a time: a new note replaces the one that rings. Time runs in seconds (ENG-04). Note names
follow BODY: MIDI 60 is C3. Unless a criterion says otherwise, "from silence" means the same note rendered with no note
before it, and tests that read the voice itself use Drive 0, as BODY-01..04 do.

## Parameters

Ids are permanent once released (ENG-06). Both knobs are read at the note-on, as Start is (BODY-10): a ringing kick
does not change, so there is nothing to smooth and nothing to click.

| Id | Knob | Range | Default | Knob feel |
| --- | --- | --- | --- | --- |
| `play_velocity` | Velocity | 0..100 % | 100 % | linear |
| `play_key_track` | Key Track | off, on | off | switch |

The defaults are today's behaviour, so a saved project keeps its sound and the state version stays 2.

## Acceptance criteria

### PLAY-01 Notes and velocity

The level of a note is `1 - Velocity * (1 - v)`, with `v` the MIDI velocity / 127. So at 100 % it is `v`, at 0 %
every note is at full level. It is applied before Drive, as today: a softer note also clips less (BODY-06c).

- **PLAY-01a** With uneven host blocks, the first sound of a note is on the note's exact sample. Two notes in one block
  each start on their own sample.
- **PLAY-01b** At Drive 0 and Start 90, where the first sample is the note's level, the peak follows the formula within
  1 %: at MIDI velocity 64 it is 0.504 at Velocity 100 %,
  0.752 at 50 % and 1.0 at 0 %; at MIDI velocity 127 it is 1.0 at each.
- **PLAY-01c** At the default Drive of 6 dB a softer note never peaks higher than a harder one, for MIDI velocities 1,
  32, 64, 96 and 127 at Velocity 100 %.
- **PLAY-01d** The kick is a one-shot: a note-off, or a note-on with velocity 0, leaves the output sample-identical to a
  render without it.
- **PLAY-01e** Moving Velocity while a kick rings leaves that kick sample-identical; the next note follows the new value.

### PLAY-02 Key tracking

With Key Track on, the note you play is the note you hear: it takes the place of Tune, and the sweeps run on top of it
as they do on Tune. Notes outside Tune's range take the nearest end of it, C0 or C3, so a note can never sweep beyond
what the knobs can reach. Times do not follow the note. Tune is not used while Key Track is on.

Measured, not a limit: at the defaults a keyed C3 peaks at +3.1 dBFS against +1.2 at C1, because the drive's filters
overshoot more on a higher sweep's first hit. Nothing limits it until the Master section (VS3-01).

- **PLAY-02a** With Key Track off, MIDI notes 24, 36, 45 and 60 render sample-identically.
- **PLAY-02b** With Key Track on and the sweeps at 0, the tail is within 10 cents of the note: 32.7 Hz at MIDI 24,
  65.4 Hz at 36, 110 Hz at 45, 261.6 Hz at 60.
- **PLAY-02c** With Key Track on, MIDI 12 plays as MIDI 24 and MIDI 72 as MIDI 60, sample-identically.
- **PLAY-02d** With Key Track on, the Tune knob at any value renders sample-identically to its default.
- **PLAY-02e** With Key Track on, the note at the default sweeps follows BODY-09a's formula with the note for Tune,
  within 50 cents, at 10, 75, 300 and 500 ms, with Amp Decay 2000 ms so that the kick still sounds at 500 ms. The -6 and
  -20 dB times at the default Amp Decay, with Drive and Start at 0, stay within 8 ms between notes 36 and 60: half a
  period of note 36, which is as closely as a peak can show it. Drive 0, because at the default Drive the times are
  read from an onset peak that itself changes with the note.
- **PLAY-02f** Switching Key Track while a kick rings leaves that kick sample-identical; the next note follows the switch.

### PLAY-03 Retrigger

A new note starts at once on its own sample, so the kick stays on the grid (ENG-02). The kick it replaces does not keep
playing: the last value it reached fades away, added to the new note before Drive, with a time constant of 3 ms. This
needs no second voice, also holds when notes pile up, and leaves Drive and the filter running as they were. There is no
knob for it. If Env Amount is on, the filter's envelope moves to its new start over the same fade, because a cutoff that
jumps with signal in the filter clicks as well; from silence it opens at once, as BODY-05c says.

A kick that starts at full level (Start 90) has a step that is the sound itself (BODY-10b). So the limit is on what the
retrigger adds, not on that hit. 200 BPM is a note every 300 ms, and its sixteenths every 75 ms.

- **PLAY-03a** The old kick stops without a click. With Wave sine, Tune C3, sweeps 0, Start 0, Attack 5 ms, Hold 500 ms
  and Drive 0, so that the new note adds no step of its own, a second note lands at 16 points across one period of the
  old tone: the largest step around it stays below 0.05, the limit of ENG-01c, at 44.1, 48, 96 and 192 kHz.
- **PLAY-03b** A second note, 50 to 310 ms after the first, keeps the largest step below 1.15 at the defaults and below
  0.75 at Start 0 with Amp Decay 2000 ms. Without the fade they reach 1.17 and 1.05. The defaults' limit lies above a hit from
  silence (0.73), because Drive clips the old kick's negative swing just before the new hit: with the anchor on or off, 13
  of 104 gaps exceed that. The first wording, within 0.05 of a hit from silence at 75 and 300 ms only, held by chance
  (changed in VS1-04, owner to confirm).
- **PLAY-03c** The filter does not click. On the values of PLAY-03a, with Cutoff 200 Hz, Env Amount 5 octaves, Hold 0,
  Amp Decay 2000 ms and Amp Shape -100, a second note at 1.6 s, where the envelope has fallen to 0.2 and the cutoff lags
  the tone, keeps the largest step below 0.05. A sine, because a saw has large steps by nature once the filter opens.
- **PLAY-03d** It is the same kick: 50 ms after a retrigger the output stays within 0.001 of the same note from silence,
  at the defaults and at Drive 0, also when the new note has another velocity or, with Key Track on, another pitch.
- **PLAY-03e** Notes that pile up: five notes 1 ms apart, on the values of PLAY-03a, keep the largest step below 0.05.
- **PLAY-03f** At the defaults, in a run at 200 BPM, the peak stays within 0.5 dB of the same note's from silence. At
  Drive 0 a long tail can add to a Start 90 hit, up to +6 dB, and nothing limits it until the Master section (VS3-01).

### Engineering, as it applies to PLAY

- **ENG-01a** Both knobs are automatable by the host. They are the exception to ENG-01b, as Start is: see Parameters.
- **ENG-01d** `processBlock` does not allocate in a run of retriggers, with Key Track on and with it off.
- **ENG-01e** Velocity shows whole percents and Key Track "Off" or "On", and each survives the round trip.
- **ENG-03, ENG-04** A run of retriggers renders identically twice. PLAY-03d holds at 44.1, 48 and 192 kHz.
- **ENG-06a** A state without these knobs gives their defaults and sounds as it did. **ENG-06b** The list of ids in the
  test holds both.
