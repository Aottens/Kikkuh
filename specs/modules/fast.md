# Module spec: fast (FAST)

Status: **approved by the owner (2026-11-05), on the clickable prototype. Scope: FAST-01 and FAST-05, built by VS1-06.**
FAST-02..04 come with slice 5.

The panel is the product (CORE-01). This spec holds what a test can show: which controls exist, where, and what they do.
How it feels is for the owner's run. Sizes are in panel points at 100 %. The reasoning and its sources are in
`docs/research/panel-design.md`; the prototype the owner approved is at
<https://claude.ai/artifact/4NnhHpZ3C6S3ZZEUh9qcn9>.

## The panel

A printed instrument front of 1154 x 318 points. A printed line runs from the name "Kikkuh" to the output, in the real
order of the signal: the sweep drives the oscillator, so PITCH comes before OSC. Controls that belong together sit closer.

| Section | Controls (id), with knob size |
| --- | --- |
| under the name | hit pad, Loop switch, Velocity S (`play_velocity`), Key Track (`play_key_track`) |
| PITCH | Amount L (`body_pitch_amount`), Decay L (`body_pitch_decay`), Shape M (`body_pitch_shape`), under SWEEP 2: Amount S (`body_sweep2_amount`), Decay S (`body_sweep2_decay`); Tail Anchor (`body_tail_anchor`) |
| OSC | Tune L (`body_tune`), Wave M (`body_wave`), Start S (`body_start`), tail readout |
| AMP | Decay L (`body_amp_decay`), Shape M (`body_amp_shape`), Attack S (`body_amp_attack`), Hold S (`body_amp_hold`) |
| DRIVE | Drive L (`body_drive`) |
| FILTER | Cutoff L (`body_filter_cutoff`), Resonance M (`body_filter_res`), Env S (`body_filter_env`) |

**One grid.** Two rows: L and M knobs on top, S knobs and switches below. Each row has one line for the discs' centres, one for
labels and one for values. A column is 88 points wide, and a bottom control stands under the top knob of its column, in the
table's order: Sweep 2 Amount under Pitch Amount, Sweep 2 Decay under Pitch Decay, Tail Anchor under Pitch Shape, Start under Tune, the tail
readout under Wave, Attack under Amp Decay, Hold under Amp Shape, Env under Cutoff. The tail readout is laid out as a knob: the note on the
disc line, its caption on the label line, the cents on the value line. (Changed after the first run, owner, 2026-11-05.)

Knobs are 68, 52 and 38 points across (L, M, S). A knob is a flat disc with a pointer, printed ticks around it and one
heavier tick at the parameter's default. There are no gradients, shadows or glows.

**Colour has one job.** The plate is aluminium (`#c8cbc6`), print is ink (`#141513`, secondary `#383b37`). Yellow
(`#f7c600`) means one thing: a kick sounds. It lights the pad on every hit, and the lamp in the Loop switch on every loop
hit. A switch that is on is filled with ink, never yellow. Nothing moves unless the signal does.

**Type is one family,** Archivo, in four cuts: the name, section titles, labels and values.

Later sections (CLICK, TOP, RUMBLE, MASTER) get a row of their own below this one when they are built; the panel holds no
empty places for them.

## Acceptance criteria

### FAST-05 One screen

- **FAST-05a** Every parameter of the plugin has exactly one control on the panel, whose component id is the parameter id.
  A parameter added later without a control fails this test.
- **FAST-05b** Controls are in the order of the table: each section lies to the right of the one before it, and all of
  them are visible at once, inside the window, with no two controls overlapping. There are no tabs, no viewports and no
  second window. A snapshot of the panel draws, at each end of FAST-05c.
- **FAST-05c** The panel is resizable from 80 % to 200 %, at a fixed aspect ratio, and FAST-05b holds at both ends: knobs
  and text scale by one factor. The default size is 100 %, and the size comes back when the editor is reopened on the same
  instance.
- **FAST-05d** Moving a control sets its parameter, and a change from the host moves the control. Dragging is vertical:
  240 points cover the whole range, and on Tune each semitone takes 12 points, because at 240 a note is hard to hit. With Shift it is
  5 times finer, and pressing or releasing Shift during a drag does not make the value jump. The wheel moves a knob the mouse is over, and
  with Shift it is 5 times finer as well, on macOS as a horizontal wheel too. Double-click and Cmd-click (Ctrl-click on Windows) return
  it to the parameter's default.
- **FAST-05e** A knob's movement follows the parameter's own range, including its centre: the knob at its middle position
  on Pitch Decay reads 60 ms, on Cutoff 632 Hz.
- **FAST-05f** The tail readout (BODY-02d) is on the panel and shows `getTailReadout()`: after a knob changes it follows
  within 100 ms, with no host action needed. It reads from Tune, also with Key Track on, because it describes the set kick.
- **FAST-05g** The Wave knob prints its four positions (SIN, TRI, SAW, SQR), and its value reads "sine", "triangle", "saw"
  or "square" on a whole position, and the number between them.
- **FAST-05h** Every control has an accessible title that is its parameter's name, so a screen reader can tell Pitch Decay
  from Amp Decay.
