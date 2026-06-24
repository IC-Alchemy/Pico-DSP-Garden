#pragma once

#include "rpdsp/hardware_interpolator.h"
#include "oscillator.h"

#include <cstddef>
#include <cstdint>

// Audio cross-fade via INTERP0 blend mode.
//
// Recreates the Cornell ECE4760 RP2040 interpolator demo "audio cross-fade via
// blend mode" (people.ece.cornell.edu/land/courses/ece4760/RP2040). Two tones —
// a low 200 Hz and a higher 600 Hz — are mixed by sweeping a single 8-bit blend
// weight (alpha). On hardware the entire mix is one interpolator read per
// sample: the blend datapath does  out = a + ((b - a) * alpha) >> 8  with no CPU
// multiply or branch. Here a slow triangle LFO sweeps alpha from 0 to 255 and
// back, so the timbre morphs smoothly between the two pitches.
//
// Cornell drives both tones from the interpolators' own DDS phase accumulators;
// to keep the focus on the blend primitive we generate the two sources with the
// software oscillators and let rpdsp::HardwareBlend do the mix.

class ExamplePatch {
 public:
  void prepare(float sampleRate, size_t maxBlockFrames) {
    (void)maxBlockFrames;
    sampleRate_ = sampleRate > 1.0f ? sampleRate : 48000.0f;

    toneLow_.prepare(sampleRate_);
    toneLow_.setFreq(200.0f);
    toneHigh_.prepare(sampleRate_);
    toneHigh_.setFreq(600.0f);

    // ~0.4 Hz crossfade sweep (≈2.5 s per full back-and-forth).
    sweep_.prepare(sampleRate_);
    sweep_.setFreq(0.4f);

    // Blend mode lives on interp0; init() claims the lane and configures it.
    blend_.init(rpdsp::HardwareInterpolatorPool::Resource::Core0Interp0);
  }

  void reset() {}
  void setOutputGainRaw(std::uint16_t) {}
  float outputGain() const { return 1.0f; }

  void process(rpdsp::DefaultAudioBlock& block) {
    for (size_t i = 0; i < block.frames(); ++i) {
      // Two signed audio sources, scaled into int16 range so the blend runs in
      // the same integer domain the hardware datapath expects.
      const std::int32_t a = static_cast<std::int32_t>(toneLow_.process() * 32767.0f);
      const std::int32_t b = static_cast<std::int32_t>(toneHigh_.process() * 32767.0f);

      // Map the bipolar LFO (-1..1) to an 8-bit blend weight (0..255).
      const float lfo = sweep_.process() * 0.5f + 0.5f;
      const std::uint8_t alphaQ8 = static_cast<std::uint8_t>(lfo * 255.0f);

      // Single hardware interpolator read does the whole crossfade.
      const std::int32_t mixed = blend_.blend(a, b, alphaQ8);

      const float out = static_cast<float>(mixed) / 32768.0f * 0.6f;
      block.left()[i] = out;
      block.right()[i] = out;
    }
  }

 private:
  float sampleRate_ = 48000.0f;
  rpdsp::SineOscillator toneLow_;   // 200 Hz source
  rpdsp::SineOscillator toneHigh_;  // 600 Hz source
  rpdsp::SineOscillator sweep_;     // crossfade LFO
  rpdsp::HardwareBlend blend_;      // INTERP0 blend-mode mixer
};
