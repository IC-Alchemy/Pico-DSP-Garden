# Repo Layout & Source of Truth

Where the canonical code lives, and how examples consume it.

## Libraries are the single source of truth

- `libraries/rpdsp/src/rpdsp/` — **canonical rpdsp source** (header-only, MIT).
  Include as `<rpdsp/oscillator.h>`, etc.
- `libraries/pico_audio_i2s/src/pico_audio_i2s/` — **canonical audio driver**
  (vendored from pico-extras, BSD-3-Clause). Include as
  `<pico_audio_i2s/audio.h>`, `<pico_audio_i2s/audio_i2s.h>`, etc.
- `libraries/pico_audio_i2s/extras/audio_i2s.pio` — PIO source archive. The
  compiled `audio_i2s.pio.h` lives in `src/` and is what actually gets built.

## No per-example copies — edit libraries directly

- **No per-example `src/` copies exist.** The old canonical-copy propagation rule
  is retired.
- Edit the files in `libraries/` directly; all examples pick up the change
  automatically through the `--library` build flags.

## Promoted / removed modules

- **`libraries/rpdsp/src/rpdsp/hypersaw.h`** (`rpdsp::Hypersaw`) is the promoted
  Hypersaw implementation. Include it as `<rpdsp/hypersaw.h>`.
- **DaisySP is gone** (migrated to rpdsp). Do not re-add it or assume it exists.

## Licensing

- rpdsp and the repo itself are **MIT**.
- The vendored `pico_audio_i2s` driver is **BSD-3-Clause** (Raspberry Pi Trading).
  Preserve its license notices when editing.
