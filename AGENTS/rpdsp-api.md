# rpdsp API Conventions

rpdsp replaced DaisySP. Names are lowercase and differ from DaisySP — agents
familiar with DaisySP will guess wrong.

## Lifecycle & naming

- `prepare(sampleRate)` — **not** `Init`. Every module must be `prepare()`d before use.
- `setFreq(hz)` / `process()` — lowercase.
- **`SineOscillator` has no `SetAmp`.** Apply gain by multiplying the `process()`
  return value.

## Helpers (usually no extra include needed)

`rpdsp::clamp(x, lo, hi)`, `rpdsp::midiNoteToHz(m)`, `rpdsp::softClip(x)`,
`rpdsp::zapDenormal(x)` live in `algorithm.h` / `realtime.h` and are pulled in
transitively via `oscillator.h`.

## Oscillator phase & anti-aliasing

- Phase convention: `process()` returns the current phase/value **before**
  advancing.
- Naive oscillators alias on saw/square. Use the `SecondOrderBSpline*` classes for
  band-limited (anti-aliased) output.
- **Read the header comment block at the top of
  `libraries/rpdsp/src/rpdsp/oscillator.h` before adding waveforms.**

## Config constants

- `RPDSP_BLOCK_SIZE` (in `config.h`) must be `16`, `32`, or `64`
  (static_assert enforced).
- Default sample-rate constant: `kDefaultSampleRate = 48000.0f`. (Note: individual
  sketches may declare their own `SAMPLE_RATE` — see
  [gotchas](gotchas.md#sample-rate-varies-across-examples).)
