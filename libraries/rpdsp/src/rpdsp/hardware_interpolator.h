#pragma once

#include <cstdint>
#include <cstddef>
#include <cmath>

#if defined(__has_include)
#if __has_include(<hardware/interp.h>)
#define RPDSP_HAS_HARDWARE_INTERP 1
#endif
#endif

#ifndef RPDSP_HAS_HARDWARE_INTERP
#define RPDSP_HAS_HARDWARE_INTERP 0
#endif

#if RPDSP_HAS_HARDWARE_INTERP
#include <hardware/interp.h>
#else
// Dummy structures and functions to compile on host
struct interp_hw_t {
  std::uint32_t accum[2];
  std::uint32_t base[3];
  std::uint32_t peek[3];
  std::uint32_t pop[3];
  std::uint32_t add_raw[2];
  std::uint32_t base01;
};

inline interp_hw_t dummyInterp0;
inline interp_hw_t dummyInterp1;

#ifndef interp0
#define interp0 (&dummyInterp0)
#endif
#ifndef interp1
#define interp1 (&dummyInterp1)
#endif

struct interp_config {
  std::uint32_t shift;
  std::uint32_t mask_lsb;
  std::uint32_t mask_msb;
  bool is_signed;
  bool cross_input;
  bool cross_result;
  bool blend;
  bool clamp;
  bool add_raw;
};

inline interp_config interp_default_config() {
  return interp_config{0, 0, 0, false, false, false, false, false, false};
}
inline void interp_config_set_shift(interp_config* c, std::uint32_t shift) { c->shift = shift; }
inline void interp_config_set_mask(interp_config* c, std::uint32_t lsb, std::uint32_t msb) {
  c->mask_lsb = lsb;
  c->mask_msb = msb;
}
inline void interp_config_set_signed(interp_config* c, bool is_signed) { c->is_signed = is_signed; }
inline void interp_config_set_cross_input(interp_config* c, bool cross_input) { c->cross_input = cross_input; }
inline void interp_config_set_cross_result(interp_config* c, bool cross_result) { c->cross_result = cross_result; }
inline void interp_config_set_blend(interp_config* c, bool blend) { c->blend = blend; }
inline void interp_config_set_clamp(interp_config* c, bool clamp) { c->clamp = clamp; }
inline void interp_config_set_add_raw(interp_config* c, bool add_raw) { c->add_raw = add_raw; }
inline void interp_set_config(interp_hw_t*, std::uint32_t, const interp_config*) {}
#endif

namespace rpdsp {

class HardwareInterpolatorPool {
 public:
  enum class Resource {
    Core0Interp0 = 0,
    Core0Interp1 = 1,
    Core1Interp0 = 2,
    Core1Interp1 = 3,
  };

  static bool claim(Resource resource) {
    std::uint32_t bit = 1u << static_cast<std::uint32_t>(resource);
    if (claimedMask_ & bit) {
      return false;
    }
    claimedMask_ |= bit;
    return true;
  }

  static void release(Resource resource) {
    std::uint32_t bit = 1u << static_cast<std::uint32_t>(resource);
    claimedMask_ &= ~bit;
  }

  static interp_hw_t* getHw(Resource resource) {
    switch (resource) {
      case Resource::Core0Interp0:
      case Resource::Core1Interp0:
        return interp0;
      case Resource::Core0Interp1:
      case Resource::Core1Interp1:
        return interp1;
    }
    return nullptr;
  }

 private:
  inline static std::uint32_t claimedMask_ = 0;
};

class HardwareWavefolder {
 public:
  HardwareWavefolder() = default;
  ~HardwareWavefolder() { deinit(); }

  HardwareWavefolder(const HardwareWavefolder&) = delete;
  HardwareWavefolder& operator=(const HardwareWavefolder&) = delete;

  int init(HardwareInterpolatorPool::Resource resource, std::uint8_t foldOrder) {
    if (foldOrder > 30) {
      return -1;
    }

    hw_ = HardwareInterpolatorPool::getHw(resource);
    if (!hw_) {
      return -2;
    }

    if (!HardwareInterpolatorPool::claim(resource)) {
      return -3;
    }

    resource_ = resource;
    foldOrder_ = foldOrder;
    numStages_ = 1;
    preGainQ16_ = 65536;

#if RPDSP_HAS_HARDWARE_INTERP
    lane0Cfg_ = interp_default_config();
    interp_config_set_shift(&lane0Cfg_, 0);
    interp_config_set_mask(&lane0Cfg_, 0, foldOrder);
    interp_config_set_signed(&lane0Cfg_, true);
    interp_set_config(hw_, 0, &lane0Cfg_);

    hw_->base[0] = 0;
#endif

    initialized_ = true;
    return 0;
  }

