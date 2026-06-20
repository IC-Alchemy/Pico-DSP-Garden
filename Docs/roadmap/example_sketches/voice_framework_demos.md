# voice_framework_demos

Host-rendered demos of the `TriggeredSynthVoice` framework across three patches.

- **Source**: [`voice_framework_demos.cpp`](voice_framework_demos.cpp)
- **Build target**: `rpdsp_example_voice_framework_demos`
- **Rendered outputs** (all 8.0 s, 48 kHz, 16-bit stereo):
  - `voice_demo_classic_three_saw.wav`
  - `voice_demo_noise_percussion.wav`
  - `voice_demo_midi_trigger_chords.wav`

`main()` renders all three in sequence and returns non-zero if any render fails,
so the single smoke test covers all of them.

## What it demonstrates

Three directly comparable uses of the same `TriggeredSynthVoice` preset API,
sharing a small `PannedVoice` stereo wrapper (mono voice + equal-power pan).

### 1. Classic three-saw (`ClassicThreeSawDemo`)

Four panned three-saw voices in a gated 16-step chord pattern, with per-step
filter cutoff changes driven by accent velocity. Uses
`classicThreeSawSubtractivePreset()` with a small noise layer added.

### 2. Noise percussion (`NoisePercussionDemo`)

Three single-oscillator `noisePluckPreset()` voices tuned as low / mid / high
percussion, each with its own short ADSR and filter. Triggers are scheduled with
sample counters and released via countdown gates (no timers, no allocation).

### 3. MIDI-style chords (`MidiStyleChordDemo`)

Four three-saw voices triggered with explicit MIDI-style
`noteOn(note, velocity, channel)` calls and released with a velocity-zero
re-trigger, mirroring how a MIDI layer would drive the voices.

## Modules used

- `rpdsp::TriggeredSynthVoice<3>` and `rpdsp::TriggeredSynthVoice<1>`.
- `rpdsp::classicThreeSawSubtractivePreset()` and `rpdsp::noisePluckPreset()`.
- `rpdsp::VoiceEnvelopeSettings`, `rpdsp::VoiceFilterSettings`.
- `rpdsp::equalPowerPanLeft/Right`, `rpdsp::StereoSample`, `rpdsp::softClip`,
  `rpdsp::lerp`.

## Build and run

```powershell
cmake --build build-host --target rpdsp_example_voice_framework_demos
ctest --test-dir build-host -R rpdsp_example_voice_framework_demos --output-on-failure
```

## Notes

- Each voice gets a distinct noise seed so multiple instances don't share
  correlated noise.
- This is the reference for the trigger surface (note / velocity / channel,
  plus `noteOnHz` for fractional pitches) that a MIDI layer should target.
