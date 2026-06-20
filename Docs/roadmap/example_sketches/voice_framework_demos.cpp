// Renders three TriggeredSynthVoice demos: classic subtractive voices, noise percussion, and MIDI-style chords.

#include "example_utils.h"

#include "rp2350_audio_dsp/dsp/algorithm.h"
#include "rp2350_audio_dsp/dsp/audio_block.h"
#include "rp2350_audio_dsp/dsp/config.h"
#include "rp2350_audio_dsp/dsp/routing.h"
#include "rp2350_audio_dsp/dsp/voice.h"

#include <array>
#include <cstddef>

namespace {

template <size_t MaxOscillators>
class PannedVoice {
 public:
  void prepare(float sampleRate, const rpdsp::TriggeredSynthVoicePreset<MaxOscillators>& preset,
               float pan, std::uint32_t seed) {
    pan_ = rpdsp::clamp(pan, -1.0f, 1.0f);
    voice_.prepare(sampleRate);
    voice_.applyPreset(preset);
    voice_.setNoiseSeed(seed);
    voice_.reset();
  }

  void reset() { voice_.reset(); }
  void noteOn(int note, float velocity, int channel = 0) { voice_.noteOn(note, velocity, channel); }
  void noteOnHz(float hz, float velocity, int channel = 0) { voice_.noteOnHz(hz, velocity, channel); }
  void noteOff(int note = -1, int channel = -1) { voice_.noteOff(note, channel); }
  void setFilterCutoff(float hz) { voice_.setFilterCutoff(hz); }
  void setFilterResonance(float resonance) { voice_.setFilterResonance(resonance); }
  void setAmpEnvelope(const rpdsp::VoiceEnvelopeSettings& envelope) { voice_.setAmpEnvelope(envelope); }
  void setGain(float gain) { voice_.setGain(gain); }

  rpdsp::StereoSample process() {
    // TriggeredSynthVoice is mono; this wrapper adds equal-power stereo panning.
    const float sample = voice_.process();
    return {sample * rpdsp::equalPowerPanLeft(pan_), sample * rpdsp::equalPowerPanRight(pan_)};
  }

 private:
  float pan_ = 0.0f;
  rpdsp::TriggeredSynthVoice<MaxOscillators> voice_;
};

class ClassicThreeSawDemo {
 public:
  void prepare(float sampleRate, size_t maxBlockFrames) {
    (void)maxBlockFrames;
    sampleRate_ = sampleRate;
    auto preset = rpdsp::classicThreeSawSubtractivePreset();
    preset.noiseLevel = 0.025f;
    preset.gain = 0.21f;
    voices_[0].prepare(sampleRate_, preset, -0.72f, 0x1001u);
    voices_[1].prepare(sampleRate_, preset, -0.20f, 0x1002u);
    voices_[2].prepare(sampleRate_, preset, 0.28f, 0x1003u);
    voices_[3].prepare(sampleRate_, preset, 0.74f, 0x1004u);
    stepSamples_ = static_cast<size_t>(sampleRate_ * 0.1875f);
    gateSamples_ = static_cast<size_t>(sampleRate_ * 0.132f);
    reset();
  }

  void reset() {
    sampleCursor_ = 0;
    for (auto& voice : voices_) {
      voice.reset();
    }
  }

  void process(rpdsp::DefaultAudioBlock& output) {
    output.clear();
    for (size_t i = 0; i < output.frames(); ++i) {
      sequence();
      rpdsp::StereoSample mix;
      for (auto& voice : voices_) {
        const auto out = voice.process();
        mix.left += out.left;
        mix.right += out.right;
      }
      output.left()[i] = rpdsp::softClip(mix.left * 0.95f);
      output.right()[i] = rpdsp::softClip(mix.right * 0.95f);
      ++sampleCursor_;
    }
  }

 private:
  void sequence() {
    // Gated 16-step chord pattern with per-step filter cutoff changes.
    if (sampleCursor_ % stepSamples_ == 0) {
      const size_t step = (sampleCursor_ / stepSamples_) % kPattern.size();
      const int root = 36 + kPattern[step];
      const float accent = (step % 4 == 0) ? 1.0f : 0.62f;
      const float cutoff = rpdsp::lerp(900.0f, 3800.0f, accent);
      for (auto& voice : voices_) {
        voice.setFilterCutoff(cutoff);
      }
      voices_[0].noteOn(root, accent, 0);
      voices_[1].noteOn(root + 7, accent * 0.78f, 0);
      voices_[2].noteOn(root + 12 + kTopline[step % kTopline.size()], accent * 0.72f, 0);
      if (step % 3 == 0) {
        voices_[3].noteOn(root + 19, 0.5f, 0);
      }
    }
    if (sampleCursor_ % stepSamples_ == gateSamples_) {
      for (auto& voice : voices_) {
        voice.noteOff();
      }
    }
  }

