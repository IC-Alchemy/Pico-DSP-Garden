#pragma once

#include "algorithm.h"
#include "filter.h"
#include "oscillator.h"
#include "realtime.h"

#include <algorithm>
#include <cmath>

namespace rpdsp {

/** @brief Hypersaw ("Super Saw") oscillator built on rpdsp primitives.
 *
 * Seven detuned band-limited sawtooth oscillators after the Roland JP-8000
 * Super Saw, based on Adam Szabo's "How to Emulate the Super Saw" research:
 * one center oscillator plus six detuned side oscillators.
 *
 *   - Non-linear (x^4) detune curve for authentic frequency spreading.
 *   - Mix control that balances center vs. side voices using the paper's
 *     gain curves, so mix actually changes the timbre.
 *   - A pitch-tracked StateVariableFilter run as a high-pass to shape the
 *     spectrum like the original.
 *   - Free-running voices with randomized phase on trigger().
 *
 * Each voice is a SecondOrderBSplineSawOscillator, the rpdsp band-limited
 * saw (the equivalent of a POLYBLEP saw), so there is no separate waveform
 * enum to set.
 */
class Hypersaw {
 public:
  static constexpr int kVoiceCount = 7;
  static constexpr int kCenterIndex = 3;
  static constexpr int kSideCount = 6;

  void prepare(float sampleRate) {
    sampleRate_ = safeSampleRate(sampleRate);
    for (int i = 0; i < kVoiceCount; ++i) {
      voices_[i].prepare(sampleRate_);
    }
    hpf_.prepare(sampleRate_);
    hpf_.setResonance(0.1f);  // low resonance: spectral shaping, not resonance
    trigger();                // randomized free-running phases
    updateCoefficients();
  }

  void reset() {
    for (int i = 0; i < kVoiceCount; ++i) {
      voices_[i].reset(0.0f);
    }
    hpf_.reset();
  }

  /** Randomize each voice's phase to simulate a fresh note trigger. */
  void trigger() {
    for (int i = 0; i < kVoiceCount; ++i) {
      voices_[i].reset((rng_.nextBipolar() * 0.5f) + 0.5f);
    }
  }

  /** Seed the internal PRNG so multiple Hypersaw instances decorrelate.
   *
   * The default seed is fixed, so two Hypersaws constructed without a reseed
   * produce identical phase spreads on every trigger(). Call reseed() once
   * per instance with a distinct non-zero value (e.g. derived from an analog
   * pin or a global counter) to avoid the voices lining up. */
  void reseed(std::uint32_t seed) {
    rng_ = XorShift32(seed);
  }

  void setFreq(float freq) {
    freq_ = std::max(0.0f, freq);
    updateCoefficients();
  }

  /** Detune amount in [0, 1]; passed through a non-linear curve internally. */
  void setDetune(float detune) {
    detune_ = clamp(detune, 0.0f, 1.0f);
    updateCoefficients();
  }

  /** Mix in [0, 1]: balances the center voice against the side voices. */
  void setMix(float mix) {
    mix_ = clamp(mix, 0.0f, 1.0f);
    updateCoefficients();
  }

  float process() {
    // Advance every voice every sample so their phases stay free-running.
    float sideSum = 0.0f;
    for (int i = 0; i < kSideCount; ++i) {
      sideSum += voices_[sideIndices_[i]].process();
    }
    const float center = voices_[kCenterIndex].process();

    const float sum = (sideSum * sideGain_) + (center * centerGain_);
    return hpf_.process(sum / 4.5f).highpass;
  }

 private:
  void updateCoefficients() {
    // Gains from the paper: center falls linearly, sides follow a parabola.
    centerGain_ = -0.55366f * mix_ + 0.99785f;
    sideGain_ = -0.73764f * mix_ * mix_ + 1.2841f * mix_ + 0.044372f;

    // Non-linear detune (x^4) for the authentic spreading curve.
    const float scaledDetune =
        clamp(detune_ * detune_ * detune_ * detune_, 0.0f, 1.0f);

    voices_[kCenterIndex].setFreq(freq_);
    for (int i = 0; i < kSideCount; ++i) {
      const float detuneFactor = 1.0f + scaledDetune * detuneRatios_[i];
      voices_[sideIndices_[i]].setFreq(freq_ * detuneFactor);
    }

    // Pitch-track the high-pass to the fundamental.
    hpf_.setCutoff(freq_);
  }

  // Side voice indices within the 7-voice array (center sits at index 3).
  static constexpr int sideIndices_[kSideCount] = {0, 1, 2, 4, 5, 6};
  // Detune ratios for the six side voices relative to the center.
  static constexpr float detuneRatios_[kSideCount] = {
      -0.11002313f, -0.06288439f, -0.01952356f,
       0.01991221f,  0.06216538f,  0.10745242f};

  float sampleRate_ = kDefaultSampleRate;
  float freq_ = 100.0f;
  float detune_ = 0.5f;
  float mix_ = 0.5f;
  float sideGain_ = 0.5f;
  float centerGain_ = 1.0f;

  SecondOrderBSplineSawOscillator voices_[kVoiceCount];
  StateVariableFilter hpf_;
  XorShift32 rng_;
};

}  // namespace rpdsp

