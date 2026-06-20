#pragma once

#include "algorithm.h"

#include <array>
#include <cmath>
#include <cstddef>

namespace rpdsp {

class ZeroCrossingPitchDetector {
 public:
  void prepare(float sampleRate) {
    sampleRate_ = sampleRate > 1.0f ? sampleRate : kDefaultSampleRate;
  }

  void reset() {
    sampleIndex_ = 0;
    lastCrossing_ = 0;
    crossings_ = 0;
    averagePeriod_ = 0.0f;
    lastSign_ = false;
    hasLastSign_ = false;
  }

  void process(float sample) {
    const bool sign = sample >= 0.0f;
    if (hasLastSign_ && !lastSign_ && sign) {
      // Use only rising zero crossings so each cycle contributes one period estimate.
      if (crossings_ > 0) {
        const size_t period = sampleIndex_ - lastCrossing_;
        // A light average makes the display usable without hiding pitch changes for long.
        averagePeriod_ = averagePeriod_ <= 0.0f ? static_cast<float>(period)
                                                : (0.8f * averagePeriod_) + (0.2f * static_cast<float>(period));
      }
      lastCrossing_ = sampleIndex_;
      ++crossings_;
    }
    lastSign_ = sign;
    hasLastSign_ = true;
    ++sampleIndex_;
  }

  [[nodiscard]] float frequencyHz() const {
    return averagePeriod_ > 0.0f ? sampleRate_ / averagePeriod_ : 0.0f;
  }

 private:
  float sampleRate_ = kDefaultSampleRate;
  size_t sampleIndex_ = 0;
  size_t lastCrossing_ = 0;
  size_t crossings_ = 0;
  float averagePeriod_ = 0.0f;
  bool lastSign_ = false;
  bool hasLastSign_ = false;
};

template <size_t WindowSize, size_t MaxTau>
class YinPitchDetector {
 public:
  // YIN is more reliable than zero crossings for real instruments, at the cost of a windowed analysis pass.
  static_assert(MaxTau >= 3, "MaxTau must leave room for interpolation");
  static_assert(WindowSize > MaxTau + 1, "WindowSize must be larger than MaxTau");

  void prepare(float sampleRate) {
    sampleRate_ = sampleRate > 1.0f ? sampleRate : kDefaultSampleRate;
    setFreqRange(minFrequencyHz_, maxFrequencyHz_);
    reset();
  }

  void reset() {
    history_.fill(0.0f);
    difference_.fill(1.0f);
    writeIndex_ = 0;
    filled_ = 0;
    samplesUntilAnalysis_ = updateIntervalSamples_;
    frequencyHz_ = 0.0f;
    confidence_ = 0.0f;
    hasPitch_ = false;
  }

  void setFreqRange(float minFrequencyHz, float maxFrequencyHz) {
    minFrequencyHz_ = std::max(1.0f, minFrequencyHz);
    maxFrequencyHz_ = std::max(minFrequencyHz_ + 1.0f, maxFrequencyHz);

    // Tau is period in samples: high frequencies use short lags, low frequencies use long lags.
    const size_t minTau = static_cast<size_t>(std::ceil(sampleRate_ / maxFrequencyHz_));
    const size_t maxTau = static_cast<size_t>(sampleRate_ / minFrequencyHz_);
    minTau_ = std::max<size_t>(2, std::min(minTau, MaxTau - 2));
    maxTau_ = std::max(minTau_ + 1, std::min<size_t>(maxTau, MaxTau));
  }

  void setThreshold(float threshold) { threshold_ = clamp(threshold, 0.02f, 0.95f); }

  void setUpdateIntervalSamples(size_t samples) {
    updateIntervalSamples_ = std::max<size_t>(1, samples);
    samplesUntilAnalysis_ = updateIntervalSamples_;
  }

  void process(float sample) {
    history_[writeIndex_] = sample;
    writeIndex_ = (writeIndex_ + 1) % WindowSize;
    filled_ = std::min(filled_ + 1, WindowSize);

    // Analyze less often than every sample so pitch tracking can coexist with realtime audio.
    if (samplesUntilAnalysis_ > 0) {
      --samplesUntilAnalysis_;
    }
    if (samplesUntilAnalysis_ == 0) {
      samplesUntilAnalysis_ = updateIntervalSamples_;
      if (filled_ == WindowSize) {
        analyze();
      }
    }
  }

  [[nodiscard]] bool hasPitch() const { return hasPitch_; }

  [[nodiscard]] float frequencyHz() const { return hasPitch_ ? frequencyHz_ : 0.0f; }

