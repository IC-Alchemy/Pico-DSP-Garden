# effects_rack

Host-rendered effects chain over a generated dub-style pattern.

- **Source**: [`effects_rack.cpp`](effects_rack.cpp)
- **Build target**: `rpdsp_example_effects_rack`
- **Rendered output**: `effects_rack_dub_space.wav` (9.0 s, 48 kHz, 16-bit stereo)

## What it demonstrates

A complete per-sample effects rack fed by a generated dry pattern. The signal
flow per sample is: ping-ponged delay → chorus → waveshaping → DC-blocked
stereo reverb → bus compression. Parameters (`wet`, `feedback`, `drive`) are
updated per rendered block and smoothed inside the rack so they never click.

The `DryPatternSource` layers a B-spline pulse lead, a B-spline saw bass, and a
short noise transient into a gated, Biquad-filtered pattern.

## Modules used

- `rpdsp::Delay<48000>` — 1 s ping-pong delays (187 ms / 281 ms).
- `rpdsp::Chorus<2048>` — independent rates/depths per side.
- `rpdsp::Waveshaper` — saturation drive.
- `rpdsp::StereoSchroederReverb` — stereo space with width and crossfeed.
- `rpdsp::DcBlocker` before the reverb tail.
- `rpdsp::Compressor` — per-side bus glue (threshold -11 dB, ratio 2.6).
- `rpdsp::LinearSmoother` for all parameter changes.
- `rpdsp::SecondOrderBSplinePulseOscillator` /
  `rpdsp::SecondOrderBSplineSawOscillator` / `rpdsp::NoiseOscillator` /
  `rpdsp::BiquadLowpass` as the dry source.

## Build and run

```powershell
cmake --build build-host --target rpdsp_example_effects_rack
ctest --test-dir build-host -R rpdsp_example_effects_rack --output-on-failure
```

## Notes

- Delay capacity is fixed at compile time (`kMaxDelaySamples = 48000`); choose a
  capacity that covers your longest delay time at the target sample rate.
- The compressor is configured once in `reset()` and reused; only audible
  parameters are changed per block.
