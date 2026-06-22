#pragma once

#include "rpdsp/hardware_interpolator.h"

#include <cstddef>
#include <cstdint>

// Minimal hardware-interpolator example.
//
// A single wavetable oscillator plays a fixed A4 (440 Hz) sine. The phase
// accumulator lives in the RP2xxx INTERP0 peripheral, and every sample is
// linearly interpolated between two table entries by the hardware blend
// datapath — no CPU multiply in the audio loop. This is the smallest possible
// patch that exercises rpdsp::HardwareOscillator; compare it with
// HardwareInterpOsc, which layers LFOs, a second voice, and stereo motion on
// top of the same primitive.

class ExamplePatch {
 public:
  void prepare(float sampleRate, size_t maxBlockFrames) {
    (void)maxBlockFrames;
    sampleRate_ = sampleRate > 1.0f ? sampleRate : 48000.0f;

    // Q12.20 phase: 12 integer bits index the 16-entry table (4 used),
    // 20 fractional bits feed the interpolator's 8-bit blend weight.
    constexpr std::uint8_t kFractionalBits = 20;

    std::uint32_t tuningWord = 0;
    rpdsp::HardwareOscillator::tuningWordFromFrequency(
        kTableSize, kFractionalBits, static_cast<std::uint32_t>(sampleRate_),
        440, tuningWord);

    // INTERP0 is required: only interp0 supports the blend mode used for
    // linear interpolation. init() claims the lane and configures both lanes.
    osc_.init(interp0, kSineTable, kTableSize, kFractionalBits, /*phase=*/0, tuningWord);
  }

  void reset() {}
  void setOutputGainRaw(std::uint16_t) {}
  float outputGain() const { return 1.0f; }

  void process(rpdsp::DefaultAudioBlock& block) {
    for (size_t i = 0; i < block.frames(); ++i) {
      // Advance the hardware phase accumulator and read the interpolated
      // 16-bit sample for this step.
      std::int32_t sample = 0;
      osc_.nextSample(sample);

      // Convert the int16 wavetable sample to floating-point ±1.0 and emit it
      // to both channels at a conservative level.
      const float out = static_cast<float>(sample) / 32768.0f * 0.5f;
      block.left()[i] = out;
      block.right()[i] = out;
    }
  }

 private:
  float sampleRate_ = 48000.0f;
  rpdsp::HardwareOscillator osc_;

  // 16-entry sine wavetable; hardware interpolation band-limits the steps.
  static constexpr std::uint32_t kTableSize = 16;
  static inline const std::int16_t kSineTable[kTableSize] = {
      0, 12539, 23170, 30273,
      32767, 30273, 23170, 12539,
      0, -12539, -23170, -30273,
      -32768, -30273, -23170, -12539
  };
};
