# Realtime Rules

The audio callback is a hard realtime section. At 48 kHz, the processor has roughly 0.667 ms per 32-frame block (or ~5.33 ms for the 256-frame buffers some examples use) to finish each stereo block before the next DMA half-buffer needs service. The safe design is boring, bounded, and measurable.

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

Use fixed storage (member, sized in the type):

```cpp
rpdsp::DelayLine<48000> delay_;
```

Avoid hidden allocation:

```cpp
// Avoid this in process:
std::vector<float> temp(numFrames);
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

Keep MIDI parsing off the audio core. If/when a `MidiByteParser` exists (see
[`roadmap.md`](roadmap.md)), it belongs on the control side; only the resulting
mapped parameter values cross into the DSP graph via `volatile` globals or a
lock-free queue. Today the examples do MIDI/sequencing directly in Core 1's
`loop1()` and publish note events as simple `volatile` flags.

For physical controls, read and smooth ADC values on the control side, then
publish a target the audio core ramps toward per-sample:

```cpp
// Control side (Core 1): read ADC, smooth, publish
volatile float cutoff_target;
void loop1() {
  float raw = analogRead(A0) / 4095.0f;
  smoothed = smoothed * 0.9f + raw * 0.1f;   // EMA
  cutoff_target = mapToHz(smoothed);
}

// Audio side (Core 0): ramp to target per-sample
rpdsp::LinearSmoother cutoff_smoother;
void fill_audio_buffer(audio_buffer_t* buffer) {
  cutoff_smoother.setTarget(cutoff_target);   // snapshot once per buffer
  for (int i = 0; i < N; ++i) {
    float c = cutoff_smoother.next();         // ramp per sample
    // ...
  }
}
```

For switches, gates, and triggers from GPIO, debounce and edge-detect on the
control side and publish only edges to the audio graph. A `GateDebouncer`
helper is on the [roadmap](roadmap.md); today the examples do simple edge
detection in `loop1()`.

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

Track measured block cost. A `CallbackPerformanceMeter` helper is on the
[roadmap](roadmap.md); today, instrument the callback by toggling a GPIO and
measuring with a logic analyzer, or by writing a cycle count to a `volatile`
for non-realtime inspection. Do not print timing data from inside the callback.

## Review Checklist

Before a processor is used in firmware:

- `prepare` initializes all sample-rate dependent state.
- `reset` returns the module to a deterministic silent or known state.
- `process` uses the host buffer frame count (`buffer->max_sample_count` in the `pico_audio_i2s` examples).
- No allocation happens in `process`.
- No blocking calls happen in `process`.
- Feedback paths are bounded.
- Parameters that can click are smoothed.
- Worst-case processing time was measured on target hardware.
- Overrun counters and worst-case callback duration are visible from non-realtime code.
