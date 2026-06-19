# Pico-DSP-Garden

**An embedded audio playground for the RP2350.**

Wire a DAC. Write a callback. Make sound.

Pico-DSP-Garden is an open-source C++ framework for building audio DSP projects on Raspberry Pi Pico2 (RP2350) and Pico (RP2040). Dual-core architecture, I2S output via PCM510x DAC, and the full DaisySP library — ready to fork, extend, and build on.

## What's inside

- **Dual-core audio engine** — Core 0 runs the real-time audio callback; Core 1 handles sequencing, UI, or whatever else needs to run without blocking audio
- **I2S output via PCM510x DAC** — Stereo, high-quality, DMA-driven. PIO handles the timing so your callback stays clean
- **DaisySP integration** — Full library included: HyperSaw, oscillators, chorus, flanger, decimator, LFOs, envelopes. Build complex signal chains without reinventing the DSP
- **Modular codebase** — Audio drivers, buffer management, and example projects are separate. Pull in what you need

## Hardware

- Raspberry Pi Pico or Pico W (RP2040 or RP2350)
- PCM510x or compatible I2S DAC
- Basic audio output circuit

Default wiring (adjustable per sketch):

| Signal | GPIO |
|--------|------|
| DATA   | 15   |
| LRCK   | 16   |
| BCLK   | 17   |

## Repository layout

```
audio/      Core I2S driver — DMA, PIO, buffer management
dsp/        Self-authored rpdsp DSP library — oscillators, filters, effects (header-only)
Examples/   Working sketches: SuperSaw, multi-oscillator, and more
build/      Build artifacts (generated, not for editing)
```

## Getting started

Clone and open an example:

```sh
git clone https://github.com/IC-Alchemy/Pico-DSP-Garden.git
```

Open any sketch from `Examples/` in Arduino IDE or PlatformIO. Adjust the I2S pin constants if your wiring differs:

```cpp
int PICO_AUDIO_I2S_DATA_PIN = 15;
int PICO_AUDIO_I2S_CLOCK_PIN_BASE = 16; // LRCK = 16, BCLK = 17
```

Build and flash to your Pico.

### Dual-core pattern

Core 0 runs the audio engine. Core 1 is yours:

```cpp
void setup1() {
    // Second core: note progression, sensor reads, UI — nothing that blocks audio
}
void loop1() {
    hypersaw.SetFreq(daisysp::mtof(scale[note_index] + 48));
    note_index = (note_index + 1) % 24;
    delay(1000);
}
```

### DaisySP in the callback

```cpp
hypersaw.SetDetune(detune_mod);
hypersaw.SetMix(mix_mod);
float signal = hypersaw.Process();
```

Add oscillators, LFOs, filters, and effects directly in the audio callback. DaisySP handles the math.

## Example projects

- **SuperSaw** — HyperSaw oscillator with LFO modulation and dual-core note sequencing
- **Oscillators** — Multiple waveform types with buffer management walkthrough
- More in `Examples/`

## License

BSD-3-Clause. See [LICENSE](LICENSE).

## Built on the shoulders of

- Audio driver based on Raspberry Pi Trading Ltd's reference I2S implementation
- DSP by [DaisySP](https://github.com/electro-smith/DaisySP) (Electrosmith)
- Timing by [µClock](https://github.com/midilab/uClock) (Midilab)
- Synthesis algorithms from [Mutable Instruments](https://github.com/pichenettes/eurorack) (Émilie Gillet)

---

*Part of the IC Alchemy open firmware ecosystem. Pico-DSP-Garden powers the synthesis engine in [Pico2Seq](https://github.com/IC-Alchemy/Pico2Seq).*
