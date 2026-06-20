# Plan: Migrate DSP from DaisySP to rpdsp (3 examples; defer SuperSaw)

## Goal
Replace the DaisySP library with the new self-authored `rpdsp` DSP headers across the sine-based examples. SuperSaw is deferred (it needs a new rpdsp hypersaw that does not exist yet).

## Verified starting state
- Root: `audio/` (canonical audio driver), `DaisySP/` (canonical DSP — **not included by any build**; only `README.md:34` mentions it), `Examples/`.
- No canonical rpdsp home exists at root yet. The 17 new rpdsp headers currently live only in `Examples/SimpleOscillators/src/dsp/`.
- **Defect to fix regardless:** 7 new headers use `#include "rp2350_audio_dsp/dsp/X.h"`, a path that resolves to nothing. The build only works today because nothing pulls those headers in (SimpleOscillators.ino includes only `oscillator.h`, which already uses correct relative includes).
- `Examples/SimpleOscillators/src/dsp/` = 17 rpdsp headers + 3 DaisySP orphans (`dsp.h`, `ladder.h`, `ladder.cpp`; nothing includes `ladder.h`).
- rpdsp is **fully header-only**. The only `.cpp` in any dsp tree is `ladder.cpp` (DaisySP).
- SuperSaw is self-contained: its `src/dsp/` (dsp.h, oscillator.h/.cpp, svf.h/.cpp, SuperSaw.h) carries its own DaisySP-derived copies and does **not** reference root `DaisySP/`.

