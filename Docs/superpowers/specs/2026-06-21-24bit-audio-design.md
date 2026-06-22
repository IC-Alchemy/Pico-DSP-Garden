# Design: Migrate output path to 24-bit audio (24-in-32, left-justified)

**Date:** 2026-06-21
**Status:** Spec (pending user review before implementation plan)
**Scope:** Library-level change across `libraries/pico_audio_i2s`, `libraries/rpdsp`, all 6 example sketches, and AGENTS docs.

---

## 1. Goal and non-goals

**Goal:** Change the entire project's I2S output from interleaved stereo int16 to
24-bit audio transmitted as **24-in-32 left-justified** (32 BCLK ticks per
channel slot, 24 audio bits in the top of a 32-bit word, low 8 bits zero).
The PCM510x-class DAC target accepts this format natively.

**Non-goals:**

- Changing the DSP graph's internal numeric format. rpdsp stays float32
  end-to-end; bit depth only matters at the final packing step.
- Changing the sample rate (stays 48000 Hz).
- Changing the dual-core split (Core 0 still owns the audio callback).
- Removing int16 support from the driver. S16 and S8 paths remain intact and
  buildable; this change *adds* an S32 path and *migrates the examples* to it.
- Verifying audio output off-target. Per `AGENTS/build.md`, the PIO/DMA/DAC
  path cannot be reasoned about off-target; final verification remains
  "flash and listen." Host tests cover only the packing math.

## 2. Why 24-in-32, left-justified

Selected over packed-24 (3 bytes/sample) and full 32-bit (32 audio bits):

- **Wire format is just 32-bit I2S** (64 BCLK per stereo frame). The only change
  to the existing PIO is the per-half-frame shift count (`set x, 14` →
  `set x, 30`). Autopull threshold stays 32. DMA stays 32-bit-wide.
- **Packing is one shift** in the fill loop. Cheap, branch-free.
- **1.5× DMA bandwidth** of packed-24 (audio data only), but trivial at 48 kHz
  stereo: 384 KB/s on a bus that handles far more.
- **PCM510x accepts it natively** in its default I2S mode.

## 3. CPU overhead (already discussed, recorded here for the record)

- **DSP graph CPU: unchanged.** rpdsp is float; the bit depth only affects the
  packing step in `fill_audio_buffer`.
- **Per-sample packing cost:** one extra `int32` shift vs the int16 path. On
  RP2350 @ 200 MHz with FPU, this is < 1% of a 32-frame block — well under the
  0.333 ms budget in `Docs/rp2350_tuning.md`.
- **DMA bandwidth:** 8 B per stereo frame vs 4 B for int16. Still negligible.
- **PIO bit-clock:** 64 ticks/frame vs 32 ticks/frame → BCLK ≈ 3.07 MHz at
  48 kHz stereo. RP2350 PIO handles far higher rates; the existing fractional
  clock divider (`system_clock * 4 / sample_freq`) already produces this
  correctly because the divider is per-sample, not per-bit.

The work is in the driver, PIO, and packing helper — not in the DSP graph.

## 4. Format model

### 4.1 New constant in `audio.h`

```c
#define AUDIO_BUFFER_FORMAT_PCM_S32 5   // signed 32-bit container, 24-in-32 audio (left-justified)
```

Value 5 preserves the existing upstream numbering (S16=1, S8=2, U16=3, U8=4).
Add a one-line comment documenting the 24-in-32 left-justified convention.

### 4.2 Producer buffer contract for S32

- Element type: `int32_t`.
- Layout: interleaved stereo, **`sample_stride = 8`** bytes (2 × `int32` per
  frame). `out[2*i]` = left, `out[2*i+1]` = right, mirroring the current S16
  convention.
- Value per word: `(int32)(clamped_float * 8388607.0f) << 8`.
  - Bits 31..8 hold the 24 audio bits.
  - Bits 7..0 are zero (padding the DAC clocks out but ignores).
- `sample_count` semantics: number of **frames** (consistent with S16 path,
  where one frame = one 32-bit DMA word). The DMA transfer count is derived
  from `sample_count` and stride in the driver (see §6).

## 5. PIO program

### 5.1 Dual program, runtime-select

Keep the existing 16-bit program intact. Add a sibling:

```
.program audio_i2s_32
.side_set 2
            ;        /--- LRCLK
            ;        |/-- BCLK
bitloop1:            ||
    out pins, 1      side 0b10
    jmp x-- bitloop1 side 0b11
    out pins, 1      side 0b00
    set x, 30        side 0b01     ; 32 bits per half-frame (was 14 → 16 bits)

bitloop0:
    out pins, 1      side 0b00
    jmp x-- bitloop0 side 0b01
    out pins, 1      side 0b10
public entry_point:
    set x, 30        side 0b11
```

