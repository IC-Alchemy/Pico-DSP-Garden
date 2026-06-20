# oscillator_gallery

Host-rendered stereo gallery layering naive, band-limited, hard-sync, and noise voices.

- **Source**: [`oscillator_gallery.cpp`](oscillator_gallery.cpp)
- **Build target**: `rpdsp_example_oscillator_gallery`
- **Rendered output**: `oscillator_gallery_sync_stack.wav` (10.0 s, 48 kHz, 16-bit stereo)

## What it demonstrates

A single patch that auditions the whole oscillator family side by side. A slow
phasor sweeps timbre and filter cutoff across a 16-step bassline. Four layers are
mixed into a fixed `StereoMixer<4>`:

1. **Naive oscillators** — sine, triangle, saw, and pulse (aliasing audible).
2. **Band-limited oscillators** — second-order B-spline saw and pulse.
3. **Hard-sync lead** — B-spline hard-sync saw through a sweeping Biquad low-pass.
4. **Noise air** — white noise through a one-pole low-pass.

This is the reference for comparing naive vs. band-limited oscillator quality and
for using the hard-sync and Biquad-filtered lead voices.

## Modules used

- `rpdsp::Phasor`, `rpdsp::SineOscillator`, `rpdsp::TriangleOscillator`,
  `rpdsp::SawOscillator`, `rpdsp::PulseOscillator`.
- `rpdsp::SecondOrderBSplineSawOscillator`,
  `rpdsp::SecondOrderBSplinePulseOscillator`,
  `rpdsp::SecondOrderBSplineHardSyncSawOscillator`.
- `rpdsp::NoiseOscillator`, `rpdsp::BiquadLowpass`, `rpdsp::OnePoleLowpass`.
- `rpdsp::StereoMixer<4>` with per-channel gain and pan.

## Build and run

```powershell
cmake --build build-host --target rpdsp_example_oscillator_gallery
ctest --test-dir build-host -R rpdsp_example_oscillator_gallery --output-on-failure
```

## Notes

- The naive layers alias on purpose, so the band-limited layers are directly
  comparable in the render.
- Hard-sync timbre is set with separate master and slave frequencies; the sweep
  moves the slave ratio from 2× to 6× the master.
