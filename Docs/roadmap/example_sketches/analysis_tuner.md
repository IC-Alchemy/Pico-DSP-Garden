# analysis_tuner

Host-rendered tuning and dynamics probe over a moving tone.

- **Source**: [`analysis_tuner.cpp`](analysis_tuner.cpp)
- **Build target**: `rpdsp_example_analysis_tuner`
- **Rendered output**: `analysis_tuner_dynamics_probe.wav` (7.0 s, 48 kHz, 16-bit stereo)

## What it demonstrates

A synthetic carrier steps through an 8-note phrase with slow vibrato while two
pitch detectors (zero-crossing and bounded YIN) track it in parallel. The same
signal drives the split compressor stages used as a gain-reduction probe:

`CompressorStaticCurve` computes the target reduction from a smoothed detector
level, `GainReductionSmoother` ramps it, and the result is applied as gain.

The left output channel is the dry carrier; the right channel is the
dynamics-processed tone plus a small envelope-follower amount, so you can hear
the analysis vs. control paths side by side.

## Modules used

- `rpdsp::ZeroCrossingPitchDetector` and `rpdsp::YinPitchDetector<1024, 512>`
  (range 70–900 Hz, threshold 0.18, 2-block update interval).
- `rpdsp::RmsPeakMeter` for output metering.
- `rpdsp::EnvelopeFollower`, `rpdsp::CompressorStaticCurve`, `rpdsp::GainReductionSmoother`.
- `rpdsp::SineOscillator` (carrier + vibrato), `rpdsp::gainToDb` /
  `rpdsp::dbToGain`.

## Build and run

```powershell
cmake --build build-host --target rpdsp_example_analysis_tuner
ctest --test-dir build-host -R rpdsp_example_analysis_tuner --output-on-failure
```

The example returns non-zero if the rendered RMS is not positive, so the smoke
test doubles as a basic sanity check that analysis ran.

## Notes

- This example shows how to reuse the compressor's *internal* stages
  (curve/envelope-follower/smoother) independently of the full `Compressor` wrapper.
- The last RMS value is stored in the volatile `lastAnalysisRms` for inspection
  with a debugger.


