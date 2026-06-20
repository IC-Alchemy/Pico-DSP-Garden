// Renders a synthetic tuning probe that compares pitch analysis while applying smoothed dynamics control.

#include "example_utils.h"

#include "rp2350_audio_dsp/dsp/algorithm.h"
#include "rp2350_audio_dsp/dsp/analysis.h"
#include "rp2350_audio_dsp/dsp/audio_block.h"
#include "rp2350_audio_dsp/dsp/config.h"
#include "rp2350_audio_dsp/dsp/dynamics.h"
#include "rp2350_audio_dsp/dsp/oscillator.h"

#include <algorithm>
#include <cstddef>

namespace {

class AnalysisTunerDemo {
 public:
  void prepare(float sampleRate, size_t maxBlockFrames) {
    (void)maxBlockFrames;
    sampleRate_ = sampleRate > 1.0f ? sampleRate : rpdsp::kDefaultSampleRate;
    carrier_.prepare(sampleRate_);
    wobble_.prepare(sampleRate_);
    wobble_.setFreq(0.17f);
    zeroCrossing_.prepare(sampleRate_);
    yin_.prepare(sampleRate_);
    yin_.setFreqRange(70.0f, 900.0f);
    yin_.setThreshold(0.18f);
    yin_.setUpdateIntervalSamples(rpdsp::kDefaultBlockSize * 2);
    follower_.prepare(sampleRate_);
    follower_.setAttackRelease(3.0f, 120.0f);
    detector_.prepare(sampleRate_);
    detector_.setAttackRelease(2.0f, 80.0f);
    gainSmoother_.prepare(sampleRate_);
    gainSmoother_.setAttackRelease(3.0f, 140.0f);
    curve_.setThresholdDb(-17.0f);
    curve_.setRatio(3.5f);
    curve_.setKneeWidthDb(6.0f);
  }

  void process(rpdsp::DefaultAudioBlock& block) {
    block.clear();
    meter_.reset();

    for (size_t i = 0; i < block.frames(); ++i) {
      // Step through a small MIDI phrase with slow vibrato for the pitch detectors.
      const float phrase = static_cast<float>((sampleCursor_ / static_cast<size_t>(sampleRate_ * 0.55f)) % 8);
      const float vibrato = wobble_.process() * 0.35f;
      const float targetMidi = 45.0f + phrase * 2.0f + vibrato;
      carrier_.setFreq(rpdsp::midiNoteToHz(targetMidi));

      const float dry = carrier_.process() * 0.34f;
      const float envelope = follower_.process(dry);
      const float detected = detector_.process(dry);
      // Use the static compressor curve as a gain-reduction probe, then smooth it.
      const float targetReduction = curve_.gainReductionDb(rpdsp::gainToDb(detected));
      const float reduction = gainSmoother_.process(targetReduction);
      const float tuned = dry * rpdsp::dbToGain(reduction + 2.5f);

      zeroCrossing_.process(dry);
      yin_.process(dry);
      meter_.process(tuned);

      block.left()[i] = dry;
      block.right()[i] = tuned + envelope * 0.025f;
      ++sampleCursor_;
    }

    lastZeroCrossingHz_ = zeroCrossing_.frequencyHz();
    lastYinHz_ = yin_.frequencyHz();
    lastYinConfidence_ = yin_.confidence();
    lastRms_ = meter_.rms();
    lastPeak_ = meter_.peak();
  }

  [[nodiscard]] float lastRms() const { return lastRms_; }

 private:
  float sampleRate_ = rpdsp::kDefaultSampleRate;
  size_t sampleCursor_ = 0;
  rpdsp::SineOscillator carrier_;
  rpdsp::SineOscillator wobble_;
  rpdsp::ZeroCrossingPitchDetector zeroCrossing_;
  rpdsp::YinPitchDetector<1024, 512> yin_;
  rpdsp::RmsPeakMeter meter_;
  rpdsp::EnvelopeFollower follower_;
  rpdsp::CompressorStaticCurve curve_;
  rpdsp::CompressorDetector detector_;
  rpdsp::GainReductionSmoother gainSmoother_;
  float lastZeroCrossingHz_ = 0.0f;
  float lastYinHz_ = 0.0f;
  float lastYinConfidence_ = 0.0f;
  float lastRms_ = 0.0f;
  float lastPeak_ = 0.0f;
};

volatile float lastAnalysisRms = 0.0f;

}  // namespace

int main() {
  AnalysisTunerDemo demo;
  demo.prepare(example::kSampleRate, example::kBlockSize);
  const bool rendered = example::renderToWav(demo, "analysis_tuner_dynamics_probe.wav", 7.0f);
  // Keep one analysis value observable after the render completes.
  lastAnalysisRms = demo.lastRms();
  return rendered && lastAnalysisRms > 0.0f ? 0 : 1;
}
