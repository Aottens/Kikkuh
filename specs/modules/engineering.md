# Module spec: engineering (ENG)

Status: **approved with the PRD scope (2026-11-03); ENG-01..04 and 06 are enforced from VS0-01, ENG-05 from VS3-02.**

## Acceptance criteria

- **ENG-01** Every parameter is host-automatable and smoothed. `processBlock` never allocates (test with `AllocationGuard`) and flushes denormals.
- **ENG-02** The plugin reports zero latency.
- **ENG-03** The same note with the same settings renders identically, also across different host block sizes.
- **ENG-04** Tail frequency and peak level stay within 1 % from 44.1 to 192 kHz.
- **ENG-05** Under 3 % CPU per instance on an M-series Mac at 48 kHz / 256 samples.
- **ENG-06** Saved state carries a version and survives updates; parameter ids are permanent once released.
  **ENG-06a, exact (VS2-01):** a reloaded state gives every parameter exactly the value it was saved with, as clap-validator
  checks: values are written with nine significant digits, a skewed knob maps back to exactly its value, and a value only
  approximately equal to the current one is still set.
