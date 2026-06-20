# subtractive_synth

Host-rendered four-voice panned subtractive synth sequence.

- **Source**: [`subtractive_synth.cpp`](subtractive_synth.cpp)
- **Build target**: `rpdsp_example_subtractive_synth`
- **Rendered output**: `subtractive_synth_sequence.wav` (10.0 s, 48 kHz, 16-bit stereo)

## What it demonstrates

A classic subtractive patch built from the preset API: four panned
`TriggeredSynthVoice<3>` instances driven by a 16-step chord pattern. Each voice
runs three detuned saw oscillators, a noise layer, a low-pass filter, and an ADSR
amp envelope. The sequence triggers root, fifth, octave, and an accented upper
note, with a gate point that releases all voices mid-step.

## Modules used

- `rpdsp::TriggeredSynthVoice<3>` with `rpdsp::classicThreeSawSubtractivePreset()`.
- `rpdsp::equalPowerPanLeft/Right` for per-voice stereo placement.
- `rpdsp::softClip` on the summed bus, `rpdsp::StereoSample` mixing.
- `rpdsp::midiNoteToHz`, `rpdsp::clamp`.

## Build and run

```powershell
cmake --build build-host --target rpdsp_example_subtractive_synth
ctest --test-dir build-host -R rpdsp_example_subtractive_synth --output-on-failure
```

## Notes

- The sequencer timing is sample-accurate (`stepSamples_` / `gateSamples_`),
  not timer-based, so it stays deterministic across host and firmware builds.
- `noteOn(float hz)` is a convenience that converts Hz to the nearest MIDI note;
  `TriggeredSynthVoice` itself is note-oriented. For fractional-pitch triggers,
  call `voice.noteOnHz(...)` directly (see `voice_framework_demos`).