  void deinit() {
    if (initialized_) {
      HardwareInterpolatorPool::release(resource_);
      initialized_ = false;
      hw_ = nullptr;
    }
  }

  void setGain(float preGain) {
    preGainQ16_ = static_cast<std::int32_t>(preGain * 65536.0f);
  }

  void setStages(std::uint8_t numStages) {
    if (numStages < 1 || numStages > 8) {
      return;
    }
    numStages_ = numStages;
  }

  std::int32_t process(std::int32_t sample) const {
    std::int64_t gained = (static_cast<std::int64_t>(sample) * preGainQ16_) >> 16;
    std::int32_t val;
    if (gained > 2147483647LL) {
      val = 2147483647;
    } else if (gained < -2147483648LL) {
      val = -2147483648;
    } else {
      val = static_cast<std::int32_t>(gained);
    }

#if RPDSP_HAS_HARDWARE_INTERP
    for (std::uint8_t s = 0; s < numStages_; s++) {
      std::uint32_t abs_val = static_cast<std::uint32_t>(val < 0 ? -static_cast<std::int64_t>(val) : static_cast<std::int64_t>(val));
      hw_->accum[0] = abs_val;
      std::int32_t tri = static_cast<std::int32_t>(hw_->peek[0]);
      val = val < 0 ? -tri : tri;
    }
    return val;
#else
    // Software folding simulation on host
    std::int32_t foldLimit = 1 << foldOrder_;
    for (std::uint8_t s = 0; s < numStages_; s++) {
      std::int32_t absVal = val < 0 ? -val : val;
      std::int32_t doubleLimit = 2 * foldLimit;
      std::int32_t mod = absVal % doubleLimit;
      std::int32_t tri = foldLimit - std::abs(mod - foldLimit);
      val = val < 0 ? -tri : tri;
    }
    return val;
#endif
  }

  void processBlock(const std::int32_t* src, std::int32_t* dst, size_t count) const {
    for (size_t i = 0; i < count; i++) {
      dst[i] = process(src[i]);
    }
  }

 private:
  interp_hw_t* hw_ = nullptr;
  HardwareInterpolatorPool::Resource resource_ = HardwareInterpolatorPool::Resource::Core0Interp0;
  interp_config lane0Cfg_;
  std::uint8_t foldOrder_ = 0;
  std::uint8_t numStages_ = 1;
  std::int32_t preGainQ16_ = 65536;
  bool initialized_ = false;
};

// Single-cycle linear crossfade using INTERP0 blend mode.
//
// Computes  out = a + ((b - a) * alpha) >> 8  for an 8-bit blend weight alpha
// (0 => all a, 255 => almost all b). On the RP2xxx this is one interpolator
// read with no branches or multiply on the CPU — the hardware datapath does the
// subtract, multiply and shift. This is the primitive behind the Cornell
// "audio cross-fade via blend mode" demo, where two signed audio sources are
// mixed by sweeping alpha.
class HardwareBlend {
 public:
  HardwareBlend() = default;
  ~HardwareBlend() { deinit(); }

  HardwareBlend(const HardwareBlend&) = delete;
  HardwareBlend& operator=(const HardwareBlend&) = delete;

  // Blend mode is only available on an interp0 lane.
  int init(HardwareInterpolatorPool::Resource resource) {
    if (resource != HardwareInterpolatorPool::Resource::Core0Interp0 &&
        resource != HardwareInterpolatorPool::Resource::Core1Interp0) {
      return -4; // Blend is only supported on interp0
    }

    hw_ = HardwareInterpolatorPool::getHw(resource);
    if (!hw_) {
      return -2;
    }
    if (!HardwareInterpolatorPool::claim(resource)) {
      return -3;
    }
    resource_ = resource;

#if RPDSP_HAS_HARDWARE_INTERP
    // Lane 0 carries the blend; lane 1 supplies the signed alpha weight.
    lane0Cfg_ = interp_default_config();
    interp_config_set_blend(&lane0Cfg_, true);
    interp_set_config(hw_, 0, &lane0Cfg_);

    lane1Cfg_ = interp_default_config();
    interp_config_set_signed(&lane1Cfg_, true);
    interp_set_config(hw_, 1, &lane1Cfg_);

    hw_->accum[1] = 0;
    hw_->base[0] = 0;
    hw_->base[1] = 0;
#endif

    initialized_ = true;
    return 0;
  }

