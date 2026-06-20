# AGENTS.md

Pico-DSP-Garden — an embedded audio DSP framework for RP2350/RP2040
(Raspberry Pi Pico 2) with I2S output. Arduino sketches built on the
earle-philhower **arduino-pico** core (not `arduino:mbed_rp2040`).

> **Local guidance, not shipped.** This file and the `AGENTS/` directory are
> gitignored. They are agent instructions, not part of the published repo.

## Critical — read first

- **No host build, no test framework, no CI, no CMake/PlatformIO.** Do not invent
  test/lint/typecheck commands; none exist. Verification = *compile the sketch,
  flash, listen.* See [build.md](AGENTS/build.md).
- **Libraries are the single source of truth.** Edit `libraries/rpdsp` and
  `libraries/pico_audio_i2s` directly — there are no per-example `src/` copies.
  All examples pick up changes via `--library` build flags. See
  [architecture.md](AGENTS/architecture.md).
- **DaisySP is gone** (migrated to rpdsp). Do not re-add it or assume it exists.

## Detailed guidance — read the file for the area you're editing

- [build.md](AGENTS/build.md) — toolchain, FQBN, arduino-cli command, `build/` dir
- [architecture.md](AGENTS/architecture.md) — canonical paths, promoted modules, licensing
- [rpdsp-api.md](AGENTS/rpdsp-api.md) — `prepare`/`process`, anti-aliasing, block size
- [audio-runtime.md](AGENTS/audio-runtime.md) — callback contract, dual-core split, I2S pins
- [gotchas.md](AGENTS/gotchas.md) — sample-rate variance, non-authoritative dirs

## Reference docs in the repo

- `Docs/realtime_rules.md` — real-time callback contract (read before audio work)
- `Docs/rp2350_tuning.md` — CPU/memory/latency budgets, dual-core guidance
- `Docs/algorithm_catalog.md` — DSP algorithm reference
- `README.md` — hardware wiring, dual-core pattern overview
- **Ignore** `Docs/example_Ideas/` — not reference material.
