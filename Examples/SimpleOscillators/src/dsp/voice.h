#pragma once

#include "rp2350_audio_dsp/dsp/algorithm.h"
#include "rp2350_audio_dsp/dsp/envelope.h"
#include "rp2350_audio_dsp/dsp/filter.h"
#include "rp2350_audio_dsp/dsp/oscillator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace rpdsp {

struct VoiceTrigger {
  int note = 69;
  float velocity = 1.0f;
  int channel = 0;
};

struct SawVoiceOscillator {
  float level = 1.0f;
  float semitoneOffset = 0.0f;
  float centOffset = 0.0f;
  float phase = 0.0f;
};

struct VoiceEnvelopeSettings {
  float attackSeconds = 0.005f;
  float decaySeconds = 0.08f;
  float sustain = 0.5f;
  float releaseSeconds = 0.08f;
};

struct VoiceFilterSettings {
  float cutoffHz = 1800.0f;
  float resonance = 0.2f;
  float velocityToCutoff = 0.35f;
};

template <size_t MaxOscillators>
struct TriggeredSynthVoicePreset {
  // Fixed-capacity presets avoid allocation while still allowing compact synth recipes.
  std::array<SawVoiceOscillator, MaxOscillators> oscillators{};
  size_t oscillatorCount = 0;
  float noiseLevel = 0.0f;
  VoiceFilterSettings filter{};
  VoiceEnvelopeSettings ampEnvelope{};
  float gain = 0.25f;
  bool resetPhaseOnTrigger = true;
};

template <size_t MaxOscillators>
class TriggeredSynthVoice {
 public:
  using Preset = TriggeredSynthVoicePreset<MaxOscillators>;

  void prepare(float sampleRate) {
    sampleRate_ = safeSampleRate(sampleRate);
    for (auto& oscillator : oscillators_) {
      oscillator.prepare(sampleRate_);
    }
    filter_.prepare(sampleRate_);
    ampEnvelope_.prepare(sampleRate_);
    applyPreset(preset_);
  }

  void reset() {
    for (size_t i = 0; i < MaxOscillators; ++i) {
      oscillators_[i].reset(preset_.oscillators[i].phase);
    }
    noise_.reseed(noiseSeed_);
    filter_.reset();
    ampEnvelope_.reset();
    currentTrigger_ = {};
    active_ = false;
  }

  void applyPreset(const Preset& preset) {
    preset_ = preset;
    // Clamp preset data at the boundary so process() can stay branch-light and realtime-safe.
    preset_.oscillatorCount = std::min(preset_.oscillatorCount, MaxOscillators);
    preset_.noiseLevel = clamp(preset_.noiseLevel, 0.0f, 4.0f);
    preset_.gain = clamp(preset_.gain, 0.0f, 4.0f);
    preset_.filter.cutoffHz = clampCutoff(preset_.filter.cutoffHz, sampleRate_);
    preset_.filter.resonance = clamp(preset_.filter.resonance, 0.0f, 0.98f);
    preset_.filter.velocityToCutoff = clamp(preset_.filter.velocityToCutoff, 0.0f, 4.0f);
    preset_.ampEnvelope.sustain = clamp01(preset_.ampEnvelope.sustain);
    ampEnvelope_.set(preset_.ampEnvelope.attackSeconds, preset_.ampEnvelope.decaySeconds,
                     preset_.ampEnvelope.sustain, preset_.ampEnvelope.releaseSeconds);
    filter_.setCutoffResonance(preset_.filter.cutoffHz, preset_.filter.resonance);
    updateOscillatorFrequencies();
  }

  void setNoiseSeed(std::uint32_t seed) {
    noiseSeed_ = seed == 0 ? 1u : seed;
    noise_.reseed(noiseSeed_);
  }

  void setFilterCutoff(float cutoffHz) {
    preset_.filter.cutoffHz = clampCutoff(cutoffHz, sampleRate_);
    filter_.setCutoffResonance(preset_.filter.cutoffHz, preset_.filter.resonance);
  }

  void setFilterResonance(float resonance) {
    preset_.filter.resonance = clamp(resonance, 0.0f, 0.98f);
    filter_.setCutoffResonance(preset_.filter.cutoffHz, preset_.filter.resonance);
  }

  void setNoiseLevel(float level) { preset_.noiseLevel = clamp(level, 0.0f, 4.0f); }
  void setGain(float gain) { preset_.gain = clamp(gain, 0.0f, 4.0f); }

  void setAmpEnvelope(const VoiceEnvelopeSettings& settings) {
    preset_.ampEnvelope = settings;
    preset_.ampEnvelope.sustain = clamp01(preset_.ampEnvelope.sustain);
    ampEnvelope_.set(preset_.ampEnvelope.attackSeconds, preset_.ampEnvelope.decaySeconds,
                     preset_.ampEnvelope.sustain, preset_.ampEnvelope.releaseSeconds);
  }

  void noteOn(int midiNote, float velocity = 1.0f, int channel = 0) {
    noteOn({midiNote, velocity, channel});
  }

  void noteOn(const VoiceTrigger& trigger) {
    if (trigger.velocity <= 0.0f) {
      // MIDI treats note-on velocity zero as note-off; mirror that at the voice API.
      noteOff(trigger.note, trigger.channel);
      return;
    }
    currentTrigger_.note = trigger.note;
    currentTrigger_.velocity = clamp01(trigger.velocity);
    currentTrigger_.channel = trigger.channel;
    baseFrequencyHz_ = midiNoteToHz(static_cast<float>(currentTrigger_.note));
    updateOscillatorFrequencies();
    if (preset_.resetPhaseOnTrigger) {
      // Phase reset gives repeatable attacks for bass/pluck sounds; presets may opt out.
      for (size_t i = 0; i < preset_.oscillatorCount; ++i) {
        oscillators_[i].reset(preset_.oscillators[i].phase);
      }
      filter_.reset();
    }
    ampEnvelope_.noteOn();
    active_ = true;
  }