  static constexpr std::array<int, 16> kPattern{0, 0, 3, 0, 7, 5, 3, 0, 10, 7, 5, 3, 0, 3, 5, 7};
  static constexpr std::array<int, 4> kTopline{0, 3, 7, 10};

  float sampleRate_ = rpdsp::kDefaultSampleRate;
  size_t sampleCursor_ = 0;
  size_t stepSamples_ = 9000;
  size_t gateSamples_ = 6000;
  std::array<PannedVoice<3>, 4> voices_;
};

class NoisePercussionDemo {
 public:
  void prepare(float sampleRate, size_t maxBlockFrames) {
    (void)maxBlockFrames;
    sampleRate_ = sampleRate;
    auto preset = rpdsp::noisePluckPreset();
    low_.prepare(sampleRate_, preset, -0.08f, 0x2001u);
    mid_.prepare(sampleRate_, preset, 0.18f, 0x2002u);
    high_.prepare(sampleRate_, preset, 0.52f, 0x2003u);

    low_.setFilterCutoff(180.0f);
    low_.setFilterResonance(0.7f);
    low_.setAmpEnvelope({0.001f, 0.05f, 0.0f, 0.08f});
    low_.setGain(0.55f);

    mid_.setFilterCutoff(2100.0f);
    mid_.setFilterResonance(0.38f);
    mid_.setAmpEnvelope({0.001f, 0.045f, 0.0f, 0.055f});
    mid_.setGain(0.36f);

    high_.setFilterCutoff(7600.0f);
    high_.setFilterResonance(0.08f);
    high_.setAmpEnvelope({0.001f, 0.012f, 0.0f, 0.018f});
    high_.setGain(0.22f);

    stepSamples_ = static_cast<size_t>(sampleRate_ * 0.125f);
    reset();
  }

  void reset() {
    sampleCursor_ = 0;
    lowGate_ = 0;
    midGate_ = 0;
    highGate_ = 0;
    low_.reset();
    mid_.reset();
    high_.reset();
  }

  void process(rpdsp::DefaultAudioBlock& output) {
    output.clear();
    for (size_t i = 0; i < output.frames(); ++i) {
      sequence();
      updateGates();
      const auto low = low_.process();
      const auto mid = mid_.process();
      const auto high = high_.process();
      output.left()[i] = rpdsp::softClip(low.left + mid.left + high.left);
      output.right()[i] = rpdsp::softClip(low.right + mid.right + high.right);
      ++sampleCursor_;
    }
  }

 private:
  void trigger(PannedVoice<1>& voice, float hz, float velocity, size_t& gate, float gateSeconds) {
    voice.noteOnHz(hz, velocity, 9);
    // A sample counter schedules noteOff without timers or allocation.
    gate = static_cast<size_t>(gateSeconds * sampleRate_);
  }

  void sequence() {
    if (sampleCursor_ % stepSamples_ != 0) {
      return;
    }

    const size_t step = (sampleCursor_ / stepSamples_) % 16;
    if (step % 4 == 0 || step == 10) {
      trigger(low_, 62.0f, step == 0 ? 1.0f : 0.75f, lowGate_, 0.045f);
    }
    if (step == 4 || step == 12 || step == 14) {
      trigger(mid_, 220.0f, step == 12 ? 0.95f : 0.72f, midGate_, 0.038f);
    }
    if (step % 2 == 0 || step == 7 || step == 15) {
      trigger(high_, 1200.0f, (step % 4 == 0) ? 0.55f : 0.38f, highGate_, 0.015f);
    }
  }

  void updateGate(PannedVoice<1>& voice, size_t& gate) {
    if (gate == 0) {
      return;
    }
    --gate;
    if (gate == 0) {
      voice.noteOff();
    }
  }

  void updateGates() {
    updateGate(low_, lowGate_);
    updateGate(mid_, midGate_);
    updateGate(high_, highGate_);
  }

