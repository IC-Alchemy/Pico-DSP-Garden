# RP2350 Tuning

This guide captures tuning assumptions for the approved target: RP2350, 48 kHz stereo, 32-sample blocks, and an Arduino-Pico I2S codec path.

## Baseline Configuration

- Sample rate: `48000.0f`.
- Block size: `32` frames.
- Channels: stereo.
- DSP format: `float`.
- Codec path: platform-managed Arduino-Pico I2S.
- License: MIT.


## Latency

At 32 frames and 48 kHz, one processing block is about 0.667 ms. End-to-end latency also includes:

- Codec conversion latency.
- Arduino-Pico I2S DMA buffering.
- Any platform double buffering.
- Control smoothing time.
- Any external audio routing.

Use 32 frames as the default balance between low latency and manageable callback overhead.

## CPU Budget

Measure the full graph on the target firmware build. Host timing is useful for regressions, but it does not represent RP2350 flash/cache/compiler behavior.

Recommended budget policy:

- Measure worst-case, not average-only.
- Measure with all voices active and all feedback effects enabled.
- Keep the DSP graph below half the block period where possible.
- Leave margin for DMA/IRQ overhead and cross-core communication.

For a 32-frame block, the hard block period is about 0.667 ms. A conservative DSP target is below 0.333 ms.

## Memory Budget

Prefer static and stack-visible storage. Delay-based algorithms should make their maximum storage obvious:

```cpp
class DelayEffect {
 private:
  rpdsp::DelayLine<48000> leftDelay_;
  rpdsp::DelayLine<48000> rightDelay_;
};
```

That example reserves one second per channel at 48 kHz. It is predictable, but not free. Use shorter capacities when the musical feature allows it.

## Dual-Core Scheduling

A practical default is:

- Core 0 owns control, USB, UI, patch state, and non-realtime services.
- Core 1 owns the audio callback and DSP graph.

Do not split one sample block across cores until a single-core graph has been measured and proven insufficient. Cross-core synchronization can cost more than a small DSP module.

## Compiler and Math Choices

Use C++17 and avoid exceptions in the realtime path. Keep math choices explicit:

- Prefer simple polynomial or one-pole approximations in per-sample paths.
- Precompute coefficients in `prepare` or on parameter changes.
- Avoid repeated `std::pow`, `std::exp`, and `std::sin` per sample unless the benchmark proves the cost is acceptable.
- Use phase accumulators for oscillators.
- Use lookup tables only when their memory cost is explicit.

## I2S and Codec Boundary

The DSP library does not own codec register details, and it does not maintain a custom PIO/DMA I2S implementation. A board support layer should:

- Configure system and peripheral clocks.
- Configure Earle Philhower's Arduino-Pico `I2S` transport.
- Configure codec sample rate, word length, format, and analog routing.
- Convert hardware buffers into `rpdsp::DefaultAudioBlock` or equivalent views.
- Call the DSP graph.
- Convert processed floats back to the codec sample format.

`PioI2sAudio` is a wrapper over the Arduino-Pico driver. Its validation reflects that driver's constraints: BCLK and LRCLK are an adjacent pin pair, standard stereo uses two channels, TDM requires output-capable mode, and only 8/16/24/32-bit sample widths are accepted.

Keep codec control outside the audio callback unless the platform has a proven nonblocking register path.

## Bring-Up Order

1. Build and run host DSP tests.
2. Run a silent callback on target and confirm stable Arduino-Pico I2S.
3. Run the Arduino-Pico firmware template with the starter patch and default gain mapping.
4. Add peak metering outside the callback.
5. Add one processor at a time.
6. Benchmark the full graph.
7. Add control smoothing and cross-core parameter publication.
8. Stress test with all voices/effects active.
