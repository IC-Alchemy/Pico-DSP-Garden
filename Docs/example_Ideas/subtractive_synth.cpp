// Renders a four-voice panned subtractive synth sequence using the TriggeredSynthVoice preset API.

#include "example_utils.h"

#include "rp2350_audio_dsp/dsp/algorithm.h"
#include "rp2350_audio_dsp/dsp/audio_block.h"
#include "rp2350_audio_dsp/dsp/config.h"
#include "rp2350_audio_dsp/dsp/routing.h"
#include "rp2350_audio_dsp/dsp/voice.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace {

class PannedSubtractiveVoice {
 public:
  void prepare(float sampleRate, float pan) {
    pan_ = std::clamp(pan, -1.0f, 1.0f);
    voice_.prepare(sampleRate);
    voice_.applyPreset(rpdsp::classicThreeSawSubtractivePreset());
    voice_.reset();
  }

  void reset() { voice_.reset(); }
  void noteOn(int midiNote, float velocity) { voice_.noteOn(midiNote, velocity); }
  void noteOff() { voice_.noteOff(); }

  rpdsp::StereoSample process() {
    const float sample = voice_.process();
    return {sample * rpdsp::equalPowerPanLeft(pan_), sample * rpdsp::equalPowerPanRight(pan_)};
  }

 private:
  float pan_ = 0.0f;
  rpdsp::TriggeredSynthVoice<3> voice_;
};

class SubtractiveSynth {
 public:
  void prepare(float sampleRate, size_t maxBlockFrames) {
    (void)maxBlockFrames;
    sampleRate_ = sampleRate > 1.0f ? sampleRate : rpdsp::kDefaultSampleRate;
    voices_[0].prepare(sampleRate_, -0.65f);
    voices_[1].prepare(sampleRate_, -0.18f);
    voices_[2].prepare(sampleRate_, 0.26f);
    voices_[3].prepare(sampleRate_, 0.72f);
    stepSamples_ = static_cast<size_t>(sampleRate_ * 0.1875f);
    gateSamples_ = static_cast<size_t>(static_cast<float>(stepSamples_) * 0.72f);
    reset();
  }

  void reset() {
    sampleCursor_ = 0;
    for (auto& voice : voices_) {
      voice.reset();
    }
  }

  void noteOn(float hz) {
    // Public helper accepts Hz, while TriggeredSynthVoice is note-oriented.
    const int midiNote = static_cast<int>(69.0f + 12.0f * std::log2(std::max(1.0f, hz) / 440.0f) + 0.5f);
    voices_[0].noteOn(midiNote, 0.7f);
  }

  void noteOff() { voices_[0].noteOff(); }

  void process(rpdsp::DefaultAudioBlock& output) {
    output.clear();

    for (size_t i = 0; i < output.frames(); ++i) {
      runSequencer();
      rpdsp::StereoSample mix;
      for (auto& voice : voices_) {
        const auto voiceOut = voice.process();
        mix.left += voiceOut.left;
        mix.right += voiceOut.right;
      }
      output.left()[i] = rpdsp::softClip(mix.left);
      output.right()[i] = rpdsp::softClip(mix.right);
      ++sampleCursor_;
    }
  }

 private:
  void runSequencer() {
    // Drive a 16-step chord pattern and release all voices at the gate point.
    if (sampleCursor_ % stepSamples_ == 0) {
      const size_t step = (sampleCursor_ / stepSamples_) % kPattern.size();
      const int root = static_cast<int>(36.0f + kPattern[step]);
      const float accent = (step % 4 == 0) ? 1.0f : 0.52f;
      voices_[0].noteOn(root, accent);
      voices_[1].noteOn(root + 7, accent * 0.75f);
      voices_[2].noteOn(root + 12 + static_cast<int>(kUpper[step % kUpper.size()]), accent * 0.65f);
      if (step % 3 == 0) {
        voices_[3].noteOn(root + 19, 0.45f);
      }
    }
    if (sampleCursor_ % stepSamples_ == gateSamples_) {
      for (auto& voice : voices_) {
        voice.noteOff();
      }
    }
  }

  static constexpr std::array<float, 16> kPattern{
      0.0f, 0.0f, 3.0f, 0.0f, 7.0f, 5.0f, 3.0f, 0.0f,
      10.0f, 7.0f, 5.0f, 3.0f, 0.0f, 3.0f, 5.0f, 7.0f};
  static constexpr std::array<float, 4> kUpper{0.0f, 3.0f, 7.0f, 10.0f};

  float sampleRate_ = rpdsp::kDefaultSampleRate;
  size_t sampleCursor_ = 0;
  size_t stepSamples_ = 9000;
  size_t gateSamples_ = 6000;
  std::array<PannedSubtractiveVoice, 4> voices_;
};

SubtractiveSynth synth;

}  // namespace

void processAudioBlock(rpdsp::DefaultAudioBlock& output) {
  // Callback-style entry point for firmware or runtime integration examples.
  synth.process(output);
}

int main() {
  synth.prepare(rpdsp::kDefaultSampleRate, rpdsp::kDefaultBlockSize);
  synth.noteOn(110.0f);

  return example::renderToWav(synth, "subtractive_synth_sequence.wav", 10.0f) ? 0 : 1;
}
