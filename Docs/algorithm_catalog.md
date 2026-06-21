# rpdsp Algorithm Catalog

Reference for the DSP modules in `libraries/rpdsp/src/rpdsp/`. Every entry
below is real, header-only, namespace `rpdsp`. Source of truth = the headers.

Conventions:
- Every module is `prepare(float sampleRate)`'d before use. `prepare` is the
  one-time setup call (not `Init`).
- Sources (oscillators) use `setFreq(hz)` / `process()`. Processors use
  `setCutoff`/`setResonance`/etc. and `process(float input)`.
- `process()` returns the value at the current phase *before* advancing.
- Naive oscillators alias on saw/square — use the `SecondOrderBSpline*`
  classes for band-limited output.

Aspirational APIs (block framework, codec bridges) are in
[`roadmap.md`](roadmap.md).

## Core utilities

`config.h`:
- `RPDSP_BLOCK_SIZE` — compile-time block size, `static_assert`'d to 16/32/64
  (default 32).
- `kDefaultSampleRate = 48000.0f`, `kDefaultBlockSize`, `kPi`, `kTwoPi`.

`algorithm.h` — free functions:
- `clamp(x, lo, hi)`, `clamp01(x)`, `lerp(a, b, t)`, `wrap01(x)`.
- `dbToGain(db)`, `gainToDb(gain)` (20 dB decade, log floor).
- `midiNoteToHz(note)` — `440 * 2^((note-69)/12)`.
- `safeSampleRate(sr)` — falls back to 48 k if `sr <= 1`.
- `clampCutoff(cutoff, sr)`, `onePoleSmooth(ms, sr)`.
- `softClip(x)` — `x / (1 + |x|)`.
- `fastTanh(x)` — 3-piece rational approximation.
- `equalPowerPanLeft/Right(pan)` — cos/sin pan law on [-1, 1].

`realtime.h`:
- `zapDenormal(x)` — returns 0 if `|x| < 1e-20`. Use at feedback boundaries.
- `XorShift32` — deterministic PRNG; `nextU32()`, `nextBipolar()`. Not crypto.

## Oscillators

`oscillator.h` — two families:

**Naive (cheap, aliasing — LFOs, tests):**
- `Phasor` — base phase accumulator; `prepare`, `reset(phase)`, `setFreq`,
  `process`, `phase()`.
- `SineOscillator` (no `SetAmp` — multiply the return value), `TriangleOscillator`,
  `SawOscillator`, `PulseOscillator` (`setPWM`), `NoiseOscillator`.

**Band-limited 2nd-order B-spline (impulse → smear → leaky-integrate):**
- `SecondOrderBSplineSawOscillator` — `setLeak` clamped to [0.9, 1.0].
- `SecondOrderBSplinePulseOscillator` — handles up to 3 edge crossings/sample.
- `SecondOrderBSplineHardSyncSawOscillator` — master + slave phases, guarded
  loop for high sync ratios.

`hypersaw.h`:
- `Hypersaw` — 7-voice "Super Saw" (1 center + 6 detuned), x⁴ detune curve,
  pitch-tracked `StateVariableFilter` high-pass, randomized phase on
  `trigger()`. `setFreq`, `setDetune`, `setMix`, `process`.

## Filters

`filter.h`:
- `OnePoleLowpass` — `prepare`, `reset(value)`, `setCutoff`, `process(input)`.
- `DcBlocker` — high-pass at 20 Hz default.
- `BiquadLowpass` — RBJ cookbook, transposed direct form II; `setCutoff`,
  `setQ` (Q clamped [0.1, 20]).
- `StateVariableFilter` — TPT SVF; `process` returns
  `StateVariableOutput{lowpass, bandpass, highpass}`; resonance clamped
  [0, 0.98]; `setCutoffResonance` combined setter.

`ladder.h`:
- `LadderFilter` — Huovilainen 4-pole, 4× oversampled. `Mode` enum:
  `LP24, LP12, BP24, BP12, HP24, HP12`. Setters: `setFreq`, `setRes`
  (0..1 → K=0..4), `setPassbandGain`, `setInputDrive`, `setMode`. Block
  overload `process(float*, size_t)`. Ported from Teensy Audio (van Hoesel).

## Effects

