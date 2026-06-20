// Renders a metered stereo pass-through gain example with smoothed gain changes.

#include "example_utils.h"

#include "rp2350_audio_dsp/dsp/algorithm.h"
#include "rp2350_audio_dsp/dsp/analysis.h"
#include "rp2350_audio_dsp/dsp/audio_block.h"
#include "rp2350_audio_dsp/dsp/config.h"
#include "rp2350_audio_dsp/dsp/oscillator.h"
#include "rp2350_audio_dsp/dsp/parameter_smoother.h"

#include <algorithm>
#include <array>
#include <cstddef>

namespace {

class PassThroughProcessor {
 public:
  void prepare(float sampleRate, size_t maxBlockFrames) {
    (void)maxBlockFrames;
    gain_.prepare(sampleRate, 2.0f);
    reset();
  }

  void reset() {
    gain_.reset(1.0f);
    leftPeak_ = 0.0f;
    rightPeak_ = 0.0f;
  }

  void setGain(float gain) { gain_.setTarget(std::clamp(gain, 0.0f, 2.0f)); }

  void process(rpdsp::DefaultAudioBlock& block) {
    // Metering is scoped to the processed block, matching callback-sized audio work.
    leftMeter_.reset();
    rightMeter_.reset();

    for (size_t i = 0; i < block.frames(); ++i) {
      const float g = gain_.next();
      block.left()[i] *= g;
      block.right()[i] *= g;
      leftMeter_.process(block.left()[i]);
      rightMeter_.process(block.right()[i]);
    }

    leftPeak_ = leftMeter_.peak();
    rightPeak_ = rightMeter_.peak();
    leftRms_ = leftMeter_.rms();
    rightRms_ = rightMeter_.rms();
  }

  [[nodiscard]] float leftPeak() const { return leftPeak_; }
  [[nodiscard]] float rightPeak() const { return rightPeak_; }
  [[nodiscard]] float leftRms() const { return leftRms_; }
  [[nodiscard]] float rightRms() const { return rightRms_; }

 private:
  rpdsp::LinearSmoother gain_;
  rpdsp::RmsPeakMeter leftMeter_;
  rpdsp::RmsPeakMeter rightMeter_;
  float leftPeak_ = 0.0f;
  float rightPeak_ = 0.0f;
  float leftRms_ = 0.0f;
  float rightRms_ = 0.0f;
};

class MeteredGainDemo {
 public:
  void prepare(float sampleRate, size_t maxBlockFrames) {
    processor_.prepare(sampleRate, maxBlockFrames);
    lfo_.prepare(sampleRate);
    lfo_.setFreq(0.11f);
    for (size_t i = 0; i < tones_.size(); ++i) {
      tones_[i].prepare(sampleRate);
      tones_[i].setFreq(rpdsp::midiNoteToHz(45.0f + static_cast<float>(i * 7)));
      tones_[i].reset(static_cast<float>(i) * 0.17f);
    }
  }

  void process(rpdsp::DefaultAudioBlock& block) {
    // The host demo fills a tone block, then runs it through the callback-style processor.
    for (size_t i = 0; i < block.frames(); ++i) {
      const float gainMovement = (lfo_.process() * 0.5f) + 0.5f;
      processor_.setGain(rpdsp::lerp(0.45f, 1.20f, gainMovement));

      float left = 0.0f;
      float right = 0.0f;
      for (size_t voice = 0; voice < tones_.size(); ++voice) {
        const float pan = voice == 0 ? -0.65f : voice == 1 ? 0.0f : 0.60f;
        const float sample = tones_[voice].process() * 0.16f;
        left += sample * rpdsp::equalPowerPanLeft(pan);
        right += sample * rpdsp::equalPowerPanRight(pan);
      }
      block.left()[i] = left;
      block.right()[i] = right;
    }
    processor_.process(block);
  }

 private:
  PassThroughProcessor processor_;
  rpdsp::SineOscillator lfo_;
  std::array<rpdsp::SineOscillator, 3> tones_;
};

PassThroughProcessor processor;

}  // namespace

void processAudioBlock(rpdsp::DefaultAudioBlock& io) {
  processor.process(io);
}

int main() {
  processor.prepare(rpdsp::kDefaultSampleRate, rpdsp::kDefaultBlockSize);
  processor.setGain(1.0f);

  MeteredGainDemo demo;
  demo.prepare(example::kSampleRate, example::kBlockSize);
  return example::renderToWav(demo, "pass_through_metered_gain.wav", 6.0f) ? 0 : 1;
}
