#pragma once

#include "rp2350_audio_dsp/platform/hardware_interpolator.h"
#include "rp2350_audio_dsp/dsp/algorithm.h"
#include "rp2350_audio_dsp/dsp/audio_block.h"
#include "rp2350_audio_dsp/dsp/filter.h"
#include "rp2350_audio_dsp/dsp/oscillator.h"

#include <cstddef>
#include <cstdint>

class ExamplePatch {
 public:
  void prepare(float sampleRate, size_t maxBlockFrames) {
    (void)maxBlockFrames;
    sampleRate_ = sampleRate > 1.0f ? sampleRate : 48000.0f;

    carrier_.prepare(sampleRate_);
    carrier_.setFreq(220.0f);
    modulator_.prepare(sampleRate_);
    modulator_.setFreq(110.0f);
    fmRatio_.prepare(sampleRate_);
    fmRatio_.setFreq(0.055f);

    lfo_.prepare(sampleRate_);
    lfo_.setFreq(0.35f);

    tone_.prepare(sampleRate_);
    tone_.setCutoff(4800.0f);

    wavefolder_.init(rpdsp::HardwareInterpolatorPool::Resource::Core0Interp1, 13);
    wavefolder_.setStages(4);
    wavefolder_.setGain(1.0f);
  }

  void reset() {}
  void setOutputGainRaw(std::uint16_t) {}
  float outputGain() const { return 1.0f; }

  void process(rpdsp::DefaultAudioBlock& block) {
    for (size_t i = 0; i < block.frames(); ++i) {
      const float lfoVal = lfo_.process() * 0.5f + 0.5f;
      const float fmMod = fmRatio_.process() * 0.5f + 0.5f;

      const float freqHz = 110.0f + fmMod * 440.0f;
      carrier_.setFreq(freqHz);
      modulator_.setFreq(freqHz * rpdsp::lerp(0.5f, 3.0f, lfoVal));

      const float fmDepth = 0.3f + lfoVal * 0.4f;
      const float carrierVal = (carrier_.process() + modulator_.process() * fmDepth) * 0.5f;

      const float gain = 1.5f + lfoVal * 3.5f;
      wavefolder_.setGain(gain);
      wavefolder_.setStages(static_cast<int>(2 + lfoVal * 4));

      std::int32_t intInput = static_cast<std::int32_t>(carrierVal * 8192.0f);
      std::int32_t intOutput = wavefolder_.process(intInput);

      float folded = static_cast<float>(intOutput) / 8192.0f;
      tone_.setCutoff(800.0f + lfoVal * 6000.0f);
      float filtered = tone_.process(folded);

      block.left()[i] = filtered * 0.45f;
      block.right()[i] = filtered * 0.45f;
    }
  }

 private:
  float sampleRate_ = 48000.0f;
  rpdsp::SineOscillator carrier_;
  rpdsp::SineOscillator modulator_;
  rpdsp::SineOscillator fmRatio_;
  rpdsp::SineOscillator lfo_;
  rpdsp::BiquadLowpass tone_;
  rpdsp::HardwareWavefolder wavefolder_;
};
