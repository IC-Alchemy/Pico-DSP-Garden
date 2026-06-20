# Roadmap

Design intent for future work. **None of the APIs below exist yet.** They
appear in the historical design docs (`algorithm_catalog.md`,
`rp2350_tuning.md`, `realtime_rules.md` prior to the 2026-06-20 cleanup) as
aspirational and were relocated here to stop them from masquerading as current
behavior.

Everything in the current `libraries/` is documented in
[`algorithm_catalog.md`](algorithm_catalog.md) and is the source of truth.

## Aspirational block-processing framework

The shipped `rpdsp` API is per-sample: sketches implement
`fill_audio_buffer(audio_buffer_t*)` and call `process(float)` on each module.
The framework below was designed but not built:

- **`AudioBlock<Capacity>` / `DefaultAudioBlock`** — fixed-capacity stereo
  sample container (contiguous L/R arrays), `setFrameCount()` for partial
  blocks, `applyGain()`.
- **`I2sSampleBridge`** — accepts interleaved signed 16-bit / Arduino-Pico
  left-aligned 24-bit / signed 32-bit stereo; outputs float `AudioBlock` and
  clipped interleaved integers for I2S. Validation enforces BCLK/LRCLK
  adjacent pin pair, two channels for stereo, 8/16/24/32-bit widths.
- **`PioI2sAudio`** — wrapper over the Arduino-Pico driver (the constraints
  above come from it).
- **`StereoSample` / `StereoMixer<Channels>`** — no-allocation stereo routing.
- **`CallbackPerformanceMeter`** — exposes `lastDurationUs`, `worstDurationUs`,
  `overrunBlocks`, `cpuLoadPercent`, `worstCpuLoadPercent`; read from
  non-realtime code only.
- **`MidiByteParser`** — adapter boundary producing `MidiNoteEvent` /
  `MidiControlEvent`; lives on the control side, not in the audio callback.
- **`SmoothedAdcParameter`** — calibrated, smoothed ADC parameter.
- **`GateDebouncer`** — debounce for switches/gates/triggers from GPIO or ADC.

## Sketch ideas and references

See [`roadmap/example_sketches/`](roadmap/example_sketches/) for sketch-idea
`.cpp`/`.md` pairs, and [`roadmap/dsp_paper_shortlist.md`](roadmap/dsp_paper_shortlist.md)
for the reference paper list. These are notes, not shipping code.
