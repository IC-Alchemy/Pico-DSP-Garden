#pragma once

#include "config.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace rpdsp {

// Small inline helpers keep coefficient setup readable without pulling policy into DSP classes.

// Saturate control-domain values before they reach DSP coefficients.
inline float clamp(float value, float low, float high) {
  return std::max(low, std::min(value, high));
}

// Normalized modulation, mix, and envelope values live on [0, 1].
inline float clamp01(float value) {
  return clamp(value, 0.0f, 1.0f);
}

// Convert a [-1, 1] float to a 24-in-32 left-justified int32 word for I2S output.
// Bits 31..8 hold the 24 audio bits; bits 7..0 are zero (the DAC clocks them out
// but ignores them). Branch-free and allocation-free; safe for the realtime
// audio callback. Scale is 2^23 - 1 so +1.0 maps to positive full-scale
// (0x7FFFFF00) without overflowing the 24-bit range. Truncates toward zero; the
// ~0.5 LSB error is inaudible at 24-bit and avoids a libc roundf() call.
inline int32_t toInt24x32(float sample) {
  const float clamped = clamp(sample, -1.0f, 1.0f);
  const int32_t s = static_cast<int32_t>(clamped * 8388607.0f);  // 2^23 - 1
  // Shift in the unsigned domain: left-shifting a negative signed value is UB
  // before C++20, and the bit pattern is identical either way.
  return static_cast<int32_t>(static_cast<uint32_t>(s) << 8);
}

// Linear interpolation; pass a clamped t when overshoot is not desired.
inline float lerp(float a, float b, float t) {
  return a + (b - a) * t;
}

// Wrap a normalized phase accumulator without accumulating drift.
inline float wrap01(float value) {
  value -= std::floor(value);
  return value >= 1.0f ? 0.0f : value;
}

// Decibels are amplitude ratios here, so use the 20 dB decade.
inline float dbToGain(float db) {
  return std::pow(10.0f, db / 20.0f);
}

// Keep log10 away from zero; silence maps to a finite floor.
inline float gainToDb(float gain) {
  constexpr float kMinGain = 1.0e-12f;
  return 20.0f * std::log10(std::max(gain, kMinGain));
}

// MIDI note 69 is A4 at 440 Hz in 12-TET.
inline float midiNoteToHz(float note) {
  return 440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f);
}

// Bad host or bring-up sample rates fall back to the library default.
inline float safeSampleRate(float sampleRate) {
  return sampleRate > 1.0f ? sampleRate : kDefaultSampleRate;
}

// Leave a little headroom below Nyquist for coefficient stability.
// Caller must ensure sampleRate is valid (validated once in prepare()).
inline float clampCutoff(float cutoff, float sampleRate) {
  const float nyquist = 0.5f * sampleRate;
  return clamp(cutoff, 1.0f, nyquist * 0.99f);
}

// One-pole smoothing coefficient from a time constant in milliseconds.
// Caller must ensure sampleRate is valid (validated once in prepare()).
inline float onePoleSmooth(float milliseconds, float sampleRate) {
  const float ms = std::max(milliseconds, 0.001f);
  return std::exp(-1.0f / (ms * 0.001f * sampleRate));
}

// Cheap, monotonic saturation for taming peaks without hard clipping.
inline float softClip(float value) {
  return value / (1.0f + std::fabs(value));
}

// Cheap rational tanh approximation; saturates to +/-1 for |x| > 3.
// For saturators/clippers in the DSP path where std::tanh is too costly.
inline float fastTanh(float x) {
  if (x > 3.0f) return 1.0f;
  if (x < -3.0f) return -1.0f;
  const float x2 = x * x;
  return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// Equal-power pan keeps perceived loudness steadier through center.
inline float equalPowerPanLeft(float pan) {
  return std::cos(clamp01((pan + 1.0f) * 0.5f) * kPi * 0.5f);
}

// Companion gain for equalPowerPanLeft; pan is expected in [-1, 1].
inline float equalPowerPanRight(float pan) {
  return std::sin(clamp01((pan + 1.0f) * 0.5f) * kPi * 0.5f);
}

}  // namespace rpdsp