And the matching `_swapped` variant (LRCLK/BCLK side-set order swapped, same
pattern as the existing `audio_i2s_swapped`).

### 5.2 Autopull and shift config — unchanged

- `sm_config_set_out_shift(&sm_config, false, true, 32)` — MSB-first,
  autopull at 32 bits. The 32-bit container means **one autopull per channel
  slot**; two slots per stereo frame = two autopulls = one DMA word per
  channel slot. Same as the existing S16 path's behavior at the FIFO level
  (just more bits shifted out per slot).
- Add `audio_i2s_32_program_init` mirroring `audio_i2s_program_init`.

### 5.3 Regenerating `audio_i2s.pio.h`

The compiled PIO header (`src/pico_audio_i2s/audio_i2s.pio.h`) is checked in
because pioasm is not in the build path. After editing `extras/audio_i2s.pio`,
the new program's instruction words must be added to `audio_i2s.pio.h`
manually (or via a one-shot pioasm run on a developer machine). The spec for
the new instruction sequence is fixed by §5.1, so this is mechanical:

| Addr | Encoding | Instruction |
|------|----------|-------------|
| 0 | `0x7001` | `out pins, 1   side 2` |
| 1 | `0x1840` | `jmp x--, 0    side 3` |
| 2 | `0x6001` | `out pins, 1   side 0` |
| 3 | `0xe83e` | `set x, 30     side 1` |
| 4 | `0x6001` | `out pins, 1   side 0` |
| 5 | `0x0844` | `jmp x--, 4    side 1` |
| 6 | `0x7001` | `out pins, 1   side 2` |
| 7 | `0xf83e` | `set x, 30     side 3` |

(The only delta from the existing program is the immediate operand of the
two `set` instructions: `0x2e`/14 → `0x3e`/30. Encoding: `set` with side-set
bit 0 set produces the documented word.)

For `audio_i2s_32_swapped`, swap side-set bits exactly as the existing
`audio_i2s_swapped` does relative to `audio_i2s`.

## 6. Driver changes in `audio_i2s.c`

### 6.1 `audio_i2s_setup`

After `pio_sm_claim`, branch on `intended_audio_format->format`:

- `AUDIO_BUFFER_FORMAT_PCM_S16` (existing) → load `audio_i2s_program` /
  `audio_i2s_swapped_program`.
- `AUDIO_BUFFER_FORMAT_PCM_S32` (new) → load `audio_i2s_32_program` /
  `audio_i2s_32_swapped_program`.

DMA transfer-size config (`channel_config_set_transfer_data_size` at line 88)
stays `DMA_SIZE_32` for both — stereo S16 already packs one frame per 32-bit
word; stereo S32 packs two 32-bit words per frame but the *width* of each DMA
transfer is still 32 bits.

### 6.2 `audio_i2s_connect_extra` (around line 203)

Replace the single `assert(producer->format->format == AUDIO_BUFFER_FORMAT_PCM_S16)`
with:

```c
assert(producer->format->format == AUDIO_BUFFER_FORMAT_PCM_S16
    || producer->format->format == AUDIO_BUFFER_FORMAT_PCM_S32);
pio_i2s_consumer_format.format = producer->format->format;
pio_i2s_consumer_format.sample_freq = producer->format->sample_freq;
pio_i2s_consumer_format.channel_count = 2;
pio_i2s_consumer_buffer_format.sample_stride =
    (producer->format->format == AUDIO_BUFFER_FORMAT_PCM_S32) ? 8 : 4;
```

The mono-output path (`PICO_AUDIO_I2S_MONO_OUTPUT`) is left S16-only for now;
a `panic("S32 mono not supported yet")` is acceptable since no example uses
mono.

### 6.3 `audio_start_dma_transfer` (around line 332-345)

Branch on `ab->format->format->format`:

```c
assert(ab->format->format->format == AUDIO_BUFFER_FORMAT_PCM_S16
    || ab->format->format->format == AUDIO_BUFFER_FORMAT_PCM_S32);
assert(ab->format->format->channel_count == 2);
// DMA transfer count: words per frame × sample frames.
//   S16: 1 word/frame (L|R packed).  S32: 2 words/frame (L, R separate).
uint32_t words_per_frame =
    (ab->format->format->format == AUDIO_BUFFER_FORMAT_PCM_S32) ? 2u : 1u;
dma_channel_transfer_from_buffer_now(
    shared_state.dma_channel, ab->buffer->bytes,
    ab->sample_count * words_per_frame);
```

