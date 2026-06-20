# pass_through

Host-rendered metered stereo gain example.

- **Source**: [`pass_through.cpp`](pass_through.cpp)
- **Build target**: `rpdsp_example_pass_through`
- **Rendered output**: `pass_through_metered_gain.wav` (6.0 s, 48 kHz, 16-bit stereo)

## What it demonstrates

The simplest end-to-end block callback: a stereo gain processor with smoothed gain
movement and per-block RMS/peak metering. A `MeteredGainDemo` source fills a block
with three panned sine tones driven by a slow LFO, then runs it through the
callback-style `PassThroughProcessor`.

This is the recommended starting point for learning the `prepare()` / `reset()` /
`process(DefaultAudioBlock&)` lifecycle and how metering is scoped per block.

## Modules used

- `rpdsp::LinearSmoother` — click-free gain ramps.
- `rpdsp::RmsPeakMeter` — block-scoped RMS and peak metering.
- `rpdsp::SineOscillator` — three panned tones plus a gain-control LFO.
- `rpdsp::equalPowerPanLeft/Right`, `rpdsp::lerp`, `rpdsp::midiNoteToHz`.

## Build and run

```powershell
cmake -S . -B build-host -DRPDSP_BUILD_HOST_TESTS=ON
cmake --build build-host --target rpdsp_example_pass_through
ctest --test-dir build-host -R rpdsp_example_pass_through --output-on-failure
```

The WAV file is written to the build directory next to the executable.

## Wiring into firmware

`processAudioBlock(rpdsp::DefaultAudioBlock&)` is the callback-style entry point.
Call `processor.prepare(sampleRate, blockFrames)` and `processor.setGain(...)`
before audio starts, then hand the block from your realtime callback to
`processor.process(io)`.
