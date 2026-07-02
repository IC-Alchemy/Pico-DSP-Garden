#pragma once

#include "algorithm.h"
#include "parameter_smoother.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace rpdsp {

template <std::size_t NumSliders>
class MuxSliderScanner {
 public:
  void prepare(float sampleRate, float smoothingMilliseconds = 8.0f) {
    for (auto& smoother : smoothers_) {
      smoother.prepare(sampleRate, smoothingMilliseconds);
      smoother.reset(0.0f);
    }
    values_.fill(0.0f);
    quantized_.fill(0);
  }

  template <typename SelectChannel, typename DelaySettling, typename ReadAdc>
  void scan(SelectChannel&& selectChannel, DelaySettling&& delaySettling, ReadAdc&& readAdc) {
    for (std::size_t channel = 0; channel < NumSliders; ++channel) {
      selectChannel(channel);
      delaySettling();

      const int a = readAdc();
      const int b = readAdc();
      const int c = readAdc();
      const int median = median3(a, b, c);
      const float normalized = clamp(static_cast<float>(median) * (1.0f / 1023.0f), 0.0f, 1.0f);

      const int q = static_cast<int>((normalized * 127.0f) + 0.5f);
      if (q != quantized_[channel]) {
        quantized_[channel] = q;
        smoothers_[channel].setTarget(static_cast<float>(q) * (1.0f / 127.0f));
      }
      values_[channel] = smoothers_[channel].next();
    }
  }

  [[nodiscard]] float value(std::size_t index) const {
    return index < NumSliders ? values_[index] : 0.0f;
  }

  [[nodiscard]] int quantizedValue(std::size_t index) const {
    return index < NumSliders ? quantized_[index] : 0;
  }

 private:
  static int median3(int a, int b, int c) {
    if (a > b) {
      const int t = a;
      a = b;
      b = t;
    }
    if (b > c) {
      const int t = b;
      b = c;
      c = t;
    }
    if (a > b) {
      const int t = a;
      a = b;
      b = t;
    }
    return b;
  }

  std::array<float, NumSliders> values_{};
  std::array<int, NumSliders> quantized_{};
  std::array<LinearSmoother, NumSliders> smoothers_{};
};

class DebouncedButton {
 public:
  void reset(bool rawPressed = false, std::uint32_t nowMilliseconds = 0) {
    stablePressed_ = rawPressed;
    candidatePressed_ = rawPressed;
    lastChangeMilliseconds_ = nowMilliseconds;
    pressedEdge_ = false;
  }

  bool update(bool rawPressed, std::uint32_t nowMilliseconds) {
    pressedEdge_ = false;

    if (rawPressed != candidatePressed_) {
      candidatePressed_ = rawPressed;
      lastChangeMilliseconds_ = nowMilliseconds;
      return false;
    }

    if (candidatePressed_ != stablePressed_ &&
        elapsed(nowMilliseconds, lastChangeMilliseconds_) >= debounceMilliseconds_) {
      stablePressed_ = candidatePressed_;
      pressedEdge_ = stablePressed_;
    }

    return pressedEdge_;
  }

  void setDebounceMilliseconds(std::uint32_t debounceMilliseconds) {
    debounceMilliseconds_ = debounceMilliseconds;
  }

  [[nodiscard]] bool pressed() const { return pressedEdge_; }
  [[nodiscard]] bool held() const { return stablePressed_; }

 private:
  static std::uint32_t elapsed(std::uint32_t now, std::uint32_t since) {
    return now - since;
  }

  std::uint32_t debounceMilliseconds_ = 5;
  std::uint32_t lastChangeMilliseconds_ = 0;
  bool stablePressed_ = false;
  bool candidatePressed_ = false;
  bool pressedEdge_ = false;
};

}  // namespace rpdsp
