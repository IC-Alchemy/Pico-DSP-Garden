# Cross-Cutting Gotchas

Things that don't belong to a single area but trip up edits across the repo.

## Sample rate

- **Sample rate is consistent at 48000 Hz** across all examples. Each sketch
  declares `SAMPLE_RATE = 48000.0f` (matching `rpdsp::kDefaultSampleRate`). Don't
  introduce a different rate without checking buffer/timing implications.

## Bit depth

- **Project default is 24-in-32 left-justified** via
  `AUDIO_BUFFER_FORMAT_PCM_S32` (`sample_stride = 8`, `int32_t *out`, pack with
  `rpdsp::toInt24x32`). All examples use this. See
  [audio-runtime.md](audio-runtime.md) "Output format" for the full contract.
- **int16 (`AUDIO_BUFFER_FORMAT_PCM_S16`, `sample_stride = 4`) remains
  supported** in the driver but is no longer exercised by any example, so it is
  compile-tested only through the asserts in `audio_i2s.c`. If you touch the
  driver, keep both the S16 and S32 paths intact.

## Dual-core split is consistent

- Core 0 (`setup`/`loop`) runs the real-time audio fill callback.
- Core 1 (`setup1`/`loop1`) runs sequencing, ADC reads, serial debug.
- Docs and examples agree on this. Don't move the audio path to Core 1 without a
  measured reason — the current split is the documented baseline.

## Non-authoritative directories

- `.kilo/` holds Kilo state (plans, `package.json`) — not source of truth.
- `.qoder/repowiki/` is another tool's generated knowledge — not source of truth.
- Both are gitignored. Neither should be edited as if it were documentation.

## See also

- [build.md](build.md) — host tests vs firmware verification; why the audio
  driver can't be reasoned about off-target.
- [architecture.md](architecture.md) — DaisySP is gone; licensing.
