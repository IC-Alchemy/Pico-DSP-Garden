# benchmark

Portable block-processing benchmark; no WAV output.

- **Source**: [`benchmark.cpp`](benchmark.cpp)
- **Build target**: `rpdsp_example_benchmark`
- **Rendered output**: none (this example is a timing harness, not an audio renderer)

## What it demonstrates

Measures the per-block CPU cost of a small but representative DSP chain against
the realtime block budget at 48 kHz / default block size. The chain per sample is
a B-spline pulse oscillator → short feedback delay → sum/difference → two
compressors. It runs 10 000 blocks and reports:

- total elapsed microseconds,
- microseconds per block,
- block budget consumed as a percentage of the realtime deadline.

Timing uses `time_us_64()` on RP2350 (when `pico/time.h` is available) and
`std::chrono::steady_clock` on the host.

## Modules used

- `rpdsp::SecondOrderBSplinePulseOscillator`, `rpdsp::Delay<256>`,
  `rpdsp::Compressor` (×2).
- `rpdsp::DefaultAudioBlock`, `rpdsp::kDefaultSampleRate`,
  `rpdsp::kDefaultBlockSize`.

## Build and run

```powershell
cmake --build build-host --target rpdsp_example_benchmark
ctest --test-dir build-host -R rpdsp_example_benchmark --output-on-failure
```

## Notes

- Results are written to volatile globals (`lastMicrosecondsPerBlock`,
  `lastBlockBudgetPercent`) rather than printed, so the example never performs
  I/O from a realtime-style path. Inspect them with a debugger, SWD probe, or
  non-realtime telemetry.
- The example returns non-zero if no time elapsed at all. For true realtime
  headroom numbers, rebuild and run on RP2350 firmware with the shipping
  compiler options and callback graph.
