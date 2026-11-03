# Module spec: build (BUILD)

Status: **built with VS0-01; owner review in its PR.**

## Acceptance criteria

- **BUILD-01** A fresh clone builds VST3, AU and CLAP with `cmake -B build -G Ninja && cmake --build build` on macOS arm64; one `CMakeLists.txt`, dependencies via `FetchContent` pinned to a tag or SHA.
- **BUILD-02** The plugin loads as an instrument in Bitwig (latest stable) on Apple Silicon as native arm64.
- **BUILD-03** CI builds a universal macOS binary (arm64 and x86_64) on every push and proves it with `lipo`.
- **BUILD-04** CI builds and runs the tests on Windows x64 on every push, so the code stays portable.
- **BUILD-05** Name, company and identifiers follow `DECISIONS.md`; no audio input, stereo output, MIDI input.
- **BUILD-06** A MIDI note-on plays a kick, starting at the note's exact sample at any block size; without a note the output is silent. The fixed test kick of VS0-01 was replaced by BODY in VS1-01.
