# Pico-DSP-Garden

**An embedded audio playground for the RP2350.**

Wire a DAC. Write a callback. Make sound.

Pico-DSP-Garden is an open-source C++ framework for building audio DSP projects
on Raspberry Pi Pico 2 (RP2350) and Pico (RP2040). Dual-core architecture, I2S
output via PCM510x DAC, and the self-authored **rpdsp** DSP library — ready to
fork, extend, and build on.

## What's inside

- **Dual-core audio engine** — Core 0 runs the real-time audio callback; Core 1
  handles sequencing, UI, or whatever else needs to run without blocking audio
- **I2S output via PCM510x DAC** — Stereo, high-quality, DMA-driven. PIO
  handles the timing so your callback stays clean
- **rpdsp library** — Header-only DSP modules: band-limited oscillators,
  state-variable filters, dynamics, Schroeder reverb, envelopes, and
  sequencing/UI helpers. No external dependencies
- **pico_audio_i2s library** — PIO/DMA I2S driver vendored from
  `pico-extras` (BSD-3-Clause)

## Hardware

- Raspberry Pi Pico 2 (RP2350)
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
libraries/
  rpdsp/                  Header-only DSP library (MIT, self-authored)
    library.properties
    src/rpdsp/            algorithm.h  config.h  delay_line.h  dynamics.h
                          effects.h  envelope.h  filter.h  gate_pattern.h
                          hypersaw.h  joystick_recorder.h  knob_bank.h
                          oscillator.h  parameter_smoother.h  pickup_knob.h
                          realtime.h  rhythm_sequencer.h  voice.h
  pico_audio_i2s/         I2S driver (BSD-3-Clause, vendored from pico-extras)
    library.properties
    src/pico_audio_i2s/   audio.h  audio.cpp  audio_i2s.h  audio_i2s.c
                          audio_i2s.pio.h  buffer.h  buffer.c  sample_conversion.h
    extras/               audio_i2s.pio   (PIO source archive; not compiled)
Examples/
  SuperSaw/               Hypersaw arpeggiator (48 kHz)
  Oscillators/            12 sine oscillators with LFO amplitude modulation (44.1 kHz)
  SimpleOscillators/      Hard-sync B-spline saw demo (44.1 kHz)
  TwoChannelOscillator/   Dual-channel sine with ADC slider control (44.1 kHz)
Docs/                     realtime_rules.md  rp2350_tuning.md  algorithm_catalog.md
build/                    Build artifacts (generated, not for editing)
```

## Getting started

### 1 — Clone

```sh
git clone https://github.com/IC-Alchemy/Pico-DSP-Garden.git
```

### 2 — Install the libraries

**Arduino IDE 2.x**: copy (or symlink) both library folders into your Arduino
sketchbook `libraries/` directory:

```
<sketchbook>/libraries/rpdsp/
<sketchbook>/libraries/pico_audio_i2s/
```

**arduino-cli**: pass `--library` flags pointing at the repo folders when
compiling — no copy needed:

```sh
arduino-cli compile \
  --fqbn "rp2040:rp2040:rpipico2:flash=4194304_0,freq=200,opt=Optimize3,usbstack=picosdk" \
  --library libraries/rpdsp \
  --library libraries/pico_audio_i2s \
  Examples/SuperSaw
```

### 3 — Open and flash an example

Open any sketch from `Examples/` in Arduino IDE (board: **Raspberry Pi Pico 2**,
core: **earle-philhower arduino-pico**). Adjust the I2S pin constants if your
wiring differs:

```cpp
static const int PICO_AUDIO_I2S_DATA_PIN       = 15;
static const int PICO_AUDIO_I2S_CLOCK_PIN_BASE = 16; // LRCK = 16, BCLK = 17
```

### Dual-core pattern

Core 0 owns the real-time audio path; Core 1 is yours for sequencing and I/O:

```cpp
// Core 0 — audio callback (never blocks)
void loop() {
    audio_buffer_t *buf = take_audio_buffer(producer_pool, true);
    if (buf) {
        fill_audio_buffer(buf);
        give_audio_buffer(producer_pool, buf);
    }
}

// Core 1 — non-real-time work
void loop1() {
    hypersaw.setFreq(rpdsp::midiNoteToHz(scale[note_index] + 48));
    note_index = (note_index + 1) % SCALE_LENGTH;
    delay(122);
}
```

## Example projects

- **SuperSaw** — Seven-voice Hypersaw (Roland JP-8000-style) arpeggiating a
  32-step sequence
- **Oscillators** — 12 sine oscillators with LFO amplitude modulation
- **SimpleOscillators** — Band-limited hard-sync B-spline saw with slow
  timbre sweep
- **TwoChannelOscillator** — Stereo sine oscillator tuned via four ADC sliders