## Decisions (locked)
1. **Scope:** SimpleOscillators, Oscillators, TwoChannelOscillator → rpdsp. SuperSaw deferred.
2. **Canonical home:** new root `dsp/` is the rpdsp source of truth; each example copies it to `src/dsp/` (mirrors `audio/`→`src/audio/`). No symlinks.
3. **Include style:** relative sibling includes inside all rpdsp headers. Normalize the 15 broken `rp2350_audio_dsp/dsp/X.h` lines → `X.h`. (Confirmed: quoted includes resolve to the including file's directory; SimpleOscillators already compiles with this pattern.)
4. **DaisySP disposition:** delete root `DaisySP/`; delete DaisySP-derived `src/dsp/` files in the 2 migrated examples and replace with rpdsp copies. SuperSaw keeps its legacy self-contained copies (deferred, not abandoned).
5. **Verification:** no test framework (per AGENTS.md). Verify by Arduino IDE Verify/Compile, then flash + listen.

## Semantic note for the implementer (critical)
`rpdsp::SineOscillator` has **no `SetAmp()`** — apply amplitude by multiplying the `process()` output.

API map (DaisySP → rpdsp):
- `daisysp::Oscillator` → `rpdsp::SineOscillator`
- `Init(sr)` → `prepare(sr)`
- `SetFreq(hz)` → `setFreq(hz)`
- `Process()` → `process()`
- `SetAmp(a)` → drop; multiply `process()` output by `a`
- `SetWaveform(WAVE_SIN)` → drop (SineOscillator is always sine)
- `daisysp::mtof(m)` → `rpdsp::midiNoteToHz(m)`
- `daisysp::fclamp(x, lo, hi)` → `rpdsp::clamp(x, lo, hi)`
- `clamp` and `midiNoteToHz` live in `algorithm.h`, pulled in transitively via `oscillator.h` — **no new include needed** in the sketches.

## Tasks (ordered)

### Task 1 — Normalize includes in the 7 affected rpdsp headers
In `Examples/SimpleOscillators/src/dsp/`, change `#include "rp2350_audio_dsp/dsp/X.h"` → `#include "X.h"`:
- `dynamics.h` — 1 line (`algorithm.h`)
- `effects.h` — 5 lines (`algorithm.h`, `delay_line.h`, `filter.h`, `oscillator.h`, `realtime.h`)
- `filter.h` — 2 lines (`algorithm.h`, `realtime.h`)
- `knob_bank.h` — 1 line (`pickup_knob.h`)
- `parameter_smoother.h` — 1 line (`config.h`)
- `rhythm_sequencer.h` — 1 line (`algorithm.h`)
- `voice.h` — 4 lines (`algorithm.h`, `envelope.h`, `filter.h`, `oscillator.h`)

Already correct (leave alone): `config.h`, `algorithm.h`, `realtime.h`, `delay_line.h`, `envelope.h`, `gate_pattern.h`, `joystick_recorder.h`, `pickup_knob.h`, `oscillator.h`.

### Task 2 — Promote rpdsp to canonical root `dsp/`
Create root `dsp/` and copy the 17 normalized rpdsp headers there (do **not** copy `dsp.h`, `ladder.h`, `ladder.cpp`). This root `dsp/` becomes the single source of truth.

### Task 3 — SimpleOscillators cleanup
- Delete orphan DaisySP files from `Examples/SimpleOscillators/src/dsp/`: `dsp.h`, `ladder.h`, `ladder.cpp`.
- Confirm `Examples/SimpleOscillators/src/dsp/` now matches canonical root `dsp/` exactly.
- `SimpleOscillators.ino` is already rpdsp-only — leave unchanged.

### Task 4 — Migrate Oscillators
- Replace `Examples/Oscillators/src/dsp/` contents with a copy of canonical root `dsp/` (delete the DaisySP-derived `dsp.h`, `oscillator.h`, `oscillator.cpp`, `SuperSaw.h`).
- Rewrite `Examples/Oscillators/Oscillators.ino`:
  - `daisysp::Oscillator carrier_osc[NUM_OSCILLATORS]` → `rpdsp::SineOscillator carrier_osc[NUM_OSCILLATORS]`; same for `lfo_mod`.
  - `initOscillators()`: `Init`→`prepare`; drop `SetWaveform`; `SetFreq(mtof(...))`→`setFreq(midiNoteToHz(...))`; drop `SetAmp` lines.
  - `fill_audio_buffer()`: `lfo_mod[j].Process()`→`process()`; keep `amp_mod=(lfo_out+1)*0.5`; drop `carrier_osc[j].SetAmp(amp_mod)`; change `mixed_signal += carrier_osc[j].Process()` → `mixed_signal += carrier_osc[j].process() * amp_mod`.
  - `convertSampleToInt16()`: `daisysp::fclamp` → `rpdsp::clamp`.

### Task 5 — Migrate TwoChannelOscillator
- Replace `Examples/TwoChannelOscillator/src/dsp/` contents with a copy of canonical root `dsp/` (delete the DaisySP-derived `dsp.h`, `oscillator.h`, `oscillator.cpp`).
- Rewrite `Examples/TwoChannelOscillator/TwoChannelOscillator.ino`:
  - `daisysp::Oscillator osc_left/osc_right` → `rpdsp::SineOscillator`.
  - `setup()`: `Init`→`prepare`; drop `SetWaveform`; `SetFreq`→`setFreq`; drop `SetAmp(0.8f)` (bake `0.8f` into the callback as a multiply).
  - `fill_audio_buffer()`: `SetFreq`→`setFreq`; `osc_left.Process()`→`osc_left.process() * 0.8f`; same for right.
  - `toInt16()` and `loop1()`: `daisysp::fclamp` → `rpdsp::clamp`.

### Task 6 — Delete root `DaisySP/`
Safe: no build references it (verified); SuperSaw is self-contained. Delete the directory.

### Task 7 — Update docs
- `AGENTS.md`: replace the "root `audio/` and `DaisySP/` are the canonical sources" statement — canonical DSP is now root `dsp/` (rpdsp, header-only). Note that SuperSaw retains legacy DaisySP-derived `src/dsp/` copies pending a future migration that requires a new rpdsp hypersaw. Update the DaisySP section accordingly.
- `README.md` line 34: replace the `DaisySP/` description with a `dsp/` description (self-authored rpdsp DSP library).

### Task 8 — Verify
- For each of the 3 migrated examples: Arduino IDE Verify/Compile (board `rp2040:rp2040:rpipico2`).
- Flash + listen on hardware. Confirm: SimpleOscillators unchanged sound; Oscillators = 12 sine carriers with LFO amplitude modulation; TwoChannelOscillator = independent L/R sines with slider control.
- Confirm SuperSaw still compiles (untouched) after deleting root `DaisySP/`.

## Risks
- **Amplitude handling:** must preserve per-sample LFO amplitude in Oscillators.ino and the `0.8f` gain in TwoChannelOscillator.ino via explicit multiply (no `SetAmp`).
- **Include resolution:** mitigated — relative includes are already proven by the current SimpleOscillators build.
- **DaisySP deletion:** mitigated — verified no build depends on root `DaisySP/`.
- **Header-only safety:** unused rpdsp headers incur zero compile cost (no standalone `.cpp`), so copying the full set into each example is safe.

## Out of scope
- SuperSaw migration (blocked on a new rpdsp hypersaw).
- Pre-existing `envelope.h` private `kDefaultSampleRate = 44100.0f` vs `config.h` 48000 (harmless; self-contained; not touched).
- Adding automated tests / CI (none exist; not requested).
