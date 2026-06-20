#pragma once

#include <cmath>
#include <cstdint>

namespace rpdsp {

inline float zapDenormal(float value) {
  // Very small feedback states can force slow CPU denormal handling on host builds.
  return std::fabs(value) < 1.0e-20f ? 0.0f : value;
}

class XorShift32 {
 public:
  // A zero xorshift state is stuck forever, so fold it to a non-zero seed.
  explicit XorShift32(std::uint32_t seed = 0x12345678u) : state_(seed == 0 ? 1u : seed) {}

  std::uint32_t nextU32() {
    // Small deterministic PRNG: suitable for noise sources, not for cryptography.
    std::uint32_t x = state_;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state_ = x;
    return x;
  }

  float nextBipolar() {
    constexpr float scale = 1.0f / 2147483648.0f;
    return static_cast<float>(static_cast<std::int32_t>(nextU32())) * scale;
  }

 private:
  std::uint32_t state_;
};

}  // namespace rpdsp
