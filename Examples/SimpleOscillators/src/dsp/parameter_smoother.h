#pragma once

#include "rp2350_audio_dsp/dsp/config.h"

#include <algorithm>
#include <cmath>

namespace rpdsp {

class LinearSmoother {
 public:
  void prepare(float sampleRate, float rampMilliseconds = 5.0f) {
    sampleRate_ = sampleRate > 1.0f ? sampleRate : kDefaultSampleRate;
    setRampMilliseconds(rampMilliseconds);
  }

  void reset(float value = 0.0f) {
    current_ = value;
    target_ = value;
    step_ = 0.0f;
    remaining_ = 0;
  }

  void setRampMilliseconds(float rampMilliseconds) {
    // At least one sample avoids divide-by-zero while still allowing near-instant moves.
    const float clamped = std::max(0.0f, rampMilliseconds);
    rampSamples_ = std::max(1, static_cast<int>((clamped * 0.001f) * sampleRate_));
  }

  void setTarget(float value) {
    // Recompute from the current value so repeated knob moves do not create a discontinuity.
    target_ = value;
    remaining_ = rampSamples_;
    step_ = (target_ - current_) / static_cast<float>(remaining_);
  }

  float next() {
    if (remaining_ <= 0) {
      current_ = target_;
      return current_;
    }
    current_ += step_;
    --remaining_;
    if (remaining_ == 0) {
      current_ = target_;
    }
    return current_;
  }

  [[nodiscard]] float current() const { return current_; }
  [[nodiscard]] float target() const { return target_; }
  [[nodiscard]] bool isSmoothing() const { return remaining_ > 0; }

 private:
  float sampleRate_ = kDefaultSampleRate;
  float current_ = 0.0f;
  float target_ = 0.0f;
  float step_ = 0.0f;
  int rampSamples_ = 240;
  int remaining_ = 0;
};

}  // namespace rpdsp
