# Realtime Rules

The audio callback is a hard realtime section. At 48 kHz with 32-sample blocks, the processor has about 0.667 ms to finish each stereo block before the next DMA half-buffer needs service. The safe design is boring, bounded, and measurable.

## Callback Contract

Inside `process(...)`:

- Do not allocate or free memory.
- Do not use `new`, `delete`, `malloc`, `free`, or STL containers that may allocate.
- Do not log, print over USB, write files, or format strings.
- Do not block on locks, semaphores, queues, USB, I2C, SPI, or codec transactions.
- Do not change codec registers.
- Do not call sleep functions.
- Do not run unbounded loops.
- Do not depend on exceptions for control flow.

Allowed work:

- Arithmetic on samples.
- Reads and writes to preallocated buffers.
- Simple deterministic branches.
- Consuming already-published parameter values.
- Writing small meter values for non-realtime code to read later.

## Memory Rules

Use fixed storage:

```cpp
rpdsp::DelayLine<48000> delay;
rpdsp::DefaultAudioBlock scratch;
```

Avoid hidden allocation:

```cpp
// Avoid this in process:
std::vector<float> temp(block.frames());
```

If a module needs scratch memory, make it a member and size it in the type or in a pre-audio initialization step.

## Parameter Rules

Control changes should be transferred into DSP state between blocks or through simple lock-free publication. Smooth audible parameters:

```cpp
gain_.setTarget(nextGain);
```

Then consume with:

```cpp
const float gain = gain_.next();
```

Do not run UI parsing, MIDI decoding, patch loading, or codec control from the audio callback.

Use `rpdsp::MidiByteParser` on the control side of the application, then publish the resulting `MidiNoteEvent`, `MidiControlEvent`, or mapped parameter values to the DSP graph. Treat the parser as an adapter boundary: USB-MIDI, UART MIDI, BLE MIDI, and test fixtures can all feed bytes into the same parser without making transport part of the audio callback.

For physical controls, calibrate raw ADC values before mapping:

```cpp
rpdsp::SmoothedAdcParameter cutoff;
cutoff.prepare(sampleRate, 8.0f);
cutoff.configure({120, 3980, false},
                 {80.0f, 8000.0f, rpdsp::ParameterCurve::Exponential, 0});
cutoff.setRaw(adcValue);
```

Use `rpdsp::GateDebouncer` for switches, gates, and triggers that arrive from GPIO or ADC thresholding. Publish only debounced edges to the audio graph.

## Dual-Core Rules

RP2350 gives two cores, but the realtime audio owner still needs a clear boundary.

Recommended split:

- Core 0: audio callback, DSP graph, DMA refill, and minimal metering publication.
- Core 1: USB, UI, MIDI, patch management, display, storage, and non-realtime control.

Communication rules:

- Prefer single-producer single-consumer queues or double-buffered parameter structs.
- Keep message sizes small.
- Never let the audio core wait for the control core.
- Treat cross-core data as shared memory that needs explicit ownership or atomic publication.

## Denormal and Tail Rules

Very small floating-point values can cause slow paths on some targets and noisy tails in feedback systems. Use `rpdsp::zapDenormal()` at feedback boundaries and on long-decay filter state:

```cpp
feedbackState_ = rpdsp::zapDenormal(feedbackState_);
```

## Deadline Budget

For 48 kHz stereo:

- 16 frames: 0.333 ms.
- 32 frames: 0.667 ms.
- 64 frames: 1.333 ms.

Keep measured worst-case time comfortably below the full block period. A practical target is to keep the full graph under 50 percent of the callback budget, leaving room for interrupt jitter and platform overhead.

Use `rpdsp::CallbackPerformanceMeter` or the runtime stats fields to track measured block cost:

```cpp
const std::uint64_t startedAtUs = meter.begin();
processor.process(block);
meter.end(startedAtUs);
const auto stats = meter.stats();
```

The useful fields are `lastDurationUs`, `worstDurationUs`, `overrunBlocks`, `cpuLoadPercent`, and `worstCpuLoadPercent`. Copy or inspect those values from non-realtime code. Do not print them from inside the callback.

## Review Checklist

Before a processor is used in firmware:

- `prepare` initializes all sample-rate dependent state.
- `reset` returns the module to a deterministic silent or known state.
- `process` uses `block.frames()`.
- No allocation happens in `process`.
- No blocking calls happen in `process`.
- Feedback paths are bounded.
- Parameters that can click are smoothed.
- Worst-case processing time was measured on target hardware.
- Overrun counters and worst-case callback duration are visible from non-realtime code.
