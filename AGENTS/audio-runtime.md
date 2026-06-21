# Real-Time Audio, Dual-Core & Wiring

Everything about getting sound out: the callback contract, the core split, the
output format, and the I2S wiring.

## Dual-core split — examples vs. docs disagree

**The examples and the design docs use opposite core assignments. Do not
"correct" one to match the other without asking.**

- **In the example sketches:** Core 0 (`setup`/`loop`) runs the real-time audio
  fill callback; Core 1 (`setup1`/`loop1`) runs sequencing, ADC reads, serial
  debug. This is what actually compiles and runs today.
- **In `Docs/realtime_rules.md` and `Docs/rp2350_tuning.md`:** the recommended
  split is Core 0 = control/UI/MIDI, Core 1 = audio callback.

When touching dual-core code, **follow the existing example's assignment** unless
the task is explicitly to realign the codebase to the documented convention.

## Real-time callback contract

The audio fill callback (`fill_audio_buffer`) is a hard real-time section.
**Full rules in `Docs/realtime_rules.md` — read it before touching the callback.**

Summary of what is forbidden inside the callback:

- No allocation — no `new`/`malloc`/`delete`/`free`, no STL containers that may
  allocate.
- No `Serial`/USB/logging, no file I/O, no string formatting.
- No blocking (locks, semaphores, queues, USB, I2C, SPI, codec transactions).
- No unbounded loops, no codec-register writes, no `sleep`.

Transfer parameter changes from the control side via plain `volatile` cross-core
state (the examples' convention) or lock-free publication. Smooth click-prone
parameters.

### Deadline budget

- At 48 kHz with 32-sample blocks: one block ≈ **0.667 ms**.
- Target **< 50%** of that budget for the DSP graph itself.
- The examples use **256-sample buffers / 3 buffers** in the producer pool.

## Output format

- Output is **interleaved stereo int16** (`sample_stride = 4`: 2 channels × 2 bytes).
- Write `out[2*i]` = left, `out[2*i+1]` = right.

## I2S wiring convention

The `.ino` files reference "see AGENTS.md wiring convention." That resolves here.
Default:

| Signal | GPIO | constant |
|--------|------|----------|
| DATA   | 15   | `PICO_AUDIO_I2S_DATA_PIN` |
| LRCK   | 16   | `PICO_AUDIO_I2S_CLOCK_PIN_BASE` (= base) |
| BCLK   | 17   | base + 1 (implicit, set by the driver) |

- `PICO_AUDIO_I2S_CLOCK_PIN_BASE` is the **LRCK** pin; BCLK is always `base + 1`
  (`audio_i2s.c` assigns both `clock_pin_base` and `clock_pin_base + 1`).
- The two clock pins **must be adjacent**.
- Hardware target is a **PCM510x-class** I2S DAC.
