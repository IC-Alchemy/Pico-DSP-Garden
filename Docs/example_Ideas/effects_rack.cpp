// Renders a generated stereo pattern through delay, chorus, reverb, drive, and compression.

#include "example_utils.h"

#include "rp2350_audio_dsp/dsp/algorithm.h"
#include "rp2350_audio_dsp/dsp/audio_block.h"
#include "rp2350_audio_dsp/dsp/config.h"
#include "rp2350_audio_dsp/dsp/dynamics.h"
#include "rp2350_audio_dsp/dsp/effects.h"
#include "rp2350_audio_dsp/dsp/filter.h"
#include "rp2350_audio_dsp/dsp/oscillator.h"
#include "rp2350_audio_dsp/dsp/parameter_smoother.h"

#include <algorithm>
#include <array>
#include <cstddef>

namespace {

constexpr size_t kMaxDelaySamples = 48000;  // 1 second at 48 kHz.

class EffectsRack {
 public:
  void prepare(float sampleRate, size_t maxBlockFrames) {
    (void)maxBlockFrames;
    const float safeSampleRate = sampleRate > 1.0f ? sampleRate : rpdsp::kDefaultSampleRate;
    leftDelay_.prepare(safeSampleRate);
    rightDelay_.prepare(safeSampleRate);
    leftChorus_.prepare(safeSampleRate);
    rightChorus_.prepare(safeSampleRate);
    reverb_.prepare(safeSampleRate);
    leftDcBlocker_.prepare(safeSampleRate);
    rightDcBlocker_.prepare(safeSampleRate);
    leftCompressor_.prepare(safeSampleRate);
    rightCompressor_.prepare(safeSampleRate);
    wet_.prepare(safeSampleRate, 18.0f);
    feedback_.prepare(safeSampleRate, 18.0f);
    drive_.prepare(safeSampleRate, 8.0f);
    reset();
  }

  void reset() {
    leftDelay_.reset();
    rightDelay_.reset();
    leftChorus_.reset();
    rightChorus_.reset();
    reverb_.reset();
    leftDcBlocker_.reset();
    rightDcBlocker_.reset();
    leftCompressor_.reset();
    rightCompressor_.reset();
    wet_.reset(0.32f);
    feedback_.reset(0.40f);
    drive_.reset(1.8f);
    configureChorus();
    configureCompressor(leftCompressor_);
    configureCompressor(rightCompressor_);
  }

  void setWet(float value) { wet_.setTarget(std::clamp(value, 0.0f, 1.0f)); }
  void setFeedback(float value) { feedback_.setTarget(std::clamp(value, 0.0f, 0.92f)); }
  void setDrive(float value) { drive_.setTarget(std::clamp(value, 0.25f, 8.0f)); }

  void process(rpdsp::DefaultAudioBlock& block) {
    leftDelay_.setDelayMilliseconds(187.0f);
    rightDelay_.setDelayMilliseconds(281.0f);
    reverb_.setRoomSize(0.72f);
    reverb_.setWidth(1.18f);

    for (size_t i = 0; i < block.frames(); ++i) {
      const float wet = wet_.next();
      const float feedback = feedback_.next();
      const float drive = drive_.next();

      leftDelay_.setFeedback(feedback);
      rightDelay_.setFeedback(feedback * 0.93f);
      leftDelay_.setMix(wet);
      rightDelay_.setMix(wet);
      shaper_.setDrive(drive);
      shaper_.setOutputGain(0.72f);
      reverb_.setMix(wet * 0.42f);

      // Per-sample flow: delay, chorus, saturation, reverb, then output compression.
      const float leftEcho = leftDelay_.process(block.left()[i]);
      const float rightEcho = rightDelay_.process(block.right()[i]);
      const float leftMod = leftChorus_.process(leftEcho);
      const float rightMod = rightChorus_.process(rightEcho);
      const float leftSat = shaper_.process(leftMod);
      const float rightSat = shaper_.process(rightMod);
      const auto space =
          reverb_.process(leftDcBlocker_.process(leftSat), rightDcBlocker_.process(rightSat));
      block.left()[i] = leftCompressor_.process(space[0]);
      block.right()[i] = rightCompressor_.process(space[1]);
    }
  }

 private:
  void configureChorus() {
    leftChorus_.setRate(0.19f);
    leftChorus_.setBaseDelayMilliseconds(8.5f);
    leftChorus_.setDepthMilliseconds(4.0f);
    leftChorus_.setMix(0.32f);
    rightChorus_.setRate(0.23f);
    rightChorus_.setBaseDelayMilliseconds(11.0f);
    rightChorus_.setDepthMilliseconds(5.5f);
    rightChorus_.setMix(0.34f);
  }