  void deinit() {
    if (initialized_) {
      HardwareInterpolatorPool::release(resource_);
      initialized_ = false;
      hw_ = nullptr;
    }
  }

  // alphaQ8 in [0, 255]: 0 selects a, 255 selects (almost all) b.
  void setAlphaQ8(std::uint8_t alphaQ8) {
    alphaQ8_ = alphaQ8;
#if RPDSP_HAS_HARDWARE_INTERP
    hw_->accum[1] = alphaQ8;
#endif
  }

  // Crossfade with the alpha set by the most recent setAlphaQ8().
  std::int32_t process(std::int32_t a, std::int32_t b) const {
#if RPDSP_HAS_HARDWARE_INTERP
    hw_->base[0] = static_cast<std::uint32_t>(a);
    hw_->base[1] = static_cast<std::uint32_t>(b);
    return static_cast<std::int32_t>(hw_->peek[1]);
#else
    return a + (((b - a) * static_cast<std::int32_t>(alphaQ8_)) >> 8);
#endif
  }

  // Convenience: set alpha and crossfade in one call.
  std::int32_t blend(std::int32_t a, std::int32_t b, std::uint8_t alphaQ8) {
    setAlphaQ8(alphaQ8);
    return process(a, b);
  }

 private:
  interp_hw_t* hw_ = nullptr;
  HardwareInterpolatorPool::Resource resource_ = HardwareInterpolatorPool::Resource::Core0Interp0;
  std::uint8_t alphaQ8_ = 0;
  bool initialized_ = false;
#if RPDSP_HAS_HARDWARE_INTERP
  interp_config lane0Cfg_;
  interp_config lane1Cfg_;
#endif
};

// Single-cycle saturating clamp using INTERP1 clamp mode.
//
// Computes  out = min(max(x, lo), hi)  with no branches on the CPU. Clamp mode
// is only available on interp1. This is the primitive behind the Cornell
// "tone burst" demo, where an integrating envelope accumulator is clamped
// between zero and a peak amplitude before it scales the oscillator.
class HardwareClamp {
 public:
  HardwareClamp() = default;
  ~HardwareClamp() { deinit(); }

  HardwareClamp(const HardwareClamp&) = delete;
  HardwareClamp& operator=(const HardwareClamp&) = delete;

  int init(HardwareInterpolatorPool::Resource resource, std::int32_t lo, std::int32_t hi) {
    if (resource != HardwareInterpolatorPool::Resource::Core0Interp1 &&
        resource != HardwareInterpolatorPool::Resource::Core1Interp1) {
      return -4; // Clamp is only supported on interp1
    }

    hw_ = HardwareInterpolatorPool::getHw(resource);
    if (!hw_) {
      return -2;
    }
    if (!HardwareInterpolatorPool::claim(resource)) {
      return -3;
    }
    resource_ = resource;
    lo_ = lo;
    hi_ = hi;

#if RPDSP_HAS_HARDWARE_INTERP
    lane0Cfg_ = interp_default_config();
    interp_config_set_clamp(&lane0Cfg_, true);
    interp_config_set_signed(&lane0Cfg_, true);
    interp_set_config(hw_, 0, &lane0Cfg_);

    lane1Cfg_ = interp_default_config();
    interp_set_config(hw_, 1, &lane1Cfg_);

    hw_->base[0] = static_cast<std::uint32_t>(lo);
    hw_->base[1] = static_cast<std::uint32_t>(hi);
#endif

    initialized_ = true;
    return 0;
  }

  void deinit() {
    if (initialized_) {
      HardwareInterpolatorPool::release(resource_);
      initialized_ = false;
      hw_ = nullptr;
    }
  }

  void setRange(std::int32_t lo, std::int32_t hi) {
    lo_ = lo;
    hi_ = hi;
#if RPDSP_HAS_HARDWARE_INTERP
    hw_->base[0] = static_cast<std::uint32_t>(lo);
    hw_->base[1] = static_cast<std::uint32_t>(hi);
#endif
  }

  std::int32_t process(std::int32_t x) const {
#if RPDSP_HAS_HARDWARE_INTERP
    hw_->accum[0] = static_cast<std::uint32_t>(x);
    return static_cast<std::int32_t>(hw_->peek[0]);
#else
    return x < lo_ ? lo_ : (x > hi_ ? hi_ : x);
#endif
  }

