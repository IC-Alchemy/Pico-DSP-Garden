# modulated_space

Host-rendered slowly modulated diffusion pad.

- **Source**: [`modulated_space.cpp`](modulated_space.cpp)
- **Build target**: `rpdsp_example_modulated_space`
- **Rendered output**: `modulated_space_diffusion_pad.wav` (11.0 s, 48 kHz, 16-bit stereo)

## What it demonstrates

A wide ambient pad that exercises the low-level delay/reverb building blocks
directly, rather than the higher-level `Delay` / `StereoSchroederReverb`
wrappers. Five triangle oscillators form a chord, are low-passed, then fed
through a moving fractional-delay feedback tap into comb filters, allpass
diffusers, a shared mono reverb, and per-side chorus.

This example is the best reference for hand-wiring a custom reverb/diffusion
topology from `DelayLine`, `CombFilter`, and `AllpassFilter`.

## Modules used

- `rpdsp::DelayLine<4096>` with `readCubic()` for the moving shimmer tap.
- `rpdsp::CombFilter<2048>` and `rpdsp::AllpassFilter<512>` for per-side
  diffusion (prime delay lengths 1103/1471 and 337/421).
- `rpdsp::Chorus<2048>` — widening chorus on each output side.
- `rpdsp::SchroederReverb` — shared mono room blended back in.
- `rpdsp::OnePoleLowpass`, `rpdsp::TriangleOscillator`, `rpdsp::SineOscillator`
  (drift LFO), `rpdsp::zapDenormal`, `rpdsp::softClip`.

## Build and run

```powershell
cmake --build build-host --target rpdsp_example_modulated_space
ctest --test-dir build-host -R rpdsp_example_modulated_space --output-on-failure
```

## Notes

- `zapDenormal()` is applied to the shimmer path to keep long-feedback delay
  networks from stalling on subnormal floats — important on Cortex-M33 without
  FTZ.
- Unlike most examples, this one does not expose a `processAudioBlock` callback;
  `main()` renders the `ModulatedSpace` processor directly.