The existing `assert(ab->format->sample_stride == 4)` becomes
`assert(ab->format->sample_stride == 4 || ab->format->sample_stride == 8)`.

### 6.4 S32 stereo pass-through connection

The existing pass-through is hard-typed to int16. `stereo_to_stereo_consumer_take`
and `stereo_to_stereo_producer_give` live in **`audio.cpp`** (not `buffer.c`),
where they instantiate `consumer_pool_take<Stereo<FmtS16>, Stereo<FmtS16>>` /
`producer_pool_blocking_give<Stereo<FmtS16>, Stereo<FmtS16>>`. The S32 work
touches three files in this order:

1. **`sample_conversion.h`** — add `FmtS32` (typedef-style, mirroring `FmtS16`).
   The identity `converting_copy<Stereo<FmtS32>, Stereo<FmtS32>>` template
   specialization already works generically (it falls into the memcpy path in
   `sample_conversion.h:164`), so no new converter arithmetic is needed —
   just the type alias.
2. **`audio.cpp`** — add two new C-callable functions next to the existing
   pair:
   ```c
   audio_buffer_t *stereo_to_stereo_consumer_take_s32(audio_connection_t *connection, bool block) {
       return consumer_pool_take<Stereo<FmtS32>, Stereo<FmtS32>>(connection, block);
   }
   void stereo_to_stereo_producer_give_s32(audio_connection_t *connection, audio_buffer_t *buffer) {
       return producer_pool_blocking_give<Stereo<FmtS32>, Stereo<FmtS32>>(connection, buffer);
   }
   ```
   Also declare them in `audio.h` alongside the existing s16 declarations.
3. **`audio_i2s.c`** — inside `wrap_consumer_take` (line 113) and
   `wrap_producer_give` (line 133), branch on
   `connection->producer_pool->format->format` to dispatch to the s16 or s32
   variant. Keeps the C-callable signature intact and avoids templating the
   whole connection struct.

The S8 path (`audio_i2s_connect_s8`, line 263) is **not** modified. It stays
S8-only and is unrelated to this work.

## 7. Packing helper in rpdsp

New function lives in the rpdsp library so all examples share one correct
implementation. Add to `algorithm.h` next to the existing `rpdsp::clamp` at
line 14 (same file, same namespace, same header-only style):

```cpp
namespace rpdsp {

/// Convert a [-1, 1] float to a 24-in-32 left-justified int32 word.
/// Bits 31..8 hold the 24 audio bits; bits 7..0 are zero (DAC padding).
/// Branch-free; suitable for the realtime callback.
static inline int32_t toInt24x32(float sample) {
    float clamped = rpdsp::clamp(sample, -1.0f, 1.0f);
    int32_t s = static_cast<int32_t>(clamped * 8388607.0f);  // truncate toward zero
    return s << 8;
}

}  // namespace rpdsp
```

Design notes:

- **No `roundf`.** `roundf` is a libc call and may not inline cleanly; the
  ~0.5 LSB error from truncation is inaudible at 24-bit. Keeps the helper
  branch-free and predictable for the realtime path.
- **Scale factor 8388607.0f** = 2²³−1, the max positive value of a signed
  24-bit sample. Clamping at +1.0f means the top code maps to 0x7FFFFF
  (positive full-scale); −1.0f maps to 0x800000 (negative full-scale).
- The shift `<< 8` is a single ARM `lsl` — essentially free.
- This replaces every sketch's `convertSampleToInt16` and the per-sketch
  `INT16_MAX_AS_FLOAT` constants.

## 8. Example migration (6 sketches)

Mechanical edit applied uniformly to each `.ino` in `Examples/`:

| Before | After |
|--------|-------|
| `.format = AUDIO_BUFFER_FORMAT_PCM_S16` | `.format = AUDIO_BUFFER_FORMAT_PCM_S32` |
| `.sample_stride = 4` | `.sample_stride = 8` |
| `int16_t *out = reinterpret_cast<int16_t*>(...)` | `int32_t *out = reinterpret_cast<int32_t*>(...)` |
| `out[2*i]   = convertSampleToInt16(x);` | `out[2*i]   = rpdsp::toInt24x32(x);` |
| `out[2*i+1] = convertSampleToInt16(y);` | `out[2*i+1] = rpdsp::toInt24x32(y);` |
| per-sketch `convertSampleToInt16` function | **deleted** |
| per-sketch `INT16_MAX_AS_FLOAT` / `INT16_MIN_AS_FLOAT` | **deleted** |

