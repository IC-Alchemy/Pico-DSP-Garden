# codec_bridge_loopback

Host-rendered loopback through the I2S 24-bit sample bridge.

- **Source**: [`codec_bridge_loopback.cpp`](codec_bridge_loopback.cpp)
- **Build target**: `rpdsp_example_codec_bridge_loopback`
- **Rendered output**: `codec_bridge_loopback.wav` (6.0 s, 48 kHz, 16-bit stereo)

## What it demonstrates

A host simulation of the codec transport boundary. Two pulse oscillators are
converted to left-aligned 24-bit interleaved I2S samples, decoded back into a
float `AudioBlock`, run through a `DspCallback`-style processor (a Biquad
low-pass with mono sum and soft clipping), then re-encoded to 24-bit interleaved
output and decoded again for the WAV render.

This is the reference for the exact float↔integer conversion contract used at the
RP2350 I2S edge, and for invoking a processor through the `DspCallback` function
pointer + `userData` signature.

## Modules used

- `rpdsp::I2sSampleBridge` — `floatToInt24`, `int24ToFloat`,
  `int24InterleavedToBlock`, `blockToInt24Interleaved`.
- `rpdsp::DspCallback` (function-pointer callback) with `userData`.
- `rpdsp::PulseOscillator`, `rpdsp::BiquadLowpass`, `rpdsp::softClip`.

## Build and run

```powershell
cmake --build build-host --target rpdsp_example_codec_bridge_loopback
ctest --test-dir build-host -R rpdsp_example_codec_bridge_loopback --output-on-failure
```

## Notes

- 24-bit I2S samples are stored left-aligned in `std::int32_t`; the bridge
  handles the shift so DSP code only sees `[-1, 1]` floats.
- On hardware the runtime performs the same conversions; this example lets you
  audition the round-trip and processor without a codec.