`effects.h`:
- `Waveshaper` — `setDrive`, `setOutputGain`; tanh normalized by `tanh(drive)`.
- `Delay<Capacity>` — cubic-interpolated read; feedback clamped [-0.99, 0.99].
- `Chorus<Capacity>` — `SineOscillator` LFO sweeps fractional delay.
- `CombFilter<Capacity>`, `AllpassFilter<Capacity>` — Schroeder primitives.
- `SchroederReverb` — 4 parallel combs + 2 serial allpasses + `OnePoleLowpass`
  damping at 6 kHz; `setRoomSize`.
- `StereoSchroederReverb` — two mono tanks, crossfeed, 257-sample right
  predelay, mid/side width.

`delay_line.h`:
- `DelayLine<Capacity>` — ring buffer; `read`, `readLinear`, `readCubic`.

## Dynamics

`dynamics.h` — decomposed for testability:
- `EnvelopeFollower`, `CompressorStaticCurve` (hard or quadratic knee),
  `CompressorDetector`, `GainReductionSmoother` (separate attack/release on
  the gain domain), `Compressor` (threshold dB, ratio, knee width, attack,
  release, makeup).

## Envelopes

`envelope.h`:
- `ADSR` — integer-sample stage counters; `noteOn`, `noteOff`, `process`.

## Voice & sequencing

`voice.h`:
- `TriggeredSynthVoice<MaxOscillators>` — subtractive: oscillators + noise →
  SVF (velocity-to-cutoff) → ADSR. `noteOn(VoiceTrigger)`, `noteOnHz`,
  `noteOff` with note/channel wildcard matching, `applyPreset`.
- `TriggeredSynthVoicePreset<MaxOscillators>`, `classicThreeSawSubtractivePreset()`,
  `noisePluckPreset()`.
- `VoiceTrigger` and settings structs.

`gate_pattern.h`:
- `GatePattern<MaxSteps=32>` — fixed-size step-mask gate sequencer.

`rhythm_sequencer.h`:
- `RhythmGateSequencer<MaxSteps=32>` — reads interleaved external pattern
  tables (caller owns storage).

`clock_tracker.h`:
- `ClockTracker` — PPQN transport (default 96 PPQN, two-bar 4/4).
  `processExternalClock(bool clockHigh)` (rising edge → true),
  `advanceTick()`, `currentStepPosition()` (nearest-step rounding),
  `isStale(timeoutSeconds=2)`, `clockOutPulse(divider=4)`.
- `StepPosition{step, offsetTicks}`.

`knob_bank.h`:
- `KnobBank<NumBanks, NumKnobs>` — multi-bank parameter storage with pickup;
  `process(array&)`.

`pickup_knob.h`:
- `PickupKnob` — software takeover, no jumps on bank switch.

`joystick_recorder.h`:
- `JoystickRecorder<MaxPositions>` — fixed-buffer X/Y motion record+loop.

## Parameter smoothing

`parameter_smoother.h`:
- `LinearSmoother` — fixed-ramp target-to-current linear interpolation;
  `prepare(sampleRate, ms)`, `setTarget`, `next`.

## Physical modeling & analysis

`waveguide.h`:
- `KarplusStrongVoice<Capacity>` — plucked string; `prepare`, `reset`,
  `setDecay(0..0.9999)`, `pluck(freqHz, amp)`, `process`, `isActive`.

`analysis.h`:
- `ZeroCrossingPitchDetector` — rising-edge-only, smoothed period estimate.
- `YinPitchDetector<WindowSize, MaxTau>` — full YIN (difference, cumulative-
  mean normalized difference, absolute-threshold walk-down, parabolic
  interpolation); `setFreqRange`, `setThreshold`, `setUpdateIntervalSamples`,
  `process`, `hasPitch`, `frequencyHz`, `confidence`.
- `RmsPeakMeter` — running RMS + peak; copy results out, no logging in callback.

## Hardware

`hardware_interpolator.h` — RP2xxx interpolator peripheral (host-dummy shim
when `<hardware/interp.h>` absent):
- `HardwareInterpolatorPool` — static bitmask allocator for the 4 interp lanes.
- `HardwareWavefolder` — hardware triangular wave-folding (RAII, non-copyable).
- `HardwareOscillator` — phase-accumulator wavetable oscillator using interp0
  blend mode for hardware linear interpolation.