Files: `Oscillators.ino`, `SimpleOscillators.ino`, `Oscillator_HardSync.ino`,
`TwoChannelOscillator.ino`, `SuperSaw.ino`, `LadderFilter.ino`.

`SAMPLES_PER_BUFFER` values per sketch are unchanged — they describe frames,
not bytes, so they're bit-depth-agnostic. Buffer memory doubles for the
producer pool, but those are tiny (32 or 256 frames × 8 B × 3 buffers).

## 9. Testing and verification

### 9.1 Host tests (`tests/`, doctest, run in CI)

New test case for `rpdsp::toInt24x32`:

- `toInt24x32(0.0f)  == 0`
- `toInt24x32(1.0f)  == 0x7FFFFF00`   (positive full-scale)
- `toInt24x32(-1.0f) == -0x80000000`  (i.e. 0x80000000; negative full-scale)
- `toInt24x32(0.5f)  == 0x40000000`   (sanity: half-scale)
- Monotonic: `toInt24x32(a) < toInt24x32(b)` for `a < b` in (−1, 1)
- Clamping: `toInt24x32(2.0f)  == toInt24x32(1.0f)`
- Clamping: `toInt24x32(-2.0f) == toInt24x32(-1.0f)`

These run on host (MSVC/gcc) per `AGENTS/build.md` and catch math regressions
without hardware. Add them to the existing doctest suite.

### 9.2 Firmware (cannot be automated)

Per `AGENTS/build.md`, the PIO program, DMA configuration, and actual DAC
output cannot be verified off-target. Verification steps after implementation:

1. `scripts/build_all_examples.sh` (or `.ps1`) — all 6 examples must compile
   clean against the modified library. Catches the bulk of integration
   errors automatically.
2. Flash one sketch to the Pico 2, confirm tone plays at expected pitch with
   no louder noise floor than the int16 baseline (subjective, by ear).
3. Visually confirm BCLK/LRCLK rates on a scope if available: LRCLK should be
   48 kHz, BCLK ≈ 3.07 MHz (vs 1.53 MHz for int16).

### 9.3 Regression coverage for S16

The S16 path through the driver must still build and run. The simplest proof
is that `build_all_examples` compiles every example — but since *all* examples
are being migrated to S32, S16 support becomes compile-time-tested only via
the asserts remaining in `audio_i2s.c`. If S16 path coverage matters, a
throwaway S16 example can be kept around temporarily; this is optional and not
required by the spec.

## 10. Documentation updates

- **`AGENTS/audio-runtime.md`** — "Output format" section: rewrite to
  describe int32/24-in-32 left-justified as the new default; note int16 is
  still supported via the alternate PIO program and format constant;
  document `sample_stride = 8` and the `rpdsp::toInt24x32` helper.
- **`AGENTS/gotchas.md`** — add a "Bit depth" entry under or next to the
  "Sample rate" note: project default is now 24-in-32 via
  `AUDIO_BUFFER_FORMAT_PCM_S32`; int16 remains supported.
- **`Docs/rp2350_tuning.md`** — baseline config: keep `DSP format: float`;
  add an "Output format: 24-in-32 left-justified int32 on the I2S wire" line
  in the Codec/I2S boundary section.
- **BSD-3 notices** preserved verbatim on every `pico_audio_i2s` file touched
  (per `AGENTS/architecture.md` licensing rule). Edits are clearly
  repo-local additions on top of upstream code, not removals of upstream
  attribution.

## 11. Open questions / risks

- **PIO instruction encoding correctness (§5.3).** The hand-derived encodings
  for the new `set x, 30` must be exactly right or the program will miscount
  bits and the DAC will receive garbage. Mitigation: a one-shot `pioasm` run
  on a developer machine (or the CI pioasm if available) to regenerate the
  header; cross-check the two `set` operands by hand against the spec table.
  This is the single highest-risk item in the plan.
- **S32 mono path (§6.2).** Out of scope. If a future example wants mono
  24-bit, the panic in the connect function should be replaced with real
  handling. Not blocking.
- **`sample_count` semantics drift.** The S32 path doubles DMA words per
  frame. If any code assumes `sample_count == DMA transfer count`, it will
  under-transfer by 2×. The §6.3 branching handles this at the only place it
  matters (`audio_start_dma_transfer`); a `static_assert` or comment is
  worthwhile to flag the assumption.

## 12. Out of scope, explicitly

- Stereo width / channel-count changes.
- Higher sample rates (96 kHz, 192 kHz).
- Input path (ADC, microphone). Output only.
- Custom codec register configuration (the DSP library still does not own
  codec registers — `Docs/rp2350_tuning.md` §"I2S and Codec Boundary").
- Changing the dual-core split.
