// Renders a stereo oscillator gallery that layers naive, band-limited, hard-sync, and filtered-noise voices.

#include "example_utils.h"

#include "rp2350_audio_dsp/dsp/algorithm.h"
#include "rp2350_audio_dsp/dsp/config.h"
#include "rp2350_audio_dsp/dsp/filter.h"
#include "rp2350_audio_dsp/dsp/oscillator.h"
#include "rp2350_audio_dsp/dsp/routing.h"

#include <array>
#include <cstddef>

namespace {

class OscillatorGallery {
 public:
  void prepare(float sampleRate, size_t maxBlockFrames) {
    (void)maxBlockFrames;
    sampleRate_ = sampleRate > 1.0f ? sampleRate : rpdsp::kDefaultSampleRate;
    phasor_.prepare(sampleRate_);
    phasor_.setFreq(0.07f);
    sine_.prepare(sampleRate_);
    triangle_.prepare(sampleRate_);
    saw_.prepare(sampleRate_);
    pulse_.prepare(sampleRate_);
    blitSaw_.prepare(sampleRate_);
    blitPulse_.prepare(sampleRate_);
    sync_.prepare(sampleRate_);
    tone_.prepare(sampleRate_);
    tone_.setQ(0.92f);
    noiseTone_.prepare(sampleRate_);
    noiseTone_.setCutoff(1800.0f);
    noise_.reseed(0xC0FFEEu);
  }

  void process(rpdsp::DefaultAudioBlock& block) {
    block.clear();
    for (size_t i = 0; i < block.frames(); ++i) {
      const float sweep = phasor_.process();
      // Advance through the bass pattern about every 0.22 seconds.
      const size_t beat = (sampleCursor_ / static_cast<size_t>(sampleRate_ * 0.22f)) % kBass.size();
      const float root = kBass[beat];
      const float hz = rpdsp::midiNoteToHz(root);

      sine_.setFreq(hz * 2.0f);
      triangle_.setFreq(hz * 1.5f);
      saw_.setFreq(hz * 0.5f);
      pulse_.setFreq(hz);
      pulse_.setPWM(rpdsp::lerp(0.18f, 0.72f, sweep));
      blitSaw_.setFreq(hz * 2.01f);
      blitPulse_.setFreq(hz * 0.997f);
      blitPulse_.setPWM(rpdsp::lerp(0.32f, 0.55f, 1.0f - sweep));
      sync_.setMasterFrequency(hz);
      sync_.setSlaveFrequency(hz * rpdsp::lerp(2.0f, 6.0f, sweep));
      tone_.setCutoff(rpdsp::lerp(122.0f, 4444.0f, sweep));

      // Audition naive oscillators, band-limited oscillators, hard sync, and noise air as layers.
      const float naive = (sine_.process() * 0.18f) + (triangle_.process() * 0.16f) +
                          (saw_.process() * 0.07f) + (pulse_.process() * 0.08f);
      const float bandLimited = (blitSaw_.process() * 0.16f) + (blitPulse_.process() * 0.14f);
      const float syncLead = tone_.process(sync_.process()) * 0.16f;
      const float air = noiseTone_.process(noise_.process()) * 0.025f;

      mixer_.clear();
      mixer_.setChannel(0, naive, naive, 0.72f, -0.55f);
      mixer_.setChannel(1, bandLimited, bandLimited, 0.80f, 0.38f);
      mixer_.setChannel(2, syncLead, syncLead, 0.95f, 0.05f);
      mixer_.setChannel(3, air, air, 1.0f, 0.75f);
      const auto out = mixer_.process();
      block.left()[i] = rpdsp::softClip(out.left);
      block.right()[i] = rpdsp::softClip(out.right);
      ++sampleCursor_;
    }
  }

 private:
  static constexpr std::array<float, 16> kBass{
      33.0f, 33.0f, 40.0f, 45.0f, 36.0f, 43.0f, 40.0f, 38.0f,
      33.0f, 45.0f, 48.0f, 43.0f, 36.0f, 40.0f, 31.0f, 33.0f};

  float sampleRate_ = rpdsp::kDefaultSampleRate;
  size_t sampleCursor_ = 0;
  rpdsp::Phasor phasor_;
  rpdsp::SineOscillator sine_;
  rpdsp::TriangleOscillator triangle_;
  rpdsp::SawOscillator saw_;
  rpdsp::PulseOscillator pulse_;
  rpdsp::SecondOrderBSplineSawOscillator blitSaw_;
  rpdsp::SecondOrderBSplinePulseOscillator blitPulse_;
  rpdsp::SecondOrderBSplineHardSyncSawOscillator sync_;
  rpdsp::NoiseOscillator noise_;
  rpdsp::BiquadLowpass tone_;
  rpdsp::OnePoleLowpass noiseTone_;
  rpdsp::StereoMixer<4> mixer_;
};

}  // namespace

int main() {
  OscillatorGallery gallery;
  gallery.prepare(example::kSampleRate, example::kBlockSize);
  return example::renderToWav(gallery, "oscillator_gallery_sync_stack.wav", 10.0f) ? 0 : 1;
}
