# Cross-Cutting Gotchas

Things that don't belong to a single area but trip up edits across the repo.

## Sample rate varies across examples

- **Sample rate is NOT consistent across examples.**
  - SuperSaw = **48000 Hz**
  - Oscillators, SimpleOscillators, TwoChannelOscillator = **44100 Hz**
- Each sketch declares its own `SAMPLE_RATE`. Don't assume one value, and don't
  "normalize" them without checking buffer/timing implications.

## Non-authoritative directories

- `.kilo/` holds Kilo state (plans, `package.json`) — not source of truth.
- `.qoder/repowiki/` is another tool's generated knowledge — not source of truth.
- Neither should be edited as if it were documentation.

## See also

- [build.md](build.md) — why the audio driver can't be reasoned about off-target.
- [architecture.md](architecture.md) — DaisySP is gone; licensing.
