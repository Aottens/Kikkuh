# Kikkuh — Ideas inbox

The owner drops ideas in chat. Claude records each one here, proposes a phase and module, and
guards the scope. An idea gets no criteria until its phase is planned.

How an idea moves: **new** → **triaged** (phase, module, open questions; owner confirms) →
**specced** (criteria in `specs/modules/`) → **backlog** (a row in `BACKLOG.md`) → **done** or
**rejected** (with the reason). Every `### IDEA-nn` section has a row in the table and the other
way round; a task status named in a row ("VS1-02 todo") must match BACKLOG
(`scripts/check-tracking.py`).

## Ideas

| Id | Idea | Status | Phase | Module |
| --- | --- | --- | --- | --- |
| IDEA-01 | Send and return for external rumble | triaged, VS4-01 todo | after v1 | rumble |
| IDEA-02 | A sound engine round, chosen by ear | done, VS1-09 done | slice 1, before VS1-03 | body |
| IDEA-03 | Tuning between notes | new | after v1, proposed | body |

### IDEA-01 — Send and return for external rumble
The owner's idea: send the kick out to your own rumble chain and take the result back into Kikkuh.
See X-01 in `specs/PRD.md`. The bus layout must be settled before release, so the spike VS4-01 comes
before the feature. Open question: do hosts allow the loop, and if not, is a plugin slot inside
Kikkuh worth the work?

### IDEA-02 — A sound engine round, chosen by ear
The owner wants high quality, not a kick like any other. The BODY voice of VS1-01 is correct but plain,
and the tests show that it works, not that it sounds good. Claude cannot hear, so the choice has to be
the owner's. Proposal, after VS1-02 (`kikkuh-render`) and before VS1-03:

1. The owner picks reference kicks. These are also the three that CORE-01 needs.
2. Claude measures them (sweep, harmonics, the first milliseconds) and writes down what BODY lacks.
3. Claude renders variants of saturation, filter and curve shapes, level-matched and unnamed. The
   owner chooses by ear in short rounds.
4. The choices become the criteria of VS1-03, and new defaults for BODY. A run in VS1-01 showed the
   defaults are far from a kick with more tick: reachable with Amount 60, Pitch Decay 40, Shape +30.

VS1-08 chose four references and measured them: `docs/research/reference-kicks.md`.

VS1-09 ran the rounds: `tests/manual/runs/2026-11-04-VS1-09.md`. The owner chose a fast sweep with a second stage, a
start at full level and hard clipping at 6 dB, with no filter and no dents; criteria BODY-05, 06, 08, 09 and 10.

Left open: is the character also the oscillator (the rounds kept the sine); may hits vary a little like analog
hardware, which ENG-03 now forbids.

### IDEA-03 — Tuning between notes
References sit up to 44 cents between notes, and Tune has whole notes only. In VS1-09's round 7 the owner hardly
heard -22 or +44 cents on single hits, so nothing was specced. Against a bass line it may matter. Open question: is
that worth a Fine knob on the one panel?