 private:
  interp_hw_t* hw_ = nullptr;
  HardwareInterpolatorPool::Resource resource_ = HardwareInterpolatorPool::Resource::Core0Interp1;
  std::int32_t lo_ = 0;
  std::int32_t hi_ = 0;
  bool initialized_ = false;
#if RPDSP_HAS_HARDWARE_INTERP
  interp_config lane0Cfg_;
  interp_config lane1Cfg_;
#endif
};

class HardwareOscillator {
 public:
  HardwareOscillator() = default;

  HardwareOscillator(const HardwareOscillator&) = delete;
  HardwareOscillator& operator=(const HardwareOscillator&) = delete;

  static bool isPowerOfTwo(std::uint32_t value) {
    return value != 0u && (value & (value - 1u)) == 0u;
  }

  static std::uint8_t log2U32(std::uint32_t value) {
    std::uint8_t bits = 0u;
    while (value > 1u) {
      value >>= 1u;
      ++bits;
    }
    return bits;
  }

  static std::uint8_t tableBitCount(std::uint32_t tableLength) {
    if (!isPowerOfTwo(tableLength) || tableLength < 2u) {
      return 0u;
    }
    return log2U32(tableLength);
  }

  static int tuningWordFromFrequency(std::uint32_t tableLength,
                                     std::uint8_t fractionalBits,
                                     std::uint32_t sampleRateHz,
                                     std::uint32_t frequencyHz,
                                     std::uint32_t& phaseIncrementQnOut) {
    if (sampleRateHz == 0u) {
      return -1; // Invalid argument
    }
    if (tableLength < 2u || !isPowerOfTwo(tableLength)) {
      return -2; // Invalid table
    }
    std::uint8_t tableBits = tableBitCount(tableLength);
    if (fractionalBits < 8u || (tableBits + fractionalBits > 32u)) {
      return -3; // Invalid configuration
    }

    std::uint64_t numerator = static_cast<std::uint64_t>(frequencyHz) * static_cast<std::uint64_t>(tableLength);
    numerator <<= fractionalBits;
    phaseIncrementQnOut = static_cast<std::uint32_t>(numerator / static_cast<std::uint64_t>(sampleRateHz));
    return 0;
  }

  int init(interp_hw_t* interp,
           const std::int16_t* table,
           std::uint32_t tableLength,
           std::uint8_t fractionalBits,
           std::uint32_t initialPhaseQn,
           std::uint32_t phaseIncrementQn) {
    if (interp == nullptr || table == nullptr) {
      return -1; // Invalid argument
    }
    if (tableLength < 2u || !isPowerOfTwo(tableLength)) {
      return -2; // Invalid table
    }
    std::uint8_t tableBits = tableBitCount(tableLength);
    if (fractionalBits < 8u || (tableBits + fractionalBits > 32u)) {
      return -3; // Invalid configuration
    }

    interp_ = interp;
    table_ = table;
    tableLength_ = tableLength;
    tableMask_ = tableLength - 1u;
    tableBits_ = tableBits;
    fractionalBits_ = fractionalBits;
    phaseIncrementQn_ = phaseIncrementQn;

#if RPDSP_HAS_HARDWARE_INTERP
    if (interp_ != interp0) {
      return -4; // Blend is only supported on interp0
    }

    lane0Cfg_ = interp_default_config();
    interp_config_set_shift(&lane0Cfg_, fractionalBits_);
    interp_config_set_mask(&lane0Cfg_, 0u, tableBits_ - 1u);
    interp_config_set_blend(&lane0Cfg_, true);
    interp_set_config(interp_, 0, &lane0Cfg_);

    lane1Cfg_ = interp_default_config();
    interp_config_set_shift(&lane1Cfg_, fractionalBits_ - 8u);
    interp_config_set_cross_input(&lane1Cfg_, true);
    interp_config_set_signed(&lane1Cfg_, true);
    interp_set_config(interp_, 1, &lane1Cfg_);

    interp_->base[0] = 0u;
    interp_->base[1] = 0u;
    interp_->base[2] = 0u;
#endif

    setPhase(initialPhaseQn);
    return 0;
  }

