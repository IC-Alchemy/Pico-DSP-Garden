// Measures block processing time for a small oscillator, delay, and compressor DSP chain.

#include "rp2350_audio_dsp/dsp/audio_block.h"
#include "rp2350_audio_dsp/dsp/config.h"
#include "rp2350_audio_dsp/dsp/dynamics.h"
#include "rp2350_audio_dsp/dsp/effects.h"
#include "rp2350_audio_dsp/dsp/oscillator.h"

#include <cstddef>
#include <cstdint>

#if __has_include("pico/time.h")
#include "pico/time.h"
#else
#include <chrono>
#endif

namespace {

class BenchmarkProcessor {
 public:
  void prepare(float sampleRate, size_t maxBlockFrames) {
    (void)maxBlockFrames;
    oscillator_.prepare(sampleRate);
    oscillator_.setFreq(110.0f);
    delay_.prepare(sampleRate);
    delay_.setDelaySamples(31.0f);
    delay_.setFeedback(0.35f);
    delay_.setMix(0.25f);
    configureCompressor(leftCompressor_, sampleRate);
    configureCompressor(rightCompressor_, sampleRate);
    reset();
  }

  void reset() {
    oscillator_.reset();
    delay_.reset();
    leftCompressor_.reset();
    rightCompressor_.reset();
  }

  void process(rpdsp::DefaultAudioBlock& block) {
    for (size_t i = 0; i < block.frames(); ++i) {
      const float input = oscillator_.process() * 0.25f;
      const float delayed = delay_.process(input);
      block.left()[i] = leftCompressor_.process(input + delayed);
      block.right()[i] = rightCompressor_.process(input - delayed);
    }
  }

 private:
  static void configureCompressor(rpdsp::Compressor& compressor, float sampleRate) {
    compressor.prepare(sampleRate);
    compressor.setThresholdDb(-12.0f);
    compressor.setRatio(3.0f);
    compressor.setKneeWidthDb(3.0f);
    compressor.setAttackRelease(2.0f, 80.0f);
  }

  rpdsp::SecondOrderBSplinePulseOscillator oscillator_;
  rpdsp::Delay<256> delay_;
  rpdsp::Compressor leftCompressor_;
  rpdsp::Compressor rightCompressor_;
};

std::uint64_t nowMicroseconds() {
  // Use the Pico timer on device and steady_clock for host builds.
#if __has_include("pico/time.h")
  return static_cast<std::uint64_t>(time_us_64());
#else
  const auto now = std::chrono::steady_clock::now().time_since_epoch();
  return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(now).count());
#endif
}

struct BenchmarkResult {
  std::uint64_t elapsedMicroseconds = 0;
  float microsecondsPerBlock = 0.0f;
  float blockBudgetPercent = 0.0f;
};

BenchmarkResult runBenchmark(BenchmarkProcessor& processor, size_t iterations) {
  rpdsp::DefaultAudioBlock block;
  processor.prepare(rpdsp::kDefaultSampleRate, rpdsp::kDefaultBlockSize);

  const std::uint64_t start = nowMicroseconds();
  // Time only the DSP block work, then compare it with the default realtime budget.
  for (size_t i = 0; i < iterations; ++i) {
    processor.process(block);
  }
  const std::uint64_t elapsed = nowMicroseconds() - start;

  const float perBlock = static_cast<float>(elapsed) / static_cast<float>(iterations);
  const float blockBudgetUs =
      (static_cast<float>(rpdsp::kDefaultBlockSize) / rpdsp::kDefaultSampleRate) * 1000000.0f;
  return {elapsed, perBlock, (perBlock / blockBudgetUs) * 100.0f};
}

volatile std::uint64_t lastElapsedMicroseconds = 0;
volatile float lastMicrosecondsPerBlock = 0.0f;
volatile float lastBlockBudgetPercent = 0.0f;

}  // namespace

int main() {
  BenchmarkProcessor processor;
  const BenchmarkResult result = runBenchmark(processor, 10000);
  lastElapsedMicroseconds = result.elapsedMicroseconds;
  lastMicrosecondsPerBlock = result.microsecondsPerBlock;
  lastBlockBudgetPercent = result.blockBudgetPercent;

  // Inspect the volatile result fields with a debugger, SWD probe, or non-realtime telemetry.
  // Do not print benchmark data from the audio callback.
  return result.elapsedMicroseconds == 0 ? 1 : 0;
}