- **FAST-05i** Opening and closing the editor leaves the audio sample-identical to a render without an editor.
- **FAST-05j** Clicking a knob's value lets you type. It takes a note on Tune ("F#0"), a wave's name on Wave ("saw"), and on
  other knobs a number with or without its unit and with "k" for thousands ("1k", "40 ms", "1.20 kHz"). The text a knob
  shows, typed back unchanged, leaves it on the same value within the shown precision. A value is shown with its unit,
  and Cutoff from 1000 Hz up in kHz.
- **FAST-05l** The grid holds: within a row, discs share their centre line, labels and values their line, and switches are centred on the
  disc line. A bottom control's centre lies on its column. Columns within a section are 88 points apart. The pad and the Loop switch share
  their left and right edges with the row below them, and the Loop switch ends on the top row's value line.
- **FAST-05k** Legibility: the secondary print on the plate has a contrast of at least 4.5:1, the WCAG floor for text.
  Labels and values are 13 points at 100 %, and no text on a label is below 11.

### FAST-01 Audition

The pad and the loop sound without MIDI and without the transport, so you can shape a kick with both hands on the knobs.
Both play the set kick: at full level, as a MIDI velocity of 127 at Velocity 100 %, on the note Tune says, also with Key
Track on. So with Key Track on, Tune is what the pad plays, and PLAY-02d still holds for MIDI. They run through
`BodyVoice::trigger`, so a retrigger behaves as PLAY-03 says.

Neither is a host parameter. They are not automatable, not stored in the project, and they do nothing in an offline
render, so a bounce can never hold a kick that is not in the piano roll.

The loop plays quarter notes. While the host's transport runs, the hits lie on its grid, derived from the host's PPQ in
every block. While it stands still there is no grid to follow, so the loop runs free at the host's tempo. With no play
head, or a tempo below 1 BPM, the tempo is 120 BPM.

- **FAST-01a** A press on the pad, on mouse-down, starts a kick at the first sample of the next block. It is
  sample-identical to a MIDI note at velocity 127 on that sample, with the Velocity knob at any value.
- **FAST-01b** With Key Track on and any MIDI note played before, the pad plays Tune's note: sample-identical to the same
  press with Key Track off.
- **FAST-01c** A press 50 to 310 ms after a kick keeps the largest step below PLAY-03b's limit of 1.15 at the defaults.
- **FAST-01d** Transport running at 120, 137 and 174 BPM at 44.1 and 48 kHz: every hit starts on the sample nearest its
  quarter note, within 1 sample, over 16 beats, and block sizes 64, 256 and an uneven 1, 37, 512 give the same hits.
- **FAST-01e** Exactly one hit per quarter note: a beat that falls on a block's first or last sample, or between two
  blocks, gives one hit, never zero or two. PPQ values that come with rounding noise of 1e-9 do not change that.
- **FAST-01f** The grid follows the host without accumulating: a tempo change takes effect in the block it arrives in, and
  a jump of the host's position, such as a loop back to a beat already played, puts the hits on the new position's grid at
  once and plays that beat again.
- **FAST-01g** Transport stopped at 137 BPM: hits are 60/137 s apart, within 1 sample, over 16 beats. Starting the
  transport moves the hits to the grid at once; stopping it keeps the interval from the last hit.
- **FAST-01h** With no play head, with a play head that gives no tempo, and with a tempo of 0: hits 0.5 s apart.
- **FAST-01i** With the transport stopped, switching the loop on gives a hit in the next block. With it running, the first
  hit is the next quarter note, because a hit off the grid would sound like a mistake. Switching off gives no new hit, and
  the kick that rings plays out sample-identically.
- **FAST-01j** The loop stops when the editor closes, because its switch can no longer be seen. Reopening shows it off.
- **FAST-01k** A MIDI note during the loop plays at once; the loop's next hit stays on its own grid.
- **FAST-01l** Neither pad nor loop changes the saved state: a state saved with the loop on is byte-identical to one saved
  with it off, and restoring it leaves the loop as it was.
- **FAST-01m** With `isNonRealtime()` true, a pad press and a loop that is on give silence.
- **FAST-01n** The panel can see a kick sound: a counter on the processor goes up by one on every kick, from the pad, the
  loop or MIDI, and a second one on every loop hit. The panel lights yellow from these.

### Engineering, as it applies to FAST

- **ENG-01d** `processBlock` does not allocate with the pad pressed or the loop running, transport on or off. The panel
  reaches the audio thread through atomics only.
- **ENG-02** Latency stays 0.
- **ENG-03** The same hits render identically twice.
- `source/ui/` holds no DSP. The loop's timing is plain C++ in `source/dsp/`, without JUCE.
- The panel repaints only what changes, from one timer at 30 Hz.
- `scripts/validate.sh` opens and closes this editor at pluginval strictness 10.

### CORE-01

This changes how the panel works, so CORE-01 must be re-run. VS1-07 is that run, on this panel, and it comes directly
after this build: VS1-06 ships its own checklist for what the panel does, and CORE-01's result is VS1-07's row. If it
fails, the panel is fixed before slice 2 starts.
