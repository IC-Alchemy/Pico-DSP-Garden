

# Pico-DSP-Garden

Pico-DSP-Garden is an open-source audio DSP playground designed for the Raspberry Pi Pico2w RP2350 (and RP2040) microcontroller, leveraging dual-core programming, high-quality I2S audio output via the PCM510x DAC, and the rich DaisySP DSP library for synthesis and effects. This project aims to make it easy to prototype, experiment, and develop embedded audio applications in C++.

## Features

- **RP2350/RP2040 Support:** Designed for Raspberry Pi Pico boards, with explicit support for both RP2040 and RP2350.
- **Dual-Core Programming:** Utilizes both cores—one for real-time audio processing, the other for high-level logic or UI.
- **I2S Audio Output:** High-quality stereo audio delivered by interfacing with a PCM510x DAC over I2S, using programmable PIO and DMA for efficient streaming.
- **DaisySP Integration:** Incorporates the DaisySP library for advanced DSP, including synthesis (e.g., oscillators, hypersaw) and effects (chorus, flanger, decimator, etc.).
- **Flexible Architecture:** Modular codebase with example projects, buffer management, and extensible audio pipeline.

## Hardware Requirements

- Raspberry Pi Pico, Pico W, or compatible RP2040/RP2350 board
- PCM510x or similar I2S DAC
- Basic audio output circuit (see examples for pinout)
- (Optional) Buttons, pots, or other controls for interactive projects

## Repository Structure

- `audio/` — Core audio driver code for I2S output, buffer management, DMA, and PIO setup.
- `DaisySP/` — Embedded DaisySP library for DSP algorithms and audio effects.
- `Examples/` — Example sketches showing off synthesis and effects (e.g., SuperSaw, Oscillators).
- `build/` — Build artifacts and preprocessed files (not for editing).

## Getting Started

### 1. Wiring

Connect your Pico board to the PCM510x DAC:

- DATA (e.g., GPIO 15)
- BCLK (e.g., GPIO 17)
- LRCK (e.g., GPIO 16)
- Power and Ground as per your DAC

Pin assignments can be customized in your sketch.

### 2. Build and Flash

1. Clone the repository:
   ```sh
   git clone https://github.com/IC-Alchemy/Pico-DSP-Garden.git
   ```
2. Open the desired example in your IDE (e.g., Arduino IDE or PlatformIO).
3. Adjust the I2S pin configuration if needed:
   ```cpp
   int PICO_AUDIO_I2S_DATA_PIN = 15;
   int PICO_AUDIO_I2S_CLOCK_PIN_BASE = 16; // LRCK = 17
   ```
4. Build and upload to your Pico board.

### 3. Dual-Core Programming

This project demonstrates dual-core usage:

- Core 0: Runs the audio engine and main loop
- Core 1: Handles musical logic (e.g., note progression) or other tasks

Example (from `SuperSaw.ino`):
```cpp
void setup1() {
    Serial.println("[CORE1] Second core started for note progression");
}
void loop1() {
    hypersaw.SetFreq(daisysp::mtof(scale[note_index] + 48));
    note_index = (note_index + 1) % 24;
    delay(1000);
}
```

### 4. DSP and Effects

The DaisySP library is included for advanced DSP:
- Add and configure oscillators, LFOs, envelopes, or effects in your audio callback
- Example:
  ```cpp
  hypersaw.SetDetune(detune_mod);
  hypersaw.SetMix(mix_mod);
  float signal = hypersaw.Process();
  ```

## Example Projects

- **SuperSaw:** Demonstrates a hypersaw oscillator with LFO modulation and dual-core note sequencing.
- **Oscillators:** Shows multiple oscillator types and buffer management.
- More in the `Examples/` folder!

## License

This project is licensed under the BSD-3-Clause License. See [LICENSE](LICENSE) for details.

## Credits

- Core audio driver based on Raspberry Pi Trading Ltd’s reference implementation.
- DSP powered by [DaisySP](https://github.com/electro-smith/DaisySP).
- Example algorithms and musical logic by IC-Alchemy.

---

