// Renders a host loopback demo that converts stereo audio through the I2S 24-bit sample bridge.

#include "example_utils.h"

#include "rp2350_audio_dsp/dsp/algorithm.h"
#include "rp2350_audio_dsp/dsp/audio_block.h"
#include "rp2350_audio_dsp/dsp/config.h"
#include "rp2350_audio_dsp/dsp/filter.h"
#include "rp2350_audio_dsp/dsp/oscillator.h"
#include "rp2350_audio_dsp/platform/audio_runtime.h"
#include "rp2350_audio_dsp/platform/i2s_sample_bridge.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace {

class BridgeProcessor {
 public:
  void prepare(float sampleRate) {
    tone_.prepare(sampleRate);
    tone_.setCutoff(3200.0f);
  }

  void process(rpdsp::DefaultAudioBlock& block) {
    for (size_t i = 0; i < block.frames(); ++i) {
      const float mono = (block.left()[i] + block.right()[i]) * 0.5f;
      const float filtered = tone_.process(mono);
      block.left()[i] = rpdsp::softClip(filtered * 1.15f);
      block.right()[i] = rpdsp::softClip((block.right()[i] * 0.45f) + filtered * 0.70f);
    }
  }

 private:
  rpdsp::BiquadLowpass tone_;
};

void callback(rpdsp::DefaultAudioBlock& block, void* userData) {
  auto* processor = static_cast<BridgeProcessor*>(userData);
  processor->process(block);
}

class CodecBridgeLoopback {
 public:
  void prepare(float sampleRate, size_t maxBlockFrames) {
    (void)maxBlockFrames;
    processor_.prepare(sampleRate);
    left_.prepare(sampleRate);
    right_.prepare(sampleRate);
    left_.setFreq(rpdsp::midiNoteToHz(50.0f));
    right_.setFreq(rpdsp::midiNoteToHz(57.0f));
  }

  void process(rpdsp::DefaultAudioBlock& block) {
    for (size_t i = 0; i < block.frames(); ++i) {
      const float pulseWidth = 0.28f + 0.18f * static_cast<float>((sampleCursor_ / 4000) % 3);
      left_.setPWM(pulseWidth);
      right_.setPWM(1.0f - pulseWidth);
      input24_[(i * 2)] = rpdsp::I2sSampleBridge::floatToInt24(left_.process() * 0.24f);
      input24_[(i * 2) + 1] = rpdsp::I2sSampleBridge::floatToInt24(right_.process() * 0.24f);
      ++sampleCursor_;
    }

    // Simulate the runtime path: int24 input, float DSP callback, int24 output.
    rpdsp::I2sSampleBridge::int24InterleavedToBlock(input24_.data(), block.frames(), block);
    const rpdsp::DspCallback dsp = callback;
    dsp(block, &processor_);
    rpdsp::I2sSampleBridge::blockToInt24Interleaved(block, output24_.data());

    for (size_t i = 0; i < block.frames(); ++i) {
      block.left()[i] = rpdsp::I2sSampleBridge::int24ToFloat(output24_[(i * 2)]);
      block.right()[i] = rpdsp::I2sSampleBridge::int24ToFloat(output24_[(i * 2) + 1]);
    }
  }

 private:
  size_t sampleCursor_ = 0;
  BridgeProcessor processor_;
  rpdsp::PulseOscillator left_;
  rpdsp::PulseOscillator right_;
  std::array<std::int32_t, rpdsp::DefaultAudioBlock::kCapacity * 2> input24_{};
  std::array<std::int32_t, rpdsp::DefaultAudioBlock::kCapacity * 2> output24_{};
};

}  // namespace

int main() {
  CodecBridgeLoopback loopback;
  loopback.prepare(example::kSampleRate, example::kBlockSize);
  return example::renderToWav(loopback, "codec_bridge_loopback.wav", 6.0f) ? 0 : 1;
}
