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

### IDEA-01 — Send and return for external rumble
The owner's idea: send the kick out to your own rumble chain and take the result back into Kikkuh.
See X-01 in `specs/PRD.md`. The bus layout must be settled before release, so the spike VS4-01 comes
before the feature. Open question: do hosts allow the loop, and if not, is a plugin slot inside
Kikkuh worth the work?
