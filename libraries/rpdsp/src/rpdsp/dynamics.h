#pragma once

#include "algorithm.h"

#include <cmath>

namespace rpdsp {

class EnvelopeFollower {
 public:
  void prepare(float sampleRate) {
    sampleRate_ = safeSampleRate(sampleRate);
    setAttackRelease(attackMs_, releaseMs_);
  }

  void reset(float value = 0.0f) { envelope_ = std::max(0.0f, value); }

  void setAttackRelease(float attackMs, float releaseMs) {
    attackMs_ = std::max(0.001f, attackMs);
    releaseMs_ = std::max(0.001f, releaseMs);
    attackCoeff_ = onePoleSmooth(attackMs_, sampleRate_);
    releaseCoeff_ = onePoleSmooth(releaseMs_, sampleRate_);
  }

  float process(float input) {
    const float x = std::fabs(input);
    // Separate coefficients let peaks rise quickly while tails decay musically.
    const float coeff = x > envelope_ ? attackCoeff_ : releaseCoeff_;
    envelope_ = ((1.0f - coeff) * x) + (coeff * envelope_);
    return envelope_;
  }

  [[nodiscard]] float value() const { return envelope_; }

 private:
  float sampleRate_ = kDefaultSampleRate;
  float attackMs_ = 5.0f;
  float releaseMs_ = 100.0f;
  float attackCoeff_ = 0.0f;
  float releaseCoeff_ = 0.0f;
  float envelope_ = 0.0f;
};

class CompressorStaticCurve {
 public:
  void setThresholdDb(float db) { thresholdDb_ = db; }
  void setRatio(float ratio) { ratio_ = std::max(1.0f, ratio); }
  void setKneeWidthDb(float db) { kneeWidthDb_ = std::max(0.0f, db); }

  [[nodiscard]] float thresholdDb() const { return thresholdDb_; }
  [[nodiscard]] float ratio() const { return ratio_; }
  [[nodiscard]] float kneeWidthDb() const { return kneeWidthDb_; }

  [[nodiscard]] float gainReductionDb(float inputDb) const {
    if (ratio_ <= 1.0f) {
      return 0.0f;
    }

    if (kneeWidthDb_ <= 0.0f) {
      return hardKneeGainReductionDb(inputDb);
    }

    const float kneeStart = thresholdDb_ - (0.5f * kneeWidthDb_);
    const float kneeEnd = thresholdDb_ + (0.5f * kneeWidthDb_);
    if (inputDb <= kneeStart) {
      return 0.0f;
    }
    if (inputDb >= kneeEnd) {
      return hardKneeGainReductionDb(inputDb);
    }

    const float kneePosition = inputDb - kneeStart;
    // Quadratic knee eases into compression without a slope discontinuity at threshold.
    return ((1.0f / ratio_) - 1.0f) * kneePosition * kneePosition / (2.0f * kneeWidthDb_);
  }

 private:
  [[nodiscard]] float hardKneeGainReductionDb(float inputDb) const {
    if (inputDb <= thresholdDb_) {
      return 0.0f;
    }
    const float compressedDb = thresholdDb_ + ((inputDb - thresholdDb_) / ratio_);
    return compressedDb - inputDb;
  }

  float thresholdDb_ = -18.0f;
  float ratio_ = 2.0f;
  float kneeWidthDb_ = 0.0f;
};

class CompressorDetector {
 public:
  void prepare(float sampleRate) {
    sampleRate_ = safeSampleRate(sampleRate);
    setAttackRelease(attackMs_, releaseMs_);
  }

  void reset(float value = 0.0f) { level_ = std::max(0.0f, value); }

  void setAttackRelease(float attackMs, float releaseMs) {
    attackMs_ = std::max(0.001f, attackMs);
    releaseMs_ = std::max(0.001f, releaseMs);
    attackCoeff_ = onePoleSmooth(attackMs_, sampleRate_);
    releaseCoeff_ = onePoleSmooth(releaseMs_, sampleRate_);
  }

  float process(float input) {
    const float x = std::fabs(input);
    const float coeff = x > level_ ? attackCoeff_ : releaseCoeff_;
    level_ = ((1.0f - coeff) * x) + (coeff * level_);
    return level_;
  }

  [[nodiscard]] float value() const { return level_; }

 private:
  float sampleRate_ = kDefaultSampleRate;
  float attackMs_ = 5.0f;
  float releaseMs_ = 100.0f;
  float attackCoeff_ = 0.0f;
  float releaseCoeff_ = 0.0f;
  float level_ = 0.0f;
};

class GainReductionSmoother {
 public:
  void prepare(float sampleRate) {
    sampleRate_ = safeSampleRate(sampleRate);
    setAttackRelease(attackMs_, releaseMs_);
  }

  void reset(float valueDb = 0.0f) { gainReductionDb_ = std::min(0.0f, valueDb); }

  void setAttackRelease(float attackMs, float releaseMs) {
    attackMs_ = std::max(0.001f, attackMs);
    releaseMs_ = std::max(0.001f, releaseMs);
    attackCoeff_ = onePoleSmooth(attackMs_, sampleRate_);
    releaseCoeff_ = onePoleSmooth(releaseMs_, sampleRate_);
  }

  float process(float targetGainReductionDb) {
    const float target = std::min(0.0f, targetGainReductionDb);
    // More negative dB is stronger compression, so attack follows downward movement.
    const float coeff = target < gainReductionDb_ ? attackCoeff_ : releaseCoeff_;
    gainReductionDb_ = ((1.0f - coeff) * target) + (coeff * gainReductionDb_);
    return gainReductionDb_;
  }

  [[nodiscard]] float valueDb() const { return gainReductionDb_; }

 private:
  float sampleRate_ = kDefaultSampleRate;
  float attackMs_ = 5.0f;
  float releaseMs_ = 100.0f;
  float attackCoeff_ = 0.0f;
  float releaseCoeff_ = 0.0f;
  float gainReductionDb_ = 0.0f;
};

class Compressor {
 public:
  void prepare(float sampleRate) {
    sampleRate_ = safeSampleRate(sampleRate);
    detector_.prepare(sampleRate_);
    gainSmoother_.prepare(sampleRate_);
  }

  void reset() {
    detector_.reset();
    gainSmoother_.reset();
  }
  void setThresholdDb(float db) { curve_.setThresholdDb(db); }
  void setRatio(float ratio) { curve_.setRatio(ratio); }
  void setKneeWidthDb(float db) { curve_.setKneeWidthDb(db); }
  void setMakeupGainDb(float db) { makeupGainDb_ = db; }
  void setAttackRelease(float attackMs, float releaseMs) {
    detector_.setAttackRelease(attackMs, releaseMs);
    gainSmoother_.setAttackRelease(attackMs, releaseMs);
  }

  float process(float input) {
    const float level = detector_.process(input);
    const float inputDb = gainToDb(level);
    const float targetGainReductionDb = curve_.gainReductionDb(inputDb);
    // Smooth gain, not audio, to avoid pumping from per-sample detector jitter.
    const float smoothedGainReductionDb = gainSmoother_.process(targetGainReductionDb);
    return input * dbToGain(smoothedGainReductionDb + makeupGainDb_);
  }

 private:
  float sampleRate_ = kDefaultSampleRate;
  float makeupGainDb_ = 0.0f;
  CompressorStaticCurve curve_;
  CompressorDetector detector_;
  GainReductionSmoother gainSmoother_;
};

}  // namespace rpdsp
