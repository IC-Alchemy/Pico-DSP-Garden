#pragma once

#include "config.h"

#include <cstdint>

namespace rpdsp {

// PPQN transport tracker: counts ticks/loops from an external clock gate,
// reports the nearest musical step, and emits divided clock-out pulses.
class ClockTracker {
 public:
  // Quantized clock position, expressed as the nearest step plus tick offset
  // from that step. Negative offsets mean the clock is closer to the next step.
  struct StepPosition {
    int step;
    int offsetTicks;
  };

  // Defaults assume a 96 PPQN clock over a two-bar 4/4 loop with 16th-note steps.
  void prepare(float sampleRate = kDefaultSampleRate,
               float bpm = 120.0f,
               int pulsesPerQuarter = 96,
               int quarterNotes = 8,
               int stepsPerQuarter = 4) {
    sampleRate_ = sampleRate > 1.0f ? sampleRate : kDefaultSampleRate;
    pulsesPerQuarter_ = pulsesPerQuarter;
    quarterNotes_ = quarterNotes;
    stepsPerQuarter_ = stepsPerQuarter;
    setBpm(bpm);
    reset();
  }

  void reset() {
    tick_ = 0;
    loopCount_ = 0;
    samplesSinceClock_ = 0;
    lastIntervalSamples_ = 0;
    haveFirstInterval_ = false;
    lastClockHigh_ = false;
    clockOutTickCount_ = 0;
  }

  void setBpm(float bpm) {
    bpm_ = bpm;
    clockFrequency_ = pulsesPerQuarter_ * bpm_ / 60.0f;
    samplesPerTick_ = clockFrequency_ > 0.0f ? sampleRate_ / clockFrequency_ : 0.0f;
  }

  [[nodiscard]] float bpm() const { return bpm_; }
  [[nodiscard]] float clockFrequency() const { return clockFrequency_; }
  [[nodiscard]] float samplesPerTick() const { return samplesPerTick_; }
  [[nodiscard]] int tick() const { return tick_; }
  [[nodiscard]] int loopCount() const { return loopCount_; }
  [[nodiscard]] int totalSteps() const { return quarterNotes_ * stepsPerQuarter_; }
  [[nodiscard]] int totalTicks() const { return quarterNotes_ * pulsesPerQuarter_; }

  // Advance the internal PPQN tick by one; returns true when the loop boundary wrapped.
  bool advanceTick() {
    const int loopTicks = totalTicks();
    if (loopTicks <= 0)
      return false;

    // Wrap at the configured loop length and count completed loops.
    ++tick_;
    if (tick_ >= loopTicks) {
      tick_ = 0;
      ++loopCount_;
      return true;
    }
    return false;
  }

  // Feed one sample of an external clock gate; returns true on a rising edge.
  bool processExternalClock(bool clockHigh) {
    const bool rising = clockHigh && !lastClockHigh_;
    if (rising) {
      // Track a lightly smoothed period between rising edges; stale clocks
      // restart the estimate instead of averaging in a long gap.
      if (!haveFirstInterval_ || isStale()) {
        // First edge (or re-sync after staleness): seed, do not average.
        lastIntervalSamples_ = samplesSinceClock_;
        haveFirstInterval_ = true;
      } else {
        // Average in float so the smoothing does not bias low from int truncation.
        lastIntervalSamples_ = static_cast<std::int32_t>(
            (static_cast<float>(lastIntervalSamples_) + static_cast<float>(samplesSinceClock_)) * 0.5f);
      }

      samplesSinceClock_ = 0;
      advanceTick();
    } else {
      ++samplesSinceClock_;
    }

    lastClockHigh_ = clockHigh;
    return rising;
  }

  [[nodiscard]] bool isStale(float timeoutSeconds = 2.0f) const {
    return samplesSinceClock_ > static_cast<std::int32_t>(sampleRate_ * timeoutSeconds);
  }

  [[nodiscard]] float externalIntervalSeconds() const {
    if (sampleRate_ <= 0.0f || !haveFirstInterval_ || lastIntervalSamples_ <= 0 || isStale())
      return 0.0f;
    return static_cast<float>(lastIntervalSamples_) / sampleRate_;
  }

  [[nodiscard]] float externalFrequency() const {
    const float interval = externalIntervalSeconds();
    return interval > 0.0f ? 1.0f / interval : 0.0f;
  }

  [[nodiscard]] StepPosition currentStepPosition() const {
    const int ticksPerStep = pulsesPerQuarter_ / stepsPerQuarter_;
    if (ticksPerStep <= 0 || tick_ == 0)
      return {0, 0};

    // Report the nearest musical step, not just the previous one, so callers
    // can tell whether a tick is late or early relative to the step grid.
    const int wholeStep = tick_ / ticksPerStep;
    const int offset = tick_ % ticksPerStep;
    const int halfStep = ticksPerStep / 2;
    const int loopSteps = totalSteps();

    if (offset >= halfStep) {
      const int nextStep = wholeStep + 1;
      return {nextStep >= loopSteps ? 0 : nextStep,
              -((nextStep * ticksPerStep) - tick_)};
    }

    return {wholeStep, offset};
  }

  // Emit a one-tick pulse at the start of each divided period.
  bool clockOutPulse(int divider = 4) {
    const int period = divider > 0 ? pulsesPerQuarter_ / divider : pulsesPerQuarter_;
    if (period <= 0)
      return false;

    const bool high = clockOutTickCount_ == 0;
    ++clockOutTickCount_;
    if (clockOutTickCount_ >= period)
      clockOutTickCount_ = 0;
    return high;
  }

 private:
  float sampleRate_ = kDefaultSampleRate;
  float bpm_ = 120.0f;
  float clockFrequency_ = 0.0f;
  float samplesPerTick_ = 0.0f;
  int pulsesPerQuarter_ = 96;
  int quarterNotes_ = 8;
  int stepsPerQuarter_ = 4;
  int tick_ = 0;
  int loopCount_ = 0;
  std::int32_t samplesSinceClock_ = 0;
  std::int32_t lastIntervalSamples_ = 0;
  bool haveFirstInterval_ = false;
  bool lastClockHigh_ = false;
  int clockOutTickCount_ = 0;
};

}  // namespace rpdsp
