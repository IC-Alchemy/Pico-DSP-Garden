#pragma once

#include "algorithm.h"
#include "delay_line.h"
#include "filter.h"
#include "oscillator.h"
#include "realtime.h"

#include <array>
#include <cmath>
#include <cstddef>

namespace rpdsp {

class Waveshaper {
 public:
  void setDrive(float drive) { drive_ = std::max(0.1f, drive); }
  void setOutputGain(float gain) { outputGain_ = gain; }

  float process(float input) const {
    // Normalize by tanh(drive) so changing drive mostly changes tone, not loudness.
    const float norm = std::tanh(drive_);
    return (std::tanh(input * drive_) / (norm > 0.0f ? norm : 1.0f)) * outputGain_;
  }

 private:
  float drive_ = 1.0f;
  float outputGain_ = 1.0f;
};

template <size_t Capacity>
class Delay {
 public:
  void prepare(float sampleRate) {
    sampleRate_ = sampleRate > 1.0f ? sampleRate : kDefaultSampleRate;
    reset();
  }

  void reset() { delay_.reset(); }
  void setDelaySamples(float samples) { delaySamples_ = clamp(samples, 0.0f, static_cast<float>(Capacity - 2)); }
  void setDelayMilliseconds(float milliseconds) { setDelaySamples(milliseconds * 0.001f * sampleRate_); }
  void setFeedback(float feedback) { feedback_ = clamp(feedback, -0.99f, 0.99f); }
  void setMix(float mix) { mix_ = clamp01(mix); }

  float process(float input) {
    // The just-pushed sample becomes visible one sample later, so compensate by one.
    const float readDelay = delaySamples_ > 0.0f ? delaySamples_ - 1.0f : 0.0f;
    const float delayed = delay_.readCubic(readDelay);
    delay_.push(input + (delayed * feedback_));
    return lerp(input, delayed, mix_);
  }

 private:
  float sampleRate_ = kDefaultSampleRate;
  float delaySamples_ = 1.0f;
  float feedback_ = 0.0f;
  float mix_ = 0.5f;
  DelayLine<Capacity> delay_;
};

template <size_t Capacity>
class Chorus {
 public:
  void prepare(float sampleRate) {
    sampleRate_ = sampleRate > 1.0f ? sampleRate : kDefaultSampleRate;
    lfo_.prepare(sampleRate_);
    lfo_.setFreq(rateHz_);
    reset();
  }

  void reset() { delay_.reset(); }
  void setRate(float hz) {
    rateHz_ = std::max(0.01f, hz);
    lfo_.setFreq(rateHz_);
  }
  void setDepthMilliseconds(float milliseconds) { depthSamples_ = std::max(0.0f, milliseconds * 0.001f * sampleRate_); }
  void setBaseDelayMilliseconds(float milliseconds) { baseDelaySamples_ = std::max(1.0f, milliseconds * 0.001f * sampleRate_); }
  void setMix(float mix) { mix_ = clamp01(mix); }

  float process(float input) {
    // Sweep fractional delay with an LFO to create pitch modulation from changing read time.
    const float lfo = (lfo_.process() * 0.5f) + 0.5f;
    const float maxDelay = static_cast<float>(Capacity - 2);
    const float delaySamples = clamp(baseDelaySamples_ + (depthSamples_ * lfo), 1.0f, maxDelay);
    const float wet = delay_.readCubic(delaySamples);
    delay_.push(input);
    return lerp(input, wet, mix_);
  }

 private:
  float sampleRate_ = kDefaultSampleRate;
  float rateHz_ = 0.25f;
  float depthSamples_ = 12.0f;
  float baseDelaySamples_ = 24.0f;
  float mix_ = 0.35f;
  SineOscillator lfo_;
  DelayLine<Capacity> delay_;
};

template <size_t Capacity>
class CombFilter {
 public:
  void reset() { delay_.reset(); }
  void setDelaySamples(size_t samples) { delaySamples_ = samples < Capacity ? samples : Capacity - 1; }
  void setFeedback(float feedback) { feedback_ = clamp(feedback, -0.99f, 0.99f); }

  float process(float input) {
    // Feed-forward output plus feedback storage is the small building block of Schroeder reverb.
    const float delayed = delay_.read(delaySamples_);
    delay_.push(input + delayed * feedback_);
    return delayed;
  }

 private:
  DelayLine<Capacity> delay_;
  size_t delaySamples_ = Capacity / 2;
  float feedback_ = 0.7f;
};

template <size_t Capacity>
class AllpassFilter {
 public:
  void reset() { delay_.reset(); }
  void setDelaySamples(size_t samples) { delaySamples_ = samples < Capacity ? samples : Capacity - 1; }
  void setFeedback(float feedback) { feedback_ = clamp(feedback, -0.99f, 0.99f); }