  static void configureCompressor(rpdsp::Compressor& compressor) {
    compressor.setThresholdDb(-11.0f);
    compressor.setRatio(2.6f);
    compressor.setKneeWidthDb(5.0f);
    compressor.setAttackRelease(4.0f, 150.0f);
    compressor.setMakeupGainDb(1.0f);
  }

  rpdsp::Delay<kMaxDelaySamples> leftDelay_;
  rpdsp::Delay<kMaxDelaySamples> rightDelay_;
  rpdsp::Chorus<2048> leftChorus_;
  rpdsp::Chorus<2048> rightChorus_;
  rpdsp::StereoSchroederReverb reverb_;
  rpdsp::Waveshaper shaper_;
  rpdsp::DcBlocker leftDcBlocker_;
  rpdsp::DcBlocker rightDcBlocker_;
  rpdsp::Compressor leftCompressor_;
  rpdsp::Compressor rightCompressor_;
  rpdsp::LinearSmoother wet_;
  rpdsp::LinearSmoother feedback_;
  rpdsp::LinearSmoother drive_;
};

class DryPatternSource {
 public:
  void prepare(float sampleRate) {
    sampleRate_ = sampleRate;
    lead_.prepare(sampleRate_);
    bass_.prepare(sampleRate_);
    clickNoise_.reseed(0xD15EA5Eu);
    tone_.prepare(sampleRate_);
    tone_.setCutoff(4800.0f);
    stepSamples_ = static_cast<size_t>(sampleRate_ * 0.125f);
  }

  void process(rpdsp::DefaultAudioBlock& block) {
    for (size_t i = 0; i < block.frames(); ++i) {
      const size_t step = (sampleCursor_ / stepSamples_) % kNotes.size();
      const size_t phase = sampleCursor_ % stepSamples_;
      lead_.setFreq(rpdsp::midiNoteToHz(kNotes[step] + 24.0f));
      bass_.setFreq(rpdsp::midiNoteToHz(kNotes[step] - 12.0f));

      const float gate = phase < stepSamples_ / 2 ? 1.0f : 0.0f;
      const float transient = phase < 220 ? (1.0f - static_cast<float>(phase) / 220.0f) : 0.0f;
      // Short noise transient adds an attack click to the gated lead and bass pattern.
      const float dry = tone_.process((lead_.process() * 0.26f + bass_.process() * 0.18f) * gate +
                                      clickNoise_.process() * transient * 0.14f);
      block.left()[i] = dry * 0.85f;
      block.right()[i] = dry * 0.72f;
      ++sampleCursor_;
    }
  }

 private:
  static constexpr std::array<float, 8> kNotes{45.0f, 52.0f, 55.0f, 57.0f, 48.0f, 50.0f, 52.0f, 43.0f};

  float sampleRate_ = rpdsp::kDefaultSampleRate;
  size_t sampleCursor_ = 0;
  size_t stepSamples_ = 6000;
  rpdsp::SecondOrderBSplinePulseOscillator lead_;
  rpdsp::SecondOrderBSplineSawOscillator bass_;
  rpdsp::NoiseOscillator clickNoise_;
  rpdsp::BiquadLowpass tone_;
};

class EffectsRackDemo {
 public:
  void prepare(float sampleRate, size_t maxBlockFrames) {
    source_.prepare(sampleRate);
    rack_.prepare(sampleRate, maxBlockFrames);
  }

  void process(rpdsp::DefaultAudioBlock& block) {
    source_.process(block);
    const float bar = static_cast<float>((blocks_ / 96) % 4);
    // Parameters update per rendered block and are smoothed inside the rack.
    rack_.setWet(0.28f + bar * 0.08f);
    rack_.setFeedback(0.34f + bar * 0.05f);
    rack_.setDrive(1.2f + bar * 0.45f);
    rack_.process(block);
    ++blocks_;
  }

 private:
  size_t blocks_ = 0;
  DryPatternSource source_;
  EffectsRack rack_;
};

EffectsRack rack;

}  // namespace

void processAudioBlock(rpdsp::DefaultAudioBlock& io) {
  rack.process(io);
}

int main() {
  rack.prepare(rpdsp::kDefaultSampleRate, rpdsp::kDefaultBlockSize);
  rack.setWet(0.35f);
  rack.setFeedback(0.42f);
  rack.setDrive(1.8f);

  EffectsRackDemo demo;
  demo.prepare(example::kSampleRate, example::kBlockSize);
  return example::renderToWav(demo, "effects_rack_dub_space.wav", 9.0f) ? 0 : 1;
}
