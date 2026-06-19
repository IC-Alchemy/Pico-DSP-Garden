#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace rpdsp {

// Fixed-size step mask sequencer for trigger/gate patterns.
template <size_t MaxSteps = 32>
class GatePattern {
 public:
  // Clears every step and rewinds the playhead; length optionally resizes the active prefix.
  void reset(size_t length = MaxSteps) {
    length_ = clampLength(length);
    step_ = 0;
    steps_.fill(false);
  }

  void setLength(size_t length) { length_ = clampLength(length); }
  [[nodiscard]] size_t length() const { return length_; }
  [[nodiscard]] size_t step() const { return step_; }

  void setStep(size_t step, bool enabled) {
    if (step < MaxSteps)
      steps_[step] = enabled;
  }

  [[nodiscard]] bool getStep(size_t step) const {
    return step < MaxSteps ? steps_[step] : false;
  }

  void loadMask(std::uint32_t mask, size_t length = MaxSteps) {
    // Bit 0 maps to step 0. Steps outside the active length are still loaded
    // so changing length later preserves the mask contents.
    setLength(length);
    for (size_t i = 0; i < MaxSteps; ++i)
      steps_[i] = (mask & (std::uint32_t{1} << i)) != 0;
  }

  // Returns the current step's gate and advances on a trigger edge.
  bool processClock(bool trigger) {
    if (!trigger)
      return false;

    // Return the current step's gate before advancing, so reset/step changes
    // take effect on the next trigger edge.
    const bool gate = steps_[step_];
    ++step_;
    if (step_ >= length_)
      step_ = 0;
    return gate;
  }

  // Rewind the playhead without clearing the pattern.
  void resetStep(size_t step = 0) {
    step_ = step < length_ ? step : 0;
  }

 private:
  size_t clampLength(size_t length) const {
    if (length < 1)
      return 1;
    return length > MaxSteps ? MaxSteps : length;
  }

  std::array<bool, MaxSteps> steps_{};
  size_t length_ = MaxSteps;
  size_t step_ = 0;
};

}  // namespace rpdsp
