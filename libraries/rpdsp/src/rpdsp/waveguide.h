#pragma once

#include "algorithm.h"
#include "delay_line.h"
#include "realtime.h"

#include <array>
#include <cstddef>

namespace rpdsp {

template <size_t Capacity>
class KarplusStrongVoice {
 public:
  void prepare(float sampleRate) {
    sampleRate_ = safeSampleRate(sampleRate);
    reset();
  }

  void reset() {
    delay_.reset();
    periodSamples_ = Capacity / 2;
    last_ = 0.0f;
    active_ = false;
  }

  void setDecay(float decay) { decay_ = clamp(decay, 0.0f, 0.9999f); }

  void pluck(float frequencyHz, float amplitude = 1.0f) {
    const float freq = std::max(1.0f, frequencyHz);
    // Period length sets pitch; clamp to the fixed delay so plucks cannot allocate or overrun.
    periodSamples_ = static_cast<size_t>(clamp(sampleRate_ / freq, 2.0f, static_cast<float>(Capacity - 1)));
    const float amp = std::max(0.0f, amplitude);
    delay_.reset();
    // Initial noise is the classic string excitation; the feedback loop turns it into pitch.
    for (size_t i = 0; i < periodSamples_; ++i) {
      delay_.push(rng_.nextBipolar() * amp);
    }
    last_ = 0.0f;
    active_ = true;
  }

  float process() {
    if (!active_) {
      return 0.0f;
    }
    const float current = delay_.read(periodSamples_ - 1);
    // Two-point averaging is the string loss filter; decay controls how quickly energy dies.
    const float next = decay_ * 0.5f * (current + last_);
    delay_.push(next);
    last_ = current;
    if (std::fabs(next) < 1.0e-5f && std::fabs(current) < 1.0e-5f) {
      active_ = false;
    }
    return current;
  }

  [[nodiscard]] bool isActive() const { return active_; }

 private:
  float sampleRate_ = kDefaultSampleRate;
  float decay_ = 0.996f;
  float last_ = 0.0f;
  bool active_ = false;
  size_t periodSamples_ = Capacity / 2;
  DelayLine<Capacity> delay_;
  XorShift32 rng_{0xA511E9B3u};
};

}  // namespace rpdsp