  float process(float input) {
    // Allpass diffusion changes phase and echo density while keeping a flat magnitude response.
    const float delayed = delay_.read(delaySamples_);
    const float bufferInput = input + delayed * feedback_;
    delay_.push(bufferInput);
    return delayed - feedback_ * bufferInput;
  }

 private:
  DelayLine<Capacity> delay_;
  size_t delaySamples_ = Capacity / 2;
  float feedback_ = 0.5f;
};

class SchroederReverb {
 public:
  void prepare(float sampleRate) {
    sampleRate_ = sampleRate > 1.0f ? sampleRate : kDefaultSampleRate;
    damping_.prepare(sampleRate_);
    damping_.setCutoff(6000.0f);
    reset();
  }

  void reset() {
    comb1_.reset();
    comb2_.reset();
    comb3_.reset();
    comb4_.reset();
    allpass1_.reset();
    allpass2_.reset();
    damping_.reset();
  }

  void setRoomSize(float roomSize) {
    // Larger rooms are modeled as longer-lived feedback, with slight offsets to decorrelate combs.
    const float feedback = lerp(0.55f, 0.88f, clamp01(roomSize));
    comb1_.setFeedback(feedback);
    comb2_.setFeedback(feedback * 0.97f);
    comb3_.setFeedback(feedback * 0.94f);
    comb4_.setFeedback(feedback * 0.91f);
  }

  void setMix(float mix) { mix_ = clamp01(mix); }

  float process(float input) {
    const float damped = damping_.process(input);
    // Parallel combs build the tail; serial allpasses turn obvious echoes into diffusion.
    float wet = comb1_.process(damped) + comb2_.process(damped) + comb3_.process(damped) + comb4_.process(damped);
    wet *= 0.25f;
    wet = allpass2_.process(allpass1_.process(wet));
    return lerp(input, wet, mix_);
  }

 private:
  float sampleRate_ = kDefaultSampleRate;
  float mix_ = 0.25f;
  OnePoleLowpass damping_;
  CombFilter<1600> comb1_;
  CombFilter<1700> comb2_;
  CombFilter<1800> comb3_;
  CombFilter<1900> comb4_;
  AllpassFilter<240> allpass1_;
  AllpassFilter<80> allpass2_;
};

class StereoSchroederReverb {
 public:
  void prepare(float sampleRate) {
    left_.prepare(sampleRate);
    right_.prepare(sampleRate);
    left_.setMix(1.0f);
    right_.setMix(1.0f);
    reset();
  }

  void reset() {
    left_.reset();
    right_.reset();
    rightPreDelay_.reset();
  }

  void setRoomSize(float roomSize) {
    left_.setRoomSize(roomSize);
    right_.setRoomSize(roomSize);
  }

  void setMix(float mix) { mix_ = clamp01(mix); }
  void setWidth(float width) { width_ = clamp(width, 0.0f, 2.0f); }
  void setCrossfeed(float crossfeed) { crossfeed_ = clamp(crossfeed, 0.0f, 0.7f); }
  void setRightPreDelaySamples(size_t samples) {
    rightPreDelaySamples_ = samples < kRightPreDelayCapacity ? samples : kRightPreDelayCapacity - 1;
  }

  std::array<float, 2> process(float leftInput, float rightInput) {
    // Crossfeed and a tiny right pre-delay decorrelate the tanks for stereo width.
    const float leftFeed = leftInput + rightInput * crossfeed_;
    const float rightFeed = rightPreDelay_.read(rightPreDelaySamples_);
    rightPreDelay_.push(rightInput + leftInput * crossfeed_);

    const float wetLeftRaw = left_.process(leftFeed);
    const float wetRightRaw = right_.process(rightFeed);
    // Mid/side width preserves the wet center while scaling stereo spread.
    const float mid = (wetLeftRaw + wetRightRaw) * 0.5f;
    const float side = (wetLeftRaw - wetRightRaw) * 0.5f * width_;
    const float wetLeft = mid + side;
    const float wetRight = mid - side;

    return {lerp(leftInput, wetLeft, mix_), lerp(rightInput, wetRight, mix_)};
  }

 private:
  static constexpr size_t kRightPreDelayCapacity = 257;

  float mix_ = 0.25f;
  float width_ = 1.0f;
  float crossfeed_ = 0.35f;
  size_t rightPreDelaySamples_ = 23;
  SchroederReverb left_;
  SchroederReverb right_;
  DelayLine<kRightPreDelayCapacity> rightPreDelay_;
};

}  // namespace rpdsp