  [[nodiscard]] float confidence() const { return confidence_; }

 private:
  [[nodiscard]] float sampleAt(size_t chronologicalIndex) const {
    return history_[(writeIndex_ + chronologicalIndex) % WindowSize];
  }

  void analyze() {
    difference_[0] = 0.0f;
    // Difference is small when the signal lines up with itself at lag tau.
    for (size_t tau = 1; tau <= maxTau_; ++tau) {
      float sum = 0.0f;
      for (size_t i = 0; i + tau < WindowSize; ++i) {
        const float delta = sampleAt(i) - sampleAt(i + tau);
        sum += delta * delta;
      }
      difference_[tau] = sum;
    }

    float runningSum = 0.0f;
    // Cumulative mean normalization prevents short lags from winning only because they compare fewer samples.
    for (size_t tau = 1; tau <= maxTau_; ++tau) {
      runningSum += difference_[tau];
      difference_[tau] = runningSum > 0.0f ? difference_[tau] * static_cast<float>(tau) / runningSum : 1.0f;
    }

    size_t estimateTau = 0;
    float estimateValue = 1.0f;
    for (size_t tau = minTau_; tau <= maxTau_; ++tau) {
      if (difference_[tau] < threshold_) {
        // Once below threshold, keep walking downhill to the local minimum for a cleaner estimate.
        while (tau + 1 <= maxTau_ && difference_[tau + 1] < difference_[tau]) {
          ++tau;
        }
        estimateTau = tau;
        estimateValue = difference_[tau];
        break;
      }
      if (difference_[tau] < estimateValue) {
        estimateTau = tau;
        estimateValue = difference_[tau];
      }
    }

    if (estimateTau == 0 || estimateValue >= threshold_) {
      hasPitch_ = false;
      confidence_ = 0.0f;
      frequencyHz_ = 0.0f;
      return;
    }

    const float refinedTau = parabolicTau(estimateTau);
    frequencyHz_ = refinedTau > 0.0f ? sampleRate_ / refinedTau : 0.0f;
    confidence_ = clamp01(1.0f - estimateValue);
    hasPitch_ = frequencyHz_ >= minFrequencyHz_ && frequencyHz_ <= maxFrequencyHz_;
    if (!hasPitch_) {
      confidence_ = 0.0f;
      frequencyHz_ = 0.0f;
    }
  }

  [[nodiscard]] float parabolicTau(size_t tau) const {
    if (tau <= minTau_ || tau >= maxTau_) {
      return static_cast<float>(tau);
    }

    const float left = difference_[tau - 1];
    const float center = difference_[tau];
    const float right = difference_[tau + 1];
    const float denominator = left - (2.0f * center) + right;
    if (std::fabs(denominator) < 1.0e-12f) {
      return static_cast<float>(tau);
    }
    // Sub-sample lag interpolation reduces pitch zippering between adjacent integer periods.
    return static_cast<float>(tau) + 0.5f * (left - right) / denominator;
  }

  std::array<float, WindowSize> history_{};
  std::array<float, MaxTau + 1> difference_{};
  float sampleRate_ = kDefaultSampleRate;
  float minFrequencyHz_ = 50.0f;
  float maxFrequencyHz_ = 2000.0f;
  float threshold_ = 0.15f;
  float frequencyHz_ = 0.0f;
  float confidence_ = 0.0f;
  size_t minTau_ = 2;
  size_t maxTau_ = MaxTau;
  size_t updateIntervalSamples_ = kDefaultBlockSize * 4;
  size_t samplesUntilAnalysis_ = kDefaultBlockSize * 4;
  size_t writeIndex_ = 0;
  size_t filled_ = 0;
  bool hasPitch_ = false;
};

class RmsPeakMeter {
 public:
  void reset() {
    sumSquares_ = 0.0f;
    peak_ = 0.0f;
    count_ = 0;
  }

  void process(float sample) {
    // Accumulate both energy and peak so metering can show loudness and clipping risk.
    const float absValue = std::fabs(sample);
    peak_ = std::max(peak_, absValue);
    sumSquares_ += sample * sample;
    ++count_;
  }

  [[nodiscard]] float rms() const {
    return count_ > 0 ? std::sqrt(sumSquares_ / static_cast<float>(count_)) : 0.0f;
  }

  [[nodiscard]] float peak() const { return peak_; }

 private:
  float sumSquares_ = 0.0f;
  float peak_ = 0.0f;
  size_t count_ = 0;
};

}  // namespace rpdsp
