#pragma once

#include "rpdsp/hardware_interpolator.h"
#include "algorithm.h"
#include "oscillator.h"

#include <cstddef>
#include <cstdint>

// Dual-oscillator wavetable patch with hardware interpolation.
// Two oscillators (sine + triangle) tuned a minor third apart are independently
// pitch-modulated by two slow LFOs at different rates. A shared unipolar LFO
// drives an auto-pan crossfade between them, producing a slowly evolving,
// dissonant drone that breathes and wanders across the stereo field.

class ExamplePatch {
 public:
  void prepare(float sampleRate, size_t maxBlockFrames) {
    (void)maxBlockFrames;
    sampleRate_ = sampleRate > 1.0f ? sampleRate : 48000.0f;

    // Two slow LFOs for independent pitch drift — emulates organic detuning
    lfo_.prepare(sampleRate_);
    lfo_.setFreq(0.17f);   // ~6-second cycle, gentle wandering
    lfo2_.prepare(sampleRate_);
    lfo2_.setFreq(0.053f);  // ~19-second cycle, slow evolving backdrop

    // Osc 1: sine wave tuned to A4 (440 Hz)
    std::uint32_t tuningWord = 0;
    rpdsp::HardwareOscillator::tuningWordFromFrequency(
        kTableSize, 20, static_cast<std::uint32_t>(sampleRate_), 440, tuningWord);

    osc_.init(interp0, kSineTable, kTableSize, 20, 0, tuningWord);

    // Osc 2: triangle wave tuned to C5 (523 Hz), a minor third above A4
    // Together they create a tense, dissonant interval that the LFOs bring to life
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
      // LFO1: bipolar, sweeps pitch above and below center
      const float lfoVal = lfo_.process();
      // LFO2: unipolar (0…1), used for both slow pitch drift and stereo panning
      const float lfo2Val = lfo2_.process() * 0.5f + 0.5f;

      // Osc 1 base ~330 Hz with ±220 Hz wiggle from LFO1 + subtle 55 Hz drift from LFO2
      const float freqHz1 = 330.0f + lfoVal * 220.0f + lfo2Val * 55.0f;
      // Osc 2 base ~220 Hz with stronger LFO2 sway (±440 Hz) and tighter LFO1 coupling
      const float freqHz2 = 220.0f + lfo2Val * 440.0f + lfoVal * 110.0f;

      // Recompute tuning words every sample for continuous LFO-driven pitch modulation
      std::uint32_t tuningWord = 0;
      rpdsp::HardwareOscillator::tuningWordFromFrequency(
          kTableSize, 20, static_cast<std::uint32_t>(sampleRate_), static_cast<std::uint32_t>(freqHz1), tuningWord);
      osc_.setPhaseIncrement(tuningWord);

      std::uint32_t tuningWord2 = 0;
      rpdsp::HardwareOscillator::tuningWordFromFrequency(
          kTableSize, 20, static_cast<std::uint32_t>(sampleRate_), static_cast<std::uint32_t>(freqHz2), tuningWord2);
      osc2_.setPhaseIncrement(tuningWord2);

      // Let the hardware interpolator advance and produce the next sample
      std::int32_t sample1 = 0;
      std::int32_t sample2 = 0;
      osc_.nextSample(sample1);
      osc2_.nextSample(sample2);

      // Convert 16-bit integer wavetable samples to floating-point ±1.0
      float out1 = static_cast<float>(sample1) / 32768.0f;
      float out2 = static_cast<float>(sample2) / 32768.0f;

      // Stereo crossfade pan: LFO2 sweeps pan from 0.3 to 0.8 (slightly off-center)
      // When pan=0, out1 is hard-left and out2 hard-right; when pan=1, positions swap
      const float pan = lfo2Val * 0.5f + 0.3f;
      const float left = out1 * (1.0f - pan) + out2 * pan;
      const float right = out1 * pan + out2 * (1.0f - pan);

      // Master level: -8.4 dB to prevent clipping when both oscillators sum
      block.left()[i] = left * 0.38f;
      block.right()[i] = right * 0.38f;
    }
  }

 private:
  float sampleRate_ = 48000.0f;
  rpdsp::HardwareOscillator osc_;    // Sine — crisp, pure, the "lead" voice
  rpdsp::HardwareOscillator osc2_;   // Triangle — warmer, harmonic-rich foil
  rpdsp::SineOscillator lfo_;        // Bipolar LFO, ~6 s cycle
  rpdsp::SineOscillator lfo2_;       // Unipolar LFO, ~19 s cycle

  // 16-sample wavetables with hardware linear interpolation for band-limiting
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
