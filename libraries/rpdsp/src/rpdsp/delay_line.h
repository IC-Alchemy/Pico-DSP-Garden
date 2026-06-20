#pragma once

#include <array>
#include <cassert>
#include <cstddef>

namespace rpdsp {

template <size_t Capacity>
class DelayLine {
  static_assert(Capacity > 1, "DelayLine capacity must be greater than one sample.");

 public:
  // Capacity is a template parameter so delay memory is reserved before audio starts.
  void reset() { buffer_.fill(0.0f); writeIndex_ = 0; }

  void push(float value) {
    buffer_[writeIndex_] = value;
    ++writeIndex_;
    if (writeIndex_ == Capacity) {
      writeIndex_ = 0;
    }
  }

  float read(size_t delaySamples) const {
    assert(delaySamples < Capacity);
    // writeIndex_ points at the next slot, so one sample of delay is just behind it.
    const size_t readOffset = delaySamples + 1;
    const size_t index = writeIndex_ >= readOffset
                                      ? writeIndex_ - readOffset
                                      : Capacity + writeIndex_ - readOffset;
    return buffer_[index];
  }

  float readLinear(float delaySamples) const {
    // Fractional delay is enough for modulation effects without a costly resampler.
    // Valid reads are clamped to the fixed buffer range: [0, Capacity - 1].
    delaySamples = clampDelaySamples(delaySamples);
    const auto whole = static_cast<size_t>(delaySamples);
    const float frac = delaySamples - static_cast<float>(whole);
    const float a = read(whole);
    const float b = read(nextDelayIndex(whole));
    return a + (b - a) * frac;
  }

  float readCubic(float delaySamples) const {
    // Third-order Lagrange interpolation keeps modulated delay taps smoother than
    // linear interpolation while using only four bounded reads from fixed storage.
    delaySamples = clampDelaySamples(delaySamples);
    const auto whole = static_cast<size_t>(delaySamples);
    const float frac = delaySamples - static_cast<float>(whole);

    const float ym1 = read(previousDelayIndex(whole));
    const float y0 = read(whole);
    const float y1 = read(nextDelayIndex(whole));
    const float y2 = read(nextDelayIndex(nextDelayIndex(whole)));

    const float c0 = (-frac * (frac - 1.0f) * (frac - 2.0f)) * (1.0f / 6.0f);
    const float c1 = ((frac + 1.0f) * (frac - 1.0f) * (frac - 2.0f)) * 0.5f;
    const float c2 = (-(frac + 1.0f) * frac * (frac - 2.0f)) * 0.5f;
    const float c3 = ((frac + 1.0f) * frac * (frac - 1.0f)) * (1.0f / 6.0f);

    return (ym1 * c0) + (y0 * c1) + (y1 * c2) + (y2 * c3);
  }

 private:
  static float clampDelaySamples(float delaySamples) {
    if (delaySamples < 0.0f) {
      return 0.0f;
    }
    const float maxDelay = static_cast<float>(Capacity - 1);
    if (delaySamples > maxDelay) {
      return maxDelay;
    }
    return delaySamples;
  }

  static constexpr size_t previousDelayIndex(size_t delaySamples) {
    return delaySamples > 0 ? delaySamples - 1 : 0;
  }

  static constexpr size_t nextDelayIndex(size_t delaySamples) {
    return delaySamples < Capacity - 1 ? delaySamples + 1 : Capacity - 1;
  }

  std::array<float, Capacity> buffer_{};
  size_t writeIndex_ = 0;
};

}  // namespace rpdsp
