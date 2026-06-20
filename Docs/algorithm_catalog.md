# Algorithm Catalog

This catalog describes DSP modules that fit the RP2350 Music DSP API style: small C++17 classes with explicit `prepare`, `reset`, and `process` methods. The approved baseline is 48 kHz stereo, 32-sample blocks, float processing, and a platform boundary that delegates codec transport to Arduino-Pico I2S when firmware is built.

## Core Block Utilities

### Stereo Audio Block

- Role: fixed-capacity stereo sample container for callback processing.
- Public type: `rpdsp::AudioBlock<Capacity>` and `rpdsp::DefaultAudioBlock`.
- Realtime cost: one contiguous left array and one contiguous right array.
- Notes: use `setFrameCount()` when the platform supplies a partial block; otherwise the default block size is 32 frames.

### Parameter Smoothing

- Role: click-free ramps for gain, cutoff, mix, or modulation depth.
- Public type: `rpdsp::LinearSmoother`.
- Realtime cost: one addition and branch per sample.
- Notes: call `setTarget()` from the control side or between blocks, then call `next()` per sample in the processor.

### Delay Line

- Role: fixed-capacity circular buffer for echo, comb filters, flangers, choruses, and string synthesis.
- Public type: `rpdsp::DelayLine<Capacity>`.
- Realtime cost: one bounded circular-buffer write and one indexed read; `readLinear()` and `readCubic()` add fractional interpolation.
- Notes: capacity is compile-time storage. Pick capacities deliberately to avoid hidden heap use.

### Shared Math and Realtime Helpers

- Role: reusable scalar utilities for module implementations and user processors.
- Public headers: `algorithm.h` and `realtime.h`.
- Public functions/types: `clamp`, `clamp01`, `lerp`, `wrap01`, `dbToGain`, `gainToDb`, `midiNoteToHz`, `clampCutoff`, `onePoleCoefficient`, `softClip`, equal-power pan helpers, `zapDenormal`, and `XorShift32`.
- Notes: these helpers keep sample-rate guards, decibel math, cutoff limits, denormal cleanup, and deterministic noise behavior consistent across modules.
- Example: `examples/oscillator_gallery.cpp` uses MIDI pitch, panning, and soft clipping; `examples/modulated_space.cpp` uses `zapDenormal()`.

## Synthesis Algorithms

### Subtractive Voice

- Inputs: MIDI-style note, velocity, channel, gate, cutoff, resonance approximation.
- Public types: `rpdsp::TriggeredSynthVoice<MaxOscillators>`, `TriggeredSynthVoicePreset<MaxOscillators>`, `classicThreeSawSubtractivePreset()`, and `noisePluckPreset()`.
- Building blocks: fixed saw oscillator bank, noise source, ADSR envelope, state-variable low-pass filter, output gain.
- RP2350 fit: excellent for polyphony when each voice avoids dynamic allocation.
- Examples: `examples/subtractive_synth.cpp` and `examples/voice_framework_demos.cpp`.

Recommended first implementation:

- Three saw oscillators through a low-pass filter.
- Fast attack, short release, and sustain near `0.5f`.
- Trigger API that can be called directly from a MIDI note-on/note-off layer.
- Stereo output by duplicating voice output to left/right or applying static pan outside the voice.

### Basic Oscillators

- Inputs: frequency, phase reset, pulse width where applicable.
- Public types: `rpdsp::Phasor`, `SineOscillator`, `TriangleOscillator`, `SawOscillator`, `PulseOscillator`, and `NoiseOscillator`.
- RP2350 fit: strong for low to moderate voice counts; the naive saw and pulse variants are simple and cheap but not band-limited.
- Notes: use the simple oscillators for LFOs, low-frequency sources, tests, and intentionally bright sounds where aliasing has been accepted.
- Example: `examples/oscillator_gallery.cpp`.

### Second-Order B-Spline Oscillators

- Inputs: frequency, optional pulse width, hard-sync master/slave frequencies.
- Public types: `SecondOrderBSplineSawOscillator`, `SecondOrderBSplinePulseOscillator`, and `SecondOrderBSplineHardSyncSawOscillator`.
- RP2350 fit: useful for audio-rate saw, pulse, and hard-sync voices when the naive discontinuity is too harsh.
- Realtime cost: fixed storage and bounded per-sample work; the hard-sync oscillator has a small guard-bounded event loop.
- Notes: increments are clamped below Nyquist. `SecondOrderBSplineSawOscillator::setLeak()` can trim the leaky integrator behavior between `0.9f` and `1.0f`.
- Example: `examples/subtractive_synth.cpp` for saw/pulse voices and `examples/oscillator_gallery.cpp` for hard sync.

### Karplus-Strong String

- Inputs: trigger, pitch, decay, damping, excitation noise.
- Building blocks: delay line, low-pass averaging, feedback gain, deterministic noise.
- RP2350 fit: strong; memory cost is predictable and proportional to maximum delay.
- Example: `examples/karplus_string.cpp`.

Recommended first implementation:

- Fill delay line with short noise burst on trigger.
- Read one period behind the write head.
- Average current and previous delayed samples.
- Feed back with decay below `1.0f`.

### Drum and Exciter Sources

- Inputs: trigger, decay, tone, level.
- Building blocks: noise, envelopes, one-pole filters, short resonators.
- RP2350 fit: strong for fixed voice counts.
- Notes: denormal cleanup matters on long decays; use `rpdsp::zapDenormal()`.

## Effect Algorithms

### Gain, Pan, and Utility

- Inputs: gain, pan, mute, phase.
- Building blocks: `AudioBlock::applyGain()`, manual per-channel transforms, smoothers.
- RP2350 fit: trivial cost; good first integration test for an I2S callback.
- Example: `examples/pass_through.cpp`.