  float sampleRate_ = rpdsp::kDefaultSampleRate;
  size_t sampleCursor_ = 0;
  size_t stepSamples_ = 6000;
  size_t lowGate_ = 0;
  size_t midGate_ = 0;
  size_t highGate_ = 0;
  PannedVoice<1> low_;
  PannedVoice<1> mid_;
  PannedVoice<1> high_;
};

class MidiStyleChordDemo {
 public:
  void prepare(float sampleRate, size_t maxBlockFrames) {
    (void)maxBlockFrames;
    sampleRate_ = sampleRate;
    auto preset = rpdsp::classicThreeSawSubtractivePreset();
    preset.ampEnvelope = {0.012f, 0.22f, 0.5f, 0.16f};
    preset.filter = {1450.0f, 0.22f, 0.42f};
    preset.gain = 0.18f;
    voices_[0].prepare(sampleRate_, preset, -0.70f, 0x3001u);
    voices_[1].prepare(sampleRate_, preset, -0.30f, 0x3002u);
    voices_[2].prepare(sampleRate_, preset, 0.18f, 0x3003u);
    voices_[3].prepare(sampleRate_, preset, 0.62f, 0x3004u);
    chordSamples_ = static_cast<size_t>(sampleRate_ * 0.72f);
    gateSamples_ = static_cast<size_t>(sampleRate_ * 0.58f);
    reset();
  }

  void reset() {
    sampleCursor_ = 0;
    for (auto& voice : voices_) {
      voice.reset();
    }
  }

  void process(rpdsp::DefaultAudioBlock& output) {
    output.clear();
    for (size_t i = 0; i < output.frames(); ++i) {
      sequence();
      rpdsp::StereoSample mix;
      for (auto& voice : voices_) {
        const auto out = voice.process();
        mix.left += out.left;
        mix.right += out.right;
      }
      output.left()[i] = rpdsp::softClip(mix.left);
      output.right()[i] = rpdsp::softClip(mix.right);
      ++sampleCursor_;
    }
  }

 private:
  void sequence() {
    if (sampleCursor_ % chordSamples_ == 0) {
      const size_t chord = (sampleCursor_ / chordSamples_) % kRoots.size();
      const int root = kRoots[chord];
      for (size_t i = 0; i < voices_.size(); ++i) {
        const float velocity = 0.58f + (0.12f * static_cast<float>(i));
        voices_[i].setFilterCutoff(1100.0f + 260.0f * static_cast<float>(i + chord));
        voices_[i].noteOn(root + kIntervals[chord][i], velocity, 1);
        heldNotes_[i] = root + kIntervals[chord][i];
      }
    }
    if (sampleCursor_ % chordSamples_ == gateSamples_) {
      for (size_t i = 0; i < voices_.size(); ++i) {
        // Velocity-zero noteOn mirrors MIDI-style note release for the held notes.
        voices_[i].noteOn(heldNotes_[i], 0.0f, 1);
      }
    }
  }

  static constexpr std::array<int, 4> kRoots{48, 43, 45, 40};
  static constexpr std::array<std::array<int, 4>, 4> kIntervals{{
      {0, 7, 12, 16},
      {0, 7, 10, 15},
      {0, 5, 12, 15},
      {0, 7, 10, 14},
  }};

  float sampleRate_ = rpdsp::kDefaultSampleRate;
  size_t sampleCursor_ = 0;
  size_t chordSamples_ = 24000;
  size_t gateSamples_ = 18000;
  std::array<int, 4> heldNotes_{};
  std::array<PannedVoice<3>, 4> voices_;
};

}  // namespace

int main() {
  ClassicThreeSawDemo classicDemo;
  classicDemo.prepare(example::kSampleRate, example::kBlockSize);
  const bool classicOk =
      example::renderToWav(classicDemo, "voice_demo_classic_three_saw.wav", 8.0f);

  NoisePercussionDemo noiseDemo;
  noiseDemo.prepare(example::kSampleRate, example::kBlockSize);
  const bool noiseOk =
      example::renderToWav(noiseDemo, "voice_demo_noise_percussion.wav", 8.0f);

  MidiStyleChordDemo midiDemo;
  midiDemo.prepare(example::kSampleRate, example::kBlockSize);
  const bool midiOk =
      example::renderToWav(midiDemo, "voice_demo_midi_trigger_chords.wav", 8.0f);

  return classicOk && noiseOk && midiOk ? 0 : 1;
}
