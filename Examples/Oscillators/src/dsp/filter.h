#pragma once

#include "algorithm.h"
#include "realtime.h"

#include <cmath>

namespace rpdsp {

class OnePoleLowpass {
 public:
  void prepare(float sampleRate) {
    sampleRate_ = safeSampleRate(sampleRate);
    setCutoff(cutoffHz_);
  }

  void reset(float value = 0.0f) { z1_ = value; }

  void setCutoff(float cutoffHz) {
    cutoffHz_ = clampCutoff(cutoffHz, sampleRate_);
    // Exact one-pole coefficient keeps the cutoff stable when sample rate changes.
    const float x = std::exp(-kTwoPi * cutoffHz_ / sampleRate_);
    a_ = x;
    b_ = 1.0f - x;
  }

  float process(float input) {
    z1_ = zapDenormal((b_ * input) + (a_ * z1_));
    return z1_;
  }

 private:
  float sampleRate_ = kDefaultSampleRate;
  float cutoffHz_ = 1000.0f;
  float a_ = 0.0f;
  float b_ = 1.0f;
  float z1_ = 0.0f;
};

class DcBlocker {
 public:
  void prepare(float sampleRate) {
    sampleRate_ = safeSampleRate(sampleRate);
    setCutoff(cutoffHz_);
  }

  void reset(float input = 0.0f) {
    x1_ = input;
    y1_ = 0.0f;
  }

  void setCutoff(float cutoffHz) {
    cutoffHz_ = clampCutoff(cutoffHz, sampleRate_);
    coefficient_ = std::exp(-kTwoPi * cutoffHz_ / sampleRate_);
  }

  float process(float input) {
    // High-pass the difference between current and previous input while preserving bass above cutoff.
    const float out = zapDenormal(input - x1_ + coefficient_ * y1_);
    x1_ = input;
    y1_ = out;
    return out;
  }

 private:
  float sampleRate_ = kDefaultSampleRate;
  float cutoffHz_ = 20.0f;
  float coefficient_ = 0.997f;
  float x1_ = 0.0f;
  float y1_ = 0.0f;
};

class BiquadLowpass {
 public:
  void prepare(float sampleRate) {
    sampleRate_ = safeSampleRate(sampleRate);
    update();
  }

  void reset(float value = 0.0f) {
    z1_ = value;
    z2_ = value;
  }

  void setCutoff(float cutoffHz) {
    cutoffHz_ = clampCutoff(cutoffHz, sampleRate_);
    update();
  }

  void setQ(float q) {
    q_ = clamp(q, 0.1f, 20.0f);
    update();
  }

  float process(float input) {
    // Transposed direct form II uses two states and behaves well for per-sample processing.
    const float out = (b0_ * input) + z1_;
    z1_ = (b1_ * input) - (a1_ * out) + z2_;
    z2_ = (b2_ * input) - (a2_ * out);
    return zapDenormal(out);
  }

 private:
  void update() {
    // RBJ cookbook lowpass coefficients, normalized by a0 for the realtime loop.
    const float w0 = kTwoPi * clampCutoff(cutoffHz_, sampleRate_) / sampleRate_;
    const float cosW0 = std::cos(w0);
    const float alpha = std::sin(w0) / (2.0f * q_);
    const float a0 = 1.0f + alpha;
    b0_ = ((1.0f - cosW0) * 0.5f) / a0;
    b1_ = (1.0f - cosW0) / a0;
    b2_ = b0_;
    a1_ = (-2.0f * cosW0) / a0;
    a2_ = (1.0f - alpha) / a0;
  }

  float sampleRate_ = kDefaultSampleRate;
  float cutoffHz_ = 1000.0f;
  float q_ = 0.70710678f;
  float b0_ = 1.0f;
  float b1_ = 0.0f;
  float b2_ = 0.0f;
  float a1_ = 0.0f;
  float a2_ = 0.0f;
  float z1_ = 0.0f;
  float z2_ = 0.0f;
};

struct StateVariableOutput {
  float lowpass = 0.0f;
  float bandpass = 0.0f;
  float highpass = 0.0f;
};

class StateVariableFilter {
 public:
  void prepare(float sampleRate) {
    sampleRate_ = safeSampleRate(sampleRate);
    update();
  }

  void reset() {
    ic1eq_ = 0.0f;
    ic2eq_ = 0.0f;
  }

  void setCutoff(float cutoffHz) {
    cutoffHz_ = clampCutoff(cutoffHz, sampleRate_);
    update();
  }

  void setResonance(float resonance) {
    resonance_ = clamp(resonance, 0.0f, 0.98f);
    update();
  }

  void setCutoffResonance(float cutoffHz, float resonance) {
    cutoffHz_ = clampCutoff(cutoffHz, sampleRate_);
    resonance_ = clamp(resonance, 0.0f, 0.98f);
    update();
  }

  StateVariableOutput process(float input) {
    // TPT SVF gives simultaneous outputs and tolerates cutoff modulation better than a naive SVF.
    const float v3 = input - ic2eq_;
    const float v1 = a1_ * ic1eq_ + a2_ * v3;
    const float v2 = ic2eq_ + a2_ * ic1eq_ + a3_ * v3;
    ic1eq_ = zapDenormal((2.0f * v1) - ic1eq_);
    ic2eq_ = zapDenormal((2.0f * v2) - ic2eq_);
    return {v2, v1, input - k_ * v1 - v2};
  }

 private:
  void update() {
    // Map resonance onto the damping term while leaving headroom before self-oscillation.
    g_ = std::tan(kPi * clampCutoff(cutoffHz_, sampleRate_) / sampleRate_);
    k_ = 2.0f - (1.9f * resonance_);
    a1_ = 1.0f / (1.0f + g_ * (g_ + k_));
    a2_ = g_ * a1_;
    a3_ = g_ * a2_;
  }

  float sampleRate_ = kDefaultSampleRate;
  float cutoffHz_ = 1000.0f;
  float resonance_ = 0.0f;
  float g_ = 0.0f;
  float k_ = 2.0f;
  float a1_ = 1.0f;
  float a2_ = 0.0f;
  float a3_ = 0.0f;
  float ic1eq_ = 0.0f;
  float ic2eq_ = 0.0f;
};

}  // namespace rpdsp