### Delay and Echo

- Inputs: time, feedback, wet/dry mix.
- Building blocks: `DelayLine`, cubic fractional reads, smoothed parameters, feedback limiter.
- RP2350 fit: strong if maximum delay memory is reserved statically.
- Example: `examples/effects_rack.cpp`.

### Chorus and Flanger

- Inputs: base delay, modulation depth, rate, feedback, mix.
- Building blocks: fractional delay with `readCubic()`, low-frequency oscillator.
- RP2350 fit: good for a few stereo instances; modulation math should be kept simple.
- Example: `examples/effects_rack.cpp` and `examples/modulated_space.cpp`.

### DC Blocking and Rumble Cleanup

- Inputs: audio sample stream and a low cutoff frequency.
- Public type: `rpdsp::DcBlocker`.
- RP2350 fit: very cheap one-pole high-pass behavior for codec offsets, biased modulation, or saturation stages.
- Notes: use after nonlinear processors or at codec input boundaries when a graph can build up DC. It keeps one previous input and one previous output sample, with no buffers or allocation.
- Example: `examples/effects_rack.cpp`.

### Saturation and Waveshaping

- Inputs: drive, trim, bias, mix.
- Building blocks: polynomial or rational transfer functions.
- RP2350 fit: strong; avoid expensive transcendental functions in large graphs unless profiled.
- Example: `examples/effects_rack.cpp`.

### Stereo Schroeder Reverb

- Inputs: left/right samples, room size, wet/dry mix, stereo width, and small right-channel predelay.
- Public type: `rpdsp::StereoSchroederReverb`.
- RP2350 fit: fixed storage and bounded work; suitable for small send effects when convolution or large FDN reverbs are too heavy.
- Notes: the wrapper owns two mono Schroeder tanks, a short predelay, and wet-side mid/side width control. Keep it as an effect send or late-stage insert rather than duplicating it per voice.
- Example: `examples/effects_rack.cpp`.

### Compressor

- Inputs: threshold in dB, ratio, knee width in dB, attack/release, makeup gain.
- Public types: `EnvelopeFollower`, `CompressorStaticCurve`, `CompressorDetector`, `GainReductionSmoother`, and `Compressor`.
- RP2350 fit: appropriate when dynamics behavior is staged and tested separately.
- Notes: the compressor is intentionally split into detector, static curve, gain-reduction smoother, and final gain application. This makes knee behavior, attack/release response, and makeup gain testable without treating the compressor as one opaque block.
- Example: `examples/effects_rack.cpp` for the full compressor and `examples/analysis_tuner.cpp` for the split stages.

## Analysis and Metering

### Zero-Crossing Pitch

- Inputs: one sample at a time.
- Public type: `rpdsp::ZeroCrossingPitchDetector`.
- Outputs: last smoothed frequency estimate.
- RP2350 fit: cheap and deterministic, but only reliable for clean monophonic signals.
- Example: `examples/analysis_tuner.cpp`.

### YIN Pitch Tracking

- Inputs: one sample at a time with fixed template storage.
- Public type: `rpdsp::YinPitchDetector<WindowSize, MaxTau>`.
- Outputs: `hasPitch()`, `frequencyHz()`, and `confidence()`.
- RP2350 fit: useful for monophonic pitch tracking when the window and maximum tau are bounded deliberately.
- Notes: call `setFreqRange(...)`, `setThreshold(...)`, and `setUpdateIntervalSamples(...)` to balance low-frequency reach, CPU cost, and update rate. Analysis runs only after the fixed window is full.
- Example: `examples/analysis_tuner.cpp`.

### RMS and Peak Meter

- Inputs: one sample at a time.
- Public type: `rpdsp::RmsPeakMeter`.
- Outputs: running RMS and peak values since the last reset.
- RP2350 fit: safe if results are copied out without logging from the audio callback.
- Example: `examples/pass_through.cpp` and `examples/analysis_tuner.cpp`.

### Routing

- Inputs: fixed channel count, per-channel stereo samples, gain, pan, and enabled state.
- Public types: `rpdsp::StereoSample` and `rpdsp::StereoMixer<Channels>`.
- RP2350 fit: good for small fixed mixers where channel count is known at compile time.
- Notes: the mixer does no allocation and ignores out-of-range channel writes.
- Example: `examples/oscillator_gallery.cpp`.

## Platform Boundary

### I2S Sample Bridge

- Inputs: interleaved signed 16-bit, Arduino-Pico left-aligned 24-bit, or signed 32-bit stereo samples.
- Public type: `rpdsp::I2sSampleBridge`.
- Outputs: float `AudioBlock` data for DSP processing and clipped interleaved integer samples for I2S transport.
- RP2350 fit: useful at the codec edge because conversion is explicit and the DSP graph stays float-based.
- Example: `examples/codec_bridge_loopback.cpp`.

### Benchmark Harness

- Inputs: processor under test and repeated audio blocks.
- Outputs: elapsed time, cycles, or overrun margin.
- RP2350 fit: necessary before adding complex modulation, filters, or polyphony.
- Example: `examples/benchmark.cpp`.

## Selection Criteria

Prefer algorithms that have:

- Fixed memory known at compile time.
- No allocation during `process`.
- Stable behavior at 48 kHz and 32-frame blocks.
- Clear reset state.
- Bounded control-rate work.
- A measurable worst-case processing budget.

Avoid algorithms that require:

- Heap allocation in the audio path.
- File I/O, logging, USB prints, or blocking locks in `process`.
- Codec register changes from the audio callback.
- Unbounded loops driven by musical input.
- Large lookup tables unless they are static and explicitly budgeted.
