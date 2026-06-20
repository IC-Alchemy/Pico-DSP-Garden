// Renders a slowly modulated pad through delay, diffusion, chorus, and reverb stages.

#include "example_utils.h"

#include "rp2350_audio_dsp/dsp/algorithm.h"
#include "rp2350_audio_dsp/dsp/audio_block.h"
#include "rp2350_audio_dsp/dsp/config.h"
#include "rp2350_audio_dsp/dsp/delay_line.h"
#include "rp2350_audio_dsp/dsp/effects.h"
#include "rp2350_audio_dsp/dsp/filter.h"
#include "rp2350_audio_dsp/dsp/oscillator.h"
#include "rp2350_audio_dsp/dsp/realtime.h"

#include <array>
#include <cstddef>

namespace {

class ModulatedSpace {
 public:
  void prepare(float sampleRate, size_t maxBlockFrames) {
    (void)maxBlockFrames;
    sampleRate_ = sampleRate > 1.0f ? sampleRate : rpdsp::kDefaultSampleRate;
    for (size_t i = 0; i < tones_.size(); ++i) {
      tones_[i].prepare(sampleRate_);
      tones_[i].setFreq(rpdsp::midiNoteToHz(kChord[i]));
      tones_[i].reset(static_cast<float>(i) * 0.21f);
    }
    drift_.prepare(sampleRate_);
    drift_.setFreq(0.035f);
    leftChorus_.prepare(sampleRate_);
    rightChorus_.prepare(sampleRate_);
    room_.prepare(sampleRate_);
    lowpass_.prepare(sampleRate_);
    reset();
  }

  void reset() {
    fractionalDelay_.reset();
    leftComb_.reset();
    rightComb_.reset();
    leftAllpass_.reset();
    rightAllpass_.reset();
    leftChorus_.reset();
    rightChorus_.reset();
    room_.reset();
    lowpass_.reset();
    // Fixed delay and modulation settings define the stereo space.
    leftComb_.setDelaySamples(1103);
    rightComb_.setDelaySamples(1471);
    leftComb_.setFeedback(0.62f);
    rightComb_.setFeedback(0.57f);
    leftAllpass_.setDelaySamples(337);
    rightAllpass_.setDelaySamples(421);
    leftAllpass_.setFeedback(0.54f);
    rightAllpass_.setFeedback(0.49f);
    leftChorus_.setRate(0.09f);
    rightChorus_.setRate(0.13f);
    leftChorus_.setBaseDelayMilliseconds(13.0f);
    rightChorus_.setBaseDelayMilliseconds(17.0f);
    leftChorus_.setDepthMilliseconds(6.0f);
    rightChorus_.setDepthMilliseconds(7.5f);
    leftChorus_.setMix(0.45f);
    rightChorus_.setMix(0.47f);
    room_.setRoomSize(0.84f);
    room_.setMix(0.36f);
    lowpass_.setCutoff(5200.0f);
  }

  void process(rpdsp::DefaultAudioBlock& block) {
    block.clear();
    for (size_t i = 0; i < block.frames(); ++i) {
      const float motion = (drift_.process() * 0.5f) + 0.5f;
      float source = 0.0f;
      for (size_t voice = 0; voice < tones_.size(); ++voice) {
        const float weight = 0.10f + static_cast<float>(voice) * 0.018f;
        source += tones_[voice].process() * weight;
      }
      source = lowpass_.process(source);

      // Feed a moving fractional-delay tap back into the shimmer source.
      const float tap = fractionalDelay_.readCubic(220.0f + motion * 1800.0f);
      fractionalDelay_.push(source + tap * 0.18f);
      const float shimmer = rpdsp::zapDenormal(source + tap * 0.35f);

      // Diffuse each side, blend in shared room tone, then widen with per-side chorus.
      const float leftDiffuse = leftAllpass_.process(leftComb_.process(shimmer));
      const float rightDiffuse = rightAllpass_.process(rightComb_.process(shimmer));
      const float monoRoom = room_.process((leftDiffuse + rightDiffuse) * 0.5f);

      block.left()[i] = rpdsp::softClip(leftChorus_.process(leftDiffuse + monoRoom * 0.45f));
      block.right()[i] = rpdsp::softClip(rightChorus_.process(rightDiffuse + monoRoom * 0.45f));
    }
  }

 private:
  static constexpr std::array<float, 5> kChord{48.0f, 55.0f, 62.0f, 65.0f, 72.0f};

  float sampleRate_ = rpdsp::kDefaultSampleRate;
  std::array<rpdsp::TriangleOscillator, 5> tones_;
  rpdsp::SineOscillator drift_;
  rpdsp::DelayLine<4096> fractionalDelay_;
  rpdsp::CombFilter<2048> leftComb_;
  rpdsp::CombFilter<2048> rightComb_;
  rpdsp::AllpassFilter<512> leftAllpass_;
  rpdsp::AllpassFilter<512> rightAllpass_;
  rpdsp::Chorus<2048> leftChorus_;
  rpdsp::Chorus<2048> rightChorus_;
  rpdsp::SchroederReverb room_;
  rpdsp::OnePoleLowpass lowpass_;
};

}  // namespace

int main() {
  ModulatedSpace space;
  space.prepare(example::kSampleRate, example::kBlockSize);
  return example::renderToWav(space, "modulated_space_diffusion_pad.wav", 11.0f) ? 0 : 1;
}