  void noteOnHz(float frequencyHz, float velocity = 1.0f, int channel = 0) {
    if (velocity <= 0.0f) {
      noteOff(-1, channel);
      return;
    }
    baseFrequencyHz_ = clamp(frequencyHz, 1.0f, sampleRate_ * 0.45f);
    currentTrigger_.note = -1;
    currentTrigger_.velocity = clamp01(velocity);
    currentTrigger_.channel = channel;
    updateOscillatorFrequencies();
    if (preset_.resetPhaseOnTrigger) {
      for (size_t i = 0; i < preset_.oscillatorCount; ++i) {
        oscillators_[i].reset(preset_.oscillators[i].phase);
      }
      filter_.reset();
    }
    ampEnvelope_.noteOn();
    active_ = true;
  }

  void noteOff(int midiNote = -1, int channel = -1) {
    // Negative note/channel act as wildcards for monophonic or externally allocated voices.
    const bool noteMatches = midiNote < 0 || currentTrigger_.note < 0 || midiNote == currentTrigger_.note;
    const bool channelMatches = channel < 0 || channel == currentTrigger_.channel;
    if (noteMatches && channelMatches) {
      ampEnvelope_.noteOff();
    }
  }

  float process() {
    if (!ampEnvelope_.isActive() && !active_) {
      return 0.0f;
    }

    // Velocity opens the filter as well as scaling loudness, matching common subtractive synth behavior.
    const float velocity = currentTrigger_.velocity;
    const float cutoffScale = 1.0f + (preset_.filter.velocityToCutoff * velocity);
    filter_.setCutoffResonance(preset_.filter.cutoffHz * cutoffScale, preset_.filter.resonance);

    float source = 0.0f;
    for (size_t i = 0; i < preset_.oscillatorCount; ++i) {
      source += oscillators_[i].process() * preset_.oscillators[i].level;
    }
    source += noise_.process() * preset_.noiseLevel;

    const float envelope = ampEnvelope_.process();
    if (!ampEnvelope_.isActive()) {
      active_ = false;
    }

    return filter_.process(source).lowpass * envelope * velocity * preset_.gain;
  }

  [[nodiscard]] bool isActive() const { return active_ || ampEnvelope_.isActive(); }
  [[nodiscard]] int currentNote() const { return currentTrigger_.note; }
  [[nodiscard]] int currentChannel() const { return currentTrigger_.channel; }
  [[nodiscard]] float currentVelocity() const { return currentTrigger_.velocity; }
  [[nodiscard]] float baseFrequencyHz() const { return baseFrequencyHz_; }

 private:
  void updateOscillatorFrequencies() {
    for (size_t i = 0; i < preset_.oscillatorCount; ++i) {
      const auto& settings = preset_.oscillators[i];
      // Cents are hundredths of a semitone; both offsets collapse to one pitch ratio.
      const float semitones = settings.semitoneOffset + (settings.centOffset * 0.01f);
      const float ratio = std::pow(2.0f, semitones / 12.0f);
      oscillators_[i].setFreq(clamp(baseFrequencyHz_ * ratio, 1.0f, sampleRate_ * 0.45f));
    }
  }

  float sampleRate_ = kDefaultSampleRate;
  float baseFrequencyHz_ = 440.0f;
  std::uint32_t noiseSeed_ = 0x12345678u;
  Preset preset_{};
  VoiceTrigger currentTrigger_{};
  bool active_ = false;
  std::array<SecondOrderBSplineSawOscillator, MaxOscillators> oscillators_{};
  NoiseOscillator noise_;
  StateVariableFilter filter_;
  ADSR ampEnvelope_;
};

inline TriggeredSynthVoicePreset<3> classicThreeSawSubtractivePreset() {
  // Slight detune and phase offsets keep repeated notes full without requiring chorus.
  TriggeredSynthVoicePreset<3> preset{};
  preset.oscillatorCount = 3;
  preset.oscillators[0] = {0.58f, 0.0f, -4.0f, 0.00f};
  preset.oscillators[1] = {0.46f, 0.0f, 5.0f, 0.29f};
  preset.oscillators[2] = {0.34f, -12.0f, 2.0f, 0.61f};
  preset.noiseLevel = 0.05f;
  preset.filter = {1800.0f, 0.34f, 0.55f};
  preset.ampEnvelope = {0.003f, 0.075f, 0.5f, 0.075f};
  preset.gain = 0.24f;
  preset.resetPhaseOnTrigger = true;
  return preset;
}

inline TriggeredSynthVoicePreset<1> noisePluckPreset() {
  // Noise-only excitation into a resonant lowpass gives a cheap percussive voice.
  TriggeredSynthVoicePreset<1> preset{};
  preset.oscillatorCount = 0;
  preset.noiseLevel = 1.0f;
  preset.filter = {1200.0f, 0.55f, 1.25f};
  preset.ampEnvelope = {0.001f, 0.035f, 0.0f, 0.045f};
  preset.gain = 0.32f;
  preset.resetPhaseOnTrigger = true;
  return preset;
}

}  // namespace rpdsp
