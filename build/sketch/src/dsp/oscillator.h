#line 1 "Z:\\Codezzz\\MusicCode\\Pico-DSP-Garden\\Examples\\SimpleOscillators\\src\\dsp\\oscillator.h"
#pragma once

#include "algorithm.h"
#include "realtime.h"

#include <cmath>
#include <cstdint>

// Two oscillator families live here. Read the first class, Phasor, to see the
// shared phase convention used by everything else.
//
// 1) Naive phasor oscillators: Phasor, Sine/Triangle/Saw/PulseOscillator.
//    These are the direct equivalents of the naive WAVE_SIN/TRI/SAW/SQUARE
//    modes in a DaisySP-style oscillator. A phase accumulator advances 0 -> 1
//    each cycle and the waveshape is read straight off the phase. Cheap, but
//    saw/square/pulse alias because their discontinuities are not band-limited.
//
// 2) Band-limited B-spline oscillators: the SecondOrderBSpline* classes below.
//    These are the anti-aliased counterparts to POLYBLEP modes, but they use a
//    different technique:
//
//      polyBLEP:    build the naive wave, then near each edge add a tiny
//                    polynomial correction that rounds off the corner.
//
//      B-spline:    treat the wave as the INTEGRAL of impulses (a saw is a unit
//                    impulse each cycle minus a constant slope; a pulse is
//                    alternating +/- impulses at its edges). Each impulse is
//                    smeared across 3 samples by a smooth B-spline kernel, and
//                    that smearing is what makes it band-limited. A leaky
//                    integrator then sums the smeared impulses back into the
//                    waveform.
//
//    SecondOrderBSplineEventBuffer is the smear step: a 3-tap delay that drops
//    each sub-sample-timed event into the kernel and shifts it out one sample
//    at a time. SecondOrderBSplineSawOscillator is the simplest example and a
//    good place to see the whole impulse -> smear -> integrate flow at once.
namespace rpdsp {

class Phasor {
 public:
  void prepare(float sampleRate) {
    sampleRate_ = safeSampleRate(sampleRate);
    updateIncrement();
  }

  void reset(float phase = 0.0f) { phase_ = wrap01(phase); }

  void setFreq(float frequencyHz) {
    frequencyHz_ = std::max(0.0f, frequencyHz);
    updateIncrement();
  }

  float process() {
    // Return the current phase before advancing so oscillators share the same phase convention.
    const float out = phase_;
    phase_ = wrap01(phase_ + increment_);
    return out;
  }

  [[nodiscard]] float phase() const { return phase_; }

 private:
  void updateIncrement() { increment_ = frequencyHz_ / sampleRate_; }

  float sampleRate_ = kDefaultSampleRate;
  float frequencyHz_ = 440.0f;
  float increment_ = 440.0f / kDefaultSampleRate;
  float phase_ = 0.0f;
};

class SineOscillator {
 public:
  void prepare(float sampleRate) { phasor_.prepare(sampleRate); }
  void reset(float phase = 0.0f) { phasor_.reset(phase); }
  void setFreq(float frequencyHz) { phasor_.setFreq(frequencyHz); }

  float process() { return std::sin(kTwoPi * phasor_.process()); }

 private:
  Phasor phasor_;
};

class TriangleOscillator {
 public:
  void prepare(float sampleRate) { phasor_.prepare(sampleRate); }
  void reset(float phase = 0.0f) { phasor_.reset(phase); }
  void setFreq(float frequencyHz) { phasor_.setFreq(frequencyHz); }

  float process() {
    const float p = phasor_.process();
    return 1.0f - (4.0f * std::fabs(p - 0.5f));
  }

 private:
  Phasor phasor_;
};

class SawOscillator {
 public:
  void prepare(float sampleRate) { phasor_.prepare(sampleRate); }
  void reset(float phase = 0.0f) { phasor_.reset(phase); }
  void setFreq(float frequencyHz) { phasor_.setFreq(frequencyHz); }

  float process() { return (2.0f * phasor_.process()) - 1.0f; }

 private:
  Phasor phasor_;
};

class PulseOscillator {
 public:
  void prepare(float sampleRate) { phasor_.prepare(sampleRate); }
  void reset(float phase = 0.0f) { phasor_.reset(phase); }
  void setFreq(float frequencyHz) { phasor_.setFreq(frequencyHz); }
  void setPWM(float width) { pulseWidth_ = clamp(width, 0.01f, 0.99f); }

  float process() { return phasor_.process() < pulseWidth_ ? 1.0f : -1.0f; }

