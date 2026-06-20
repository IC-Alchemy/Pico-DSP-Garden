# Plan: Port DaisySP `LadderFilter` into rpdsp as `rpdsp::LadderFilter`

## Goal
Bring the Huovilainen New Moog (HNM) single-nonlinearity 4-pole ladder filter from
DaisySP into the canonical rpdsp library as a header-only class, adapted to rpdsp
conventions, with attribution preserved.

## Source of truth
- **Source (external, MIT):** `x:\CodePCB\Daisy\clean\DaisyExamples\DaisySP\Source\Filters\ladder.{h,cpp}`
  (van Hoesel / Infrasonic Audio; HNM model per Välimäki & Huovilainen CMJ 2006).
- **rpdsp conventions:** `libraries/rpdsp/src/rpdsp/` — see `filter.h` (existing filters),
  `algorithm.h` (`clamp`, `kTwoPi`, `safeSampleRate`, soon `fastTanh`), `realtime.h` (`zapDenormal`),
  `config.h` (`kDefaultSampleRate`).
- **Project rules:** `AGENTS.md` — lowercase rpdsp API, `prepare()` not `Init()`, MIT license,
  no host build/test framework (verify = compile a sketch via `arduino-cli`).

## Decisions (resolved)
1. **Location:** new file `libraries/rpdsp/src/rpdsp/ladder.h` (mirrors `hypersaw.h` precedent).
2. **Oversampling:** `static constexpr kInterpolation = 4` (editable/overridable). `alpha_` and
   `sr_int_recip_` derive from it, so changing the constant self-adjusts the tuning.
3. **fast_tanh:** promote to shared `rpdsp::fastTanh` in `algorithm.h`; `ladder.h` uses it.
   (`effects.h` keeps `std::tanh` — switching it is out of scope.)
4. **Block method:** include `process(float*, size_t)`; guard the GCC
   `__attribute__((optimize("unroll-loops")))` with `#if defined(__GNUC__)`.
5. **Denormals:** call `zapDenormal()` on `z1_[i]` in `LPF()` (matches all of `filter.h`);
   therefore `ladder.h` includes `realtime.h`.

## API (rpdsp lowercase, namespace `rpdsp`)
```cpp
class LadderFilter {
 public:
  enum class Mode { LP24, LP12, BP24, BP12, HP24, HP12 };

  void prepare(float sampleRate);                 // uses safeSampleRate(); sets defaults
  void reset();                                   // zeros z0_[4], z1_[4], oldinput_
  float process(float in);
  void process(float* buf, size_t size);          // in-place block; guarded GCC attribute
  void setFreq(float hz);                         // clamps [5, sr*0.425] in compute_coeffs
  void setRes(float res);                         // 0..1.8 -> K = 4*res
  void setMode(Mode mode);                        // default LP24
  void setPassbandGain(float pbg);                // 0..0.5
  void setInputDrive(float drv);                  // 0..4
};
```
- Enum is **nested** as `LadderFilter::Mode` (idiomatic; not a top-level `FilterMode`).
- `setFreq`/`process`/`processBlock` semantics identical to source.

## Mechanical ports (no behavior change)
- `daisysp::fclamp` → `rpdsp::clamp`; `daisysp::fmax`/`fmin` → `std::max`/`std::min`.
- `2.0f * PI_F` → `kTwoPi` (compute_coeffs).
- File-local `fast_tanh` → `rpdsp::fastTanh`.
- `Init` → `prepare`; `Process` → `process`; `SetFreq/SetRes/SetPassbandGain/SetInputDrive/SetFilterMode`
  → `setFreq/setRes/setPassbandGain/setInputDrive/setMode`.
- Header-only: move `.cpp` bodies inline into `ladder.h` (private helpers `LPF`, `compute_coeffs`,
  `weightedSumForCurrentMode` as inline private methods).
- Includes in `ladder.h`: `<array>`, `<cstddef>`, `<cstdint>`, `"algorithm.h"`, `"realtime.h"`.

## Cleanups vs. source
- **Remove dead `beta_[4]`** member (declared, never read).
- **Keep** the source's cutoff clamp `[5, sr*0.425]` in `compute_coeffs` — this is model-tuned;
  do NOT replace with `clampCutoff` (which would widen to nyquist*0.99 and break the HNM coeff range).
- **Keep** `kMaxResonance = 1.8` and the `Qadjust_` polynomial verbatim.
- **Retain** the full MIT attribution header block (van Hoesel / Infrasonic Audio) at top of `ladder.h`,
  plus a one-line note that the rpdsp port is MIT and tracks the rpdsp conventions.

## `rpdsp::fastTanh` (add to `algorithm.h`)
```cpp
// Cheap rational tanh approx (|x|>3 saturates to +/-1). For saturators/clippers in the DSP path.
inline float fastTanh(float x) {
  if (x > 3.0f) return 1.0f;
  if (x < -3.0f) return -1.0f;
  const float x2 = x * x;
  return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}
```

## Files changed
1. `libraries/rpdsp/src/rpdsp/algorithm.h` — add `rpdsp::fastTanh`.
2. `libraries/rpdsp/src/rpdsp/ladder.h` — NEW: full `rpdsp::LadderFilter`.

## Out of scope
- No example sketch is required to ship the class (no existing example uses `filter.h` filters).
  Validation can fold the `#include` + instantiation into any existing example temporarily.
- No changes to `effects.h` (optional future swap to `fastTanh`).
- No changes to `Docs/` beyond what naturally follows.

## Risks / gotchas
- **CPU in the audio callback:** at 4x oversampling + per-sample `fastTanh` + 4 LPF stages, this is
  the heaviest filter in rpdsp. The constexpr `kInterpolation` lets an RP2350 build drop to 2x if the
  realtime budget (<50% of ~0.667 ms / 32-frame block) is blown. Flag this in a header comment.
- **Real-time safety:** `process`/`processBlock` must stay allocation-free, no `Serial`/blocking.
  The `std::array<float,5>` in `weightedSumForCurrentMode` is stack-only — OK. `zapDenormal` added.
- **Header comment drift:** source `.cpp` and `.h` disagree on "2x" vs "4x" oversampling; the real
  constant is 4. Use 4 in the rpdsp header comment and don't copy the contradictory line.
- **Cutoff clamp:** must remain the model-specific `[5, sr*0.425]`, not rpdsp's generic `clampCutoff`.

## Validation
No host build / test framework exists. Verify by compiling under `arduino-cli` (earle-philhower core):
```sh
arduino-cli compile \
  --fqbn rp2040:rp2040:rpipico2:flash=4194304_0,freq=200,opt=Optimize3,usbstack=picosdk \
  --library libraries/rpdsp --library libraries/pico_audio_i2s \
  Examples/<Name>
```
Minimal compile check: temporarily add to an example's `.ino`
```cpp
#include <rpdsp/ladder.h>
rpdsp::LadderFilter ladder;
// in setup:  ladder.prepare(48000.0f); ladder.setFreq(1000); ladder.setRes(0.3f);
// in fill:   float y = ladder.process(x);
```
Success = clean compile for the RP2350 target. Listening test optional (classic Moog-style sweep,
self-oscillation at high `setRes`, mode switching LP/BP/HP 12/24).

## Open questions
None — all decisions resolved. Scope notes taken as decided: nested `enum class Mode`, and keeping
the source's `[5, sr*0.425]` cutoff clamp.
