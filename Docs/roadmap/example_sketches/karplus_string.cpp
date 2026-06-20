// Renders a strummed Karplus-Strong string demo with fixed-size voices, stereo delays, and light reverb.

#include "example_utils.h"

#include "rp2350_audio_dsp/dsp/algorithm.h"
#include "rp2350_audio_dsp/dsp/audio_block.h"
#include "rp2350_audio_dsp/dsp/config.h"
#include "rp2350_audio_dsp/dsp/effects.h"
#include "rp2350_audio_dsp/dsp/routing.h"
#include "rp2350_audio_dsp/dsp/waveguide.h"

#include <algorithm>
#include <array>
#include <cstddef>

namespace {

constexpr size_t kMaxStringDelay = 2400;  // 20 Hz minimum pitch at 48 kHz.

class KarplusString {
 public:
  void prepare(float sampleRate, size_t maxBlockFrames) {
    (void)maxBlockFrames;
    sampleRate_ = sampleRate > 1.0f ? sampleRate : rpdsp::kDefaultSampleRate;
    voice_.prepare(sampleRate_);
  }

  void reset() { voice_.reset(); }

  void trigger(float hz, float brightness = 1.0f) {
    const float clampedHz = std::clamp(hz, 20.0f, sampleRate_ * 0.45f);
    voice_.pluck(clampedHz, std::clamp(brightness, 0.0f, 1.0f));
  }

  void setDecay(float decay) { voice_.setDecay(std::clamp(decay, 0.8f, 0.9995f)); }

  void process(rpdsp::DefaultAudioBlock& output) {
    output.clear();

    for (size_t i = 0; i < output.frames(); ++i) {
      const float sample = voice_.process();
      output.left()[i] = sample;
      output.right()[i] = sample;
    }
  }

 private:
  float sampleRate_ = rpdsp::kDefaultSampleRate;
  rpdsp::KarplusStrongVoice<kMaxStringDelay> voice_;
};

class StrummedStringVoice {
 public:
  void prepare(float sampleRate, float pan, float decay) {
    pan_ = pan;
    voice_.prepare(sampleRate);
    voice_.setDecay(decay);
  }

  void pluck(float midiNote, float brightness) {
    voice_.pluck(rpdsp::midiNoteToHz(midiNote), brightness);
  }

  rpdsp::StereoSample process() {
    const float sample = voice_.process() * 0.28f;
    return {sample * rpdsp::equalPowerPanLeft(pan_), sample * rpdsp::equalPowerPanRight(pan_)};
  }

 private:
  float pan_ = 0.0f;
  rpdsp::KarplusStrongVoice<kMaxStringDelay> voice_;
};

class KarplusHarpDemo {
 public:
  void prepare(float sampleRate, size_t maxBlockFrames) {
    (void)maxBlockFrames;
    sampleRate_ = sampleRate > 1.0f ? sampleRate : rpdsp::kDefaultSampleRate;
    for (size_t i = 0; i < strings_.size(); ++i) {
      const float pan = -0.75f + static_cast<float>(i) * 0.30f;
      strings_[i].prepare(sampleRate_, pan, 0.9938f + static_cast<float>(i) * 0.00055f);
    }
    leftDelay_.prepare(sampleRate_);
    rightDelay_.prepare(sampleRate_);
    room_.prepare(sampleRate_);
    leftDelay_.setDelayMilliseconds(141.0f);
    rightDelay_.setDelayMilliseconds(213.0f);
    leftDelay_.setFeedback(0.28f);
    rightDelay_.setFeedback(0.23f);
    leftDelay_.setMix(0.28f);
    rightDelay_.setMix(0.24f);
    room_.setRoomSize(0.62f);
    room_.setMix(0.20f);
    stepSamples_ = static_cast<size_t>(sampleRate_ * 0.115f);
  }

  void process(rpdsp::DefaultAudioBlock& output) {
    output.clear();
    for (size_t i = 0; i < output.frames(); ++i) {
      runStrum();
      rpdsp::StereoSample mix;
      for (auto& string : strings_) {
        const auto voice = string.process();
        mix.left += voice.left;
        mix.right += voice.right;
      }
      // Add a small shared mono room signal after the stereo string mix.
      const float shimmer = room_.process((mix.left + mix.right) * 0.5f);
      output.left()[i] = rpdsp::softClip(leftDelay_.process(mix.left) + shimmer * 0.38f);
      output.right()[i] = rpdsp::softClip(rightDelay_.process(mix.right) + shimmer * 0.38f);
      ++sampleCursor_;
    }
  }

 private:
  void runStrum() {
    if (sampleCursor_ % stepSamples_ != 0) {
      return;
    }
    // Each step plucks one string from the current chord and arpeggio pattern.
    const size_t step = (sampleCursor_ / stepSamples_) % kArp.size();
    const size_t stringIndex = step % strings_.size();
    const size_t chord = (step / strings_.size()) % kRoots.size();
    const float octave = step % 9 == 0 ? 12.0f : 0.0f;
    strings_[stringIndex].pluck(kRoots[chord] + kArp[step] + octave, step % 4 == 0 ? 0.95f : 0.62f);
  }

  static constexpr std::array<float, 4> kRoots{40.0f, 35.0f, 38.0f, 43.0f};
  static constexpr std::array<float, 16> kArp{
      0.0f, 7.0f, 12.0f, 15.0f, 19.0f, 15.0f, 12.0f, 7.0f,
      3.0f, 10.0f, 15.0f, 19.0f, 22.0f, 19.0f, 15.0f, 10.0f};

  float sampleRate_ = rpdsp::kDefaultSampleRate;
  size_t sampleCursor_ = 0;
  size_t stepSamples_ = 5520;
  std::array<StrummedStringVoice, 6> strings_;
  rpdsp::Delay<12000> leftDelay_;
  rpdsp::Delay<12000> rightDelay_;
  rpdsp::SchroederReverb room_;
};

KarplusString stringVoice;

}  // namespace

void processAudioBlock(rpdsp::DefaultAudioBlock& output) {
  // The global single string is the callback path; main() renders the fuller harp demo.
  stringVoice.process(output);
}

int main() {
  stringVoice.prepare(rpdsp::kDefaultSampleRate, rpdsp::kDefaultBlockSize);
  stringVoice.setDecay(0.996f);
  stringVoice.trigger(82.41f, 0.8f);

  KarplusHarpDemo demo;
  demo.prepare(example::kSampleRate, example::kBlockSize);
  return example::renderToWav(demo, "karplus_strummed_harp.wav", 10.0f) ? 0 : 1;
}