 private:
  Phasor phasor_;
  float pulseWidth_ = 0.5f;
};

inline float secondOrderBSpline(float x) {
  // The standard 2nd-order (quadratic) uniform B-spline. It is a smooth,
  // bell-shaped kernel with 3-sample support used to smear a single impulse
  // across several samples; that smearing is what makes the event band-limited.
  // Because it is smooth (C1), its spectrum falls off faster than linear
  // interpolation, so it aliases less.
  const float ax = std::fabs(x);
  if (ax < 0.5f) {
    return 0.75f - (ax * ax);
  }
  if (ax < 1.5f) {
    const float edge = 1.5f - ax;
    return 0.5f * edge * edge;
  }
  return 0.0f;
}

// A 3-tap (plus one spare) delay line that performs the band-limiting "smear"
// step. An event scheduled at a sub-sample time within the current sample is
// spread across taps 0..2 by the B-spline kernel; process() then shifts the
// taps out one sample at a time so the smeared energy lands on the right audio
// samples.
class SecondOrderBSplineEventBuffer {
 public:
  void reset() {
    for (float& tap : taps_) {
      tap = 0.0f;
    }
  }

  float process() {
    // Output the energy due now, then shift the remaining taps one step closer.
    const float out = taps_[0];
    taps_[0] = taps_[1];
    taps_[1] = taps_[2];
    taps_[2] = taps_[3];
    taps_[3] = 0.0f;
    return out;
  }

  void addImpulse(float fraction, float amplitude) {
    // fraction is the sub-sample time of the event inside this sample period
    // (0 = right now, 1 = one sample later). The +0.5 centers the kernel peak
    // on the correct tap; the kernel then splits the impulse over taps 0..2.
    const float center = clamp(fraction, 0.0f, 1.0f) + 0.5f;
    for (int i = 0; i < 3; ++i) {
      const float sampleTime = static_cast<float>(i);
      taps_[i] += amplitude * secondOrderBSpline(sampleTime - center);
    }
  }

 private:
  float taps_[4] = {};
};

// Simplest band-limited oscillator, and the template for the pulse and
// hard-sync variants. A sawtooth is reconstructed as: one +1 impulse at each
// phase wrap (the rising edge), integrated against a constant negative slope.
class SecondOrderBSplineSawOscillator {
 public:
  void prepare(float sampleRate) {
    sampleRate_ = safeSampleRate(sampleRate);
    updateIncrement();
  }

  void reset(float phase = 0.0f) {
    phase_ = wrap01(phase);
    // The leaky integrator reconstructs a saw from impulses and a constant negative slope.
    integrator_ = 0.5f - phase_;
    events_.reset();
  }

  void setFreq(float frequencyHz) {
    frequencyHz_ = std::max(0.0f, frequencyHz);
    updateIncrement();
  }

  void setLeak(float leak) { leak_ = clamp(leak, 0.9f, 1.0f); }

  float process() {
    scheduleWrapImpulse();
    const float impulse = events_.process();
    // Integrate the smeared wrap impulse against the constant negative slope
    // (-increment). The impulse kicks the ramp back up each cycle; the tiny
    // leak bleeds off DC drift over long renders. * -2 scales the ramp to +/-1.
    integrator_ = zapDenormal((leak_ * integrator_) + impulse - increment_);
    return -2.0f * integrator_;
  }

 private:
  void updateIncrement() { increment_ = clamp(frequencyHz_ / sampleRate_, 0.0f, 0.49f); }

  void scheduleWrapImpulse() {
    if (increment_ <= 0.0f) {
      return;
    }

    const float nextPhase = phase_ + increment_;
    if (nextPhase >= 1.0f) {
      // Place the discontinuity at its exact sub-sample time to reduce aliasing.
      const float fraction = (1.0f - phase_) / increment_;
      events_.addImpulse(fraction, 1.0f);
    }
    phase_ = wrap01(nextPhase);
  }

  float sampleRate_ = kDefaultSampleRate;
  float frequencyHz_ = 440.0f;
  float increment_ = 440.0f / kDefaultSampleRate;
  float phase_ = 0.0f;
  float integrator_ = 0.5f;
  float leak_ = 0.9999f;
  SecondOrderBSplineEventBuffer events_;
};

// Band-limited square/pulse. A pulse is the integral of alternating edge
// impulses: +1 at the rising edge (the wrap at phase 0) and -1 at the falling
// edge (phase == pulseWidth). Integrating those gives the +/-1 square output.
class SecondOrderBSplinePulseOscillator {
 public:
  void prepare(float sampleRate) {
    sampleRate_ = safeSampleRate(sampleRate);
    updateIncrement();
  }

  void reset(float phase = 0.0f) {
    phase_ = wrap01(phase);
    // A pulse is the integral of alternating edge impulses.
    integrator_ = phase_ < pulseWidth_ ? 0.5f : -0.5f;
    events_.reset();
  }

  void setFreq(float frequencyHz) {
    frequencyHz_ = std::max(0.0f, frequencyHz);
    updateIncrement();
  }

  void setPWM(float width) { pulseWidth_ = clamp(width, 0.01f, 0.99f); }

  float process() {
    schedulePulseImpulses();
    const float impulse = events_.process();
    integrator_ = zapDenormal(integrator_ + impulse);
    return 2.0f * integrator_;
  }

 private:
  void updateIncrement() { increment_ = clamp(frequencyHz_ / sampleRate_, 0.0f, 0.49f); }

  void addEdgeIfCrossed(float start, float end, float edge, float amplitude) {
    if (edge > start && edge <= end && increment_ > 0.0f) {
      events_.addImpulse((edge - start) / increment_, amplitude);
    }
  }