  int setTable(const std::int16_t* table, std::uint32_t tableLength) {
    if (table == nullptr) return -1;
    if (tableLength < 2u || !isPowerOfTwo(tableLength)) return -2;

    table_ = table;
    tableLength_ = tableLength;
    tableMask_ = tableLength - 1u;
    tableBits_ = tableBitCount(tableLength);

#if RPDSP_HAS_HARDWARE_INTERP
    if (interp_ == interp0) {
      lane0Cfg_ = interp_default_config();
      interp_config_set_shift(&lane0Cfg_, fractionalBits_);
      interp_config_set_mask(&lane0Cfg_, 0u, tableBits_ - 1u);
      interp_config_set_blend(&lane0Cfg_, true);
      interp_set_config(interp_, 0, &lane0Cfg_);
    }
#endif

    return loadEndpoints();
  }

  int setPhase(std::uint32_t phaseQn) {
#if RPDSP_HAS_HARDWARE_INTERP
    interp_->accum[0] = phaseQn;
#else
    phase_ = phaseQn;
#endif
    return loadEndpoints();
  }

  int setPhaseIncrement(std::uint32_t phaseIncrementQn) {
    phaseIncrementQn_ = phaseIncrementQn;
    return 0;
  }

  std::uint32_t getPhase() const {
#if RPDSP_HAS_HARDWARE_INTERP
    return interp_->accum[0];
#else
    return phase_;
#endif
  }

  std::uint32_t getTableIndex() const {
#if RPDSP_HAS_HARDWARE_INTERP
    // add_raw[0] is the raw phase accumulator; the integer table index lives in
    // the bits above the fractional part, so shift the fraction out first.
    return (interp_->add_raw[0] >> fractionalBits_) & tableMask_;
#else
    return (phase_ >> fractionalBits_) & tableMask_;
#endif
  }

  std::uint8_t getFractionQ8() const {
#if RPDSP_HAS_HARDWARE_INTERP
    // Top 8 bits of the fractional part = the Q8 blend weight (alpha).
    return static_cast<std::uint8_t>((interp_->add_raw[0] >> (fractionalBits_ - 8)) & 0xffu);
#else
    std::uint32_t fractionMask = (1u << fractionalBits_) - 1;
    return static_cast<std::uint8_t>((phase_ & fractionMask) >> (fractionalBits_ - 8));
#endif
  }

  int peekSample(std::int32_t& sampleOut) {
    int status = loadEndpoints();
    if (status != 0) return status;

#if RPDSP_HAS_HARDWARE_INTERP
    sampleOut = static_cast<std::int32_t>(interp_->peek[1]);
#else
    std::uint32_t index = (phase_ >> fractionalBits_) & tableMask_;
    std::uint32_t nextIndex = (index + 1u) & tableMask_;
    std::int16_t sampleA = table_[index];
    std::int16_t sampleB = table_[nextIndex];
    std::uint32_t fractionMask = (1u << fractionalBits_) - 1;
    std::uint32_t fraction = phase_ & fractionMask;
    std::uint32_t alpha = fraction >> (fractionalBits_ - 8);
    sampleOut = sampleA + static_cast<std::int32_t>(alpha) * (sampleB - sampleA) / 256;
#endif
    return 0;
  }

  int nextSample(std::int32_t& sampleOut) {
    int status = peekSample(sampleOut);
    if (status != 0) return status;

#if RPDSP_HAS_HARDWARE_INTERP
    interp_->add_raw[0] = phaseIncrementQn_;
#else
    phase_ += phaseIncrementQn_;
#endif
    return 0;
  }

 private:
  int loadEndpoints() {
    if (table_ == nullptr) return -1;

#if RPDSP_HAS_HARDWARE_INTERP
    std::uint32_t index = (interp_->add_raw[0] >> fractionalBits_) & tableMask_;
    std::int16_t sample_a = table_[index];
    std::int16_t sample_b = table_[(index + 1u) & tableMask_];
    interp_->base01 = (static_cast<std::uint32_t>(static_cast<std::uint16_t>(sample_a))) |
                      (static_cast<std::uint32_t>(static_cast<std::uint16_t>(sample_b)) << 16);
#endif
    return 0;
  }

  interp_hw_t* interp_ = nullptr;
  const std::int16_t* table_ = nullptr;
  std::uint32_t tableLength_ = 0;
  std::uint32_t tableMask_ = 0;
  std::uint32_t phaseIncrementQn_ = 0;
  std::uint8_t tableBits_ = 0;
  std::uint8_t fractionalBits_ = 0;

#if RPDSP_HAS_HARDWARE_INTERP
  interp_config lane0Cfg_;
  interp_config lane1Cfg_;
#else
  std::uint32_t phase_ = 0;
#endif
};

} // namespace rpdsp
