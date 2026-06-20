# karplus_string

Host-rendered strummed Karplus-Strong harp with stereo delays and room reverb.

- **Source**: [`karplus_string.cpp`](karplus_string.cpp)
- **Build target**: `rpdsp_example_karplus_string`
- **Rendered output**: `karplus_strummed_harp.wav` (10.0 s, 48 kHz, 16-bit stereo)

## What it demonstrates

A physical-modeling string patch using `KarplusStrongVoice`. Six panned string
voices are plucked in a rotating arpeggio over four chord roots, each with its
own decay time so the strings ring out at slightly different rates. The stereo
string mix passes through ping-pong delays and a shared mono Schroeder reverb
tail.

A simpler single-voice `KarplusString` (and its `processAudioBlock` callback) is
also provided as a minimal callback example.

## Modules used

- `rpdsp::KarplusStrongVoice<2400>` — 20 Hz minimum pitch at 48 kHz
  (`kMaxStringDelay`).
- `rpdsp::Delay<12000>` — stereo delays (141 ms / 213 ms).
- `rpdsp::SchroederReverb` — mono room tone blended into both channels.
- `rpdsp::equalPowerPanLeft/Right`, `rpdsp::softClip`, `rpdsp::midiNoteToHz`.

## Build and run

```powershell
cmake --build build-host --target rpdsp_example_karplus_string
ctest --test-dir build-host -R rpdsp_example_karplus_string --output-on-failure
```

## Notes

- `setDecay()` is clamped to `[0.8, 0.9995]`; values near 0.999 ring for a long
  time and are good for harp/string timbres.
- Pick the delay-line capacity so `sampleRate / capacity` is at or below your
  lowest playable pitch. Here `2400` allows down to 20 Hz at 48 kHz.