  void schedulePulseImpulses() {
    if (increment_ <= 0.0f) {
      return;
    }

    // Check both the falling PWM edge and the wrap edge; wrapping can expose next-cycle PWM too.
    const float start = phase_;
    const float end = phase_ + increment_;
    addEdgeIfCrossed(start, end, pulseWidth_, -1.0f);
    addEdgeIfCrossed(start, end, 1.0f, 1.0f);
    addEdgeIfCrossed(start, end, 1.0f + pulseWidth_, -1.0f);
    phase_ = wrap01(end);
  }

  float sampleRate_ = kDefaultSampleRate;
  float frequencyHz_ = 440.0f;
  float increment_ = 440.0f / kDefaultSampleRate;
  float phase_ = 0.0f;
  float pulseWidth_ = 0.5f;
  float integrator_ = 0.5f;
  SecondOrderBSplineEventBuffer events_;
};

// Band-limited "hard-synced" saw: a slave sawtooth that is forced to restart
// (phase jumps back to 0) every time the master wraps. Sync ratios produce the
// classic "sync sweep" timbre. Like the plain saw, the output is an integral of
// impulses: a +1 impulse each time the slave wraps (its normal rising edge),
// plus a special impulse each time the master wraps. That master impulse is
// sized to the slave's current ramp height (not a full unit step), because the
// reset cancels only however high the slave had climbed, not a whole cycle.
class SecondOrderBSplineHardSyncSawOscillator {
 public:
  void prepare(float sampleRate) {
    sampleRate_ = safeSampleRate(sampleRate);
    updateIncrements();
  }

  void reset(float masterPhase = 0.0f, float slavePhase = 0.0f) {
    masterPhase_ = wrap01(masterPhase);
    slavePhase_ = wrap01(slavePhase);
    integrator_ = 0.5f - slavePhase_;
    events_.reset();
  }

  void setMasterFrequency(float frequencyHz) {
    masterFrequencyHz_ = std::max(0.0f, frequencyHz);
    updateIncrements();
  }

  void setSlaveFrequency(float frequencyHz) {
    slaveFrequencyHz_ = std::max(0.0f, frequencyHz);
    updateIncrements();
  }

  float process() {
    scheduleSyncImpulses();
    const float impulse = events_.process();
    integrator_ = zapDenormal(integrator_ + impulse - slaveIncrement_);
    return -2.0f * integrator_;
  }

 private:
  void updateIncrements() {
    masterIncrement_ = clamp(masterFrequencyHz_ / sampleRate_, 0.0f, 0.49f);
    slaveIncrement_ = clamp(slaveFrequencyHz_ / sampleRate_, 0.0f, 0.49f);
  }

  static bool sameEventTime(float a, float b) { return std::fabs(a - b) <= 1.0e-5f; }

  void scheduleSyncImpulses() {
    if (masterIncrement_ <= 0.0f || slaveIncrement_ <= 0.0f) {
      return;
    }

    float elapsed = 0.0f;
    int guard = 0;
    // Multiple slave wraps can happen within one master sample at high sync ratios; cap the loop.
    while (elapsed < 1.0f && guard < 4) {
      ++guard;
      const float masterTime = (1.0f - masterPhase_) / masterIncrement_;
      const float slaveTime = (1.0f - slavePhase_) / slaveIncrement_;
      const float nextEventTime = std::min(masterTime, slaveTime);

      if (elapsed + nextEventTime > 1.0f) {
        const float remaining = 1.0f - elapsed;
        masterPhase_ = wrap01(masterPhase_ + (masterIncrement_ * remaining));
        slavePhase_ = wrap01(slavePhase_ + (slaveIncrement_ * remaining));
        return;
      }

      masterPhase_ += masterIncrement_ * nextEventTime;
      slavePhase_ += slaveIncrement_ * nextEventTime;
      elapsed += nextEventTime;

      if (sameEventTime(masterTime, nextEventTime)) {
        // Master reset removes the current slave ramp height, not a full unit step.
        events_.addImpulse(elapsed, slavePhase_);
        masterPhase_ = 0.0f;
        slavePhase_ = 0.0f;
      } else {
        events_.addImpulse(elapsed, 1.0f);
        slavePhase_ = 0.0f;
      }
    }
  }

  float sampleRate_ = kDefaultSampleRate;
  float masterFrequencyHz_ = 110.0f;
  float slaveFrequencyHz_ = 440.0f;
  float masterIncrement_ = 110.0f / kDefaultSampleRate;
  float slaveIncrement_ = 440.0f / kDefaultSampleRate;
  float masterPhase_ = 0.0f;
  float slavePhase_ = 0.0f;
  float integrator_ = 0.5f;
  SecondOrderBSplineEventBuffer events_;
};

class NoiseOscillator {
 public:
  explicit NoiseOscillator(std::uint32_t seed = 0x12345678u) : rng_(seed) {}

  void reseed(std::uint32_t seed) { rng_ = XorShift32(seed); }
  float process() { return rng_.nextBipolar(); }

 private:
  XorShift32 rng_;
};

}  // namespace rpdsp
