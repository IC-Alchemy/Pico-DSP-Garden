#pragma once

#include "algorithm.h"

#include <cstddef>

namespace rpdsp {

// Reads gate rows from an external, interleaved pattern table (row-major:
// patterns[pattern * MaxSteps + step]). The caller owns the storage so large
// pattern banks stay out of fixed template memory.
template <size_t MaxSteps = 32>
class RhythmGateSequencer {
 public:
  void reset(const bool* patterns, size_t patternCount, size_t stepCount = MaxSteps) {
    patterns_ = patterns;
    patternCount_ = patternCount;
    stepCount_ = clampStepCount(stepCount);
  }

  // Maps a normalized [0, 1] selector to a valid pattern index.
  [[nodiscard]] size_t selectPattern(float normalized) const {
    if (patternCount_ <= 1)
      return 0;

    normalized = clamp01(normalized);
    return clampPattern(static_cast<size_t>(normalized * static_cast<float>(patternCount_ - 1) + 0.5f));
  }

  // pattern is a non-negative selector; step may be negative to wrap backwards.
  [[nodiscard]] bool gate(size_t pattern, int step) const {
    if (patterns_ == nullptr || patternCount_ == 0)
      return false;

    const size_t safePattern = clampPattern(pattern);
    const size_t safeStep = wrapStep(step);
    return patterns_[safePattern * MaxSteps + safeStep];
  }

  [[nodiscard]] size_t wrapStep(int step) const {
    if (stepCount_ == 0)
      return 0;

    while (step < 0)
      step += static_cast<int>(stepCount_);
    return static_cast<size_t>(step) % stepCount_;
  }

  [[nodiscard]] size_t patternCount() const { return patternCount_; }
  [[nodiscard]] size_t stepCount() const { return stepCount_; }

 private:
  size_t clampPattern(size_t pattern) const {
    if (patternCount_ == 0)
      return 0;
    return pattern >= patternCount_ ? patternCount_ - 1 : pattern;
  }

  size_t clampStepCount(size_t stepCount) const {
    if (stepCount < 1)
      return 1;
    return stepCount > MaxSteps ? MaxSteps : stepCount;
  }

  const bool* patterns_ = nullptr;
  size_t patternCount_ = 0;
  size_t stepCount_ = MaxSteps;
};

}  // namespace rpdsp
