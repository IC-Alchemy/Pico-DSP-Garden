#pragma once

#include "rpdsp/hardware_interpolator.h"
#include "algorithm.h"
#include "oscillator.h"

#include <cstddef>
#include <cstdint>

class ExamplePatch {
 public:
  void prepare(float sampleRate, size_t maxBlockFrames) {
    (void)maxBlockFrames;
    sampleRate_ = sampleRate > 1.0f ? sampleRate : 48000.0f;

    lfo_.prepare(sampleRate_);
    lfo_.setFreq(0.17f);
    lfo2_.prepare(sampleRate_);
    lfo2_.setFreq(0.053f);

    std::uint32_t tuningWord = 0;
    rpdsp::HardwareOscillator::tuningWordFromFrequency(
        kTableSize, 20, static_cast<std::uint32_t>(sampleRate_), 440, tuningWord);

    osc_.init(interp0, kSineTable, kTableSize, 20, 0, tuningWord);

    std::uint32_t tuningWord2 = 0;
    rpdsp::HardwareOscillator::tuningWordFromFrequency(
        kTableSize, 20, static_cast<std::uint32_t>(sampleRate_), 523, tuningWord2);

    osc2_.init(interp1, kTriangleTable, kTableSize, 20, 0, tuningWord2);
  }

  void reset() {}
  void setOutputGainRaw(std::uint16_t) {}
  float outputGain() const { return 1.0f; }

  void process(rpdsp::DefaultAudioBlock& block) {
    for (size_t i = 0; i < block.frames(); ++i) {
      const float lfoVal = lfo_.process();
      const float lfo2Val = lfo2_.process() * 0.5f + 0.5f;

      const float freqHz1 = 330.0f + lfoVal * 220.0f + lfo2Val * 55.0f;
      const float freqHz2 = 220.0f + lfo2Val * 440.0f + lfoVal * 110.0f;

      std::uint32_t tuningWord = 0;
      rpdsp::HardwareOscillator::tuningWordFromFrequency(
          kTableSize, 20, static_cast<std::uint32_t>(sampleRate_), static_cast<std::uint32_t>(freqHz1), tuningWord);
      osc_.setPhaseIncrement(tuningWord);

      std::uint32_t tuningWord2 = 0;
      rpdsp::HardwareOscillator::tuningWordFromFrequency(
          kTableSize, 20, static_cast<std::uint32_t>(sampleRate_), static_cast<std::uint32_t>(freqHz2), tuningWord2);
      osc2_.setPhaseIncrement(tuningWord2);

      std::int32_t sample1 = 0;
      std::int32_t sample2 = 0;
      osc_.nextSample(sample1);
      osc2_.nextSample(sample2);

      float out1 = static_cast<float>(sample1) / 32768.0f;
      float out2 = static_cast<float>(sample2) / 32768.0f;

      const float pan = lfo2Val * 0.5f + 0.3f;
      const float left = out1 * (1.0f - pan) + out2 * pan;
      const float right = out1 * pan + out2 * (1.0f - pan);

      block.left()[i] = left * 0.38f;
      block.right()[i] = right * 0.38f;
    }
  }

 private:
  float sampleRate_ = 48000.0f;
  rpdsp::HardwareOscillator osc_;
  rpdsp::HardwareOscillator osc2_;
  rpdsp::SineOscillator lfo_;
  rpdsp::SineOscillator lfo2_;

  static constexpr std::uint32_t kTableSize = 16;
  static inline const std::int16_t kSineTable[kTableSize] = {
      0, 12539, 23170, 30273,
      32767, 30273, 23170, 12539,
      0, -12539, -23170, -30273,
      -32768, -30273, -23170, -12539
  };

  static inline const std::int16_t kTriangleTable[kTableSize] = {
      0, 8192, 16384, 24576,
      32767, 24576, 16384, 8192,
      0, -8192, -16384, -24576,
      -32768, -24576, -16384, -8192
  };
};
