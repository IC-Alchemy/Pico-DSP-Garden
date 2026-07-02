#pragma once

#include <HarmonyEngine/CounterpointGenerator.h>
#include <HarmonyEngine/MusicTheory.h>
#include <HarmonyEngine/ReharmonizationEngine.h>
#include <HarmonyEngine/TensionAnalysisEngine.h>
#include <HarmonyEngine/VoiceLeadingEngine.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace harmony_examples {

static const std::size_t kMaxPlanVoices = 4;
static const std::size_t kMaxPlanEvents = 64;
static const std::size_t kMaxTensionPoints = 64;

struct NoteEvent {
  int midi = 60;
  float velocity = 0.8f;
  std::uint32_t startTick = 0;
  std::uint32_t lenTicks = 1;
  float tension = 0.0f;
  bool accent = false;
};

struct PlanVoice {
  NoteEvent events[kMaxPlanEvents]{};
  std::size_t eventCount = 0;

  bool push(const NoteEvent& event) {
    if (eventCount >= kMaxPlanEvents) {
      return false;
    }
    events[eventCount++] = event;
    return true;
  }
};

struct PlanBuffer {
  PlanVoice voices[kMaxPlanVoices]{};
  float tension[kMaxTensionPoints]{};
  bool tensionPeak[kMaxTensionPoints]{};
  std::size_t voiceCount = 0;
  std::size_t tensionCount = 0;
  std::uint32_t totalTicks = 0;
  bool valid = false;

  void clear() {
    for (std::size_t i = 0; i < kMaxPlanVoices; ++i) {
      voices[i].eventCount = 0;
    }
    for (std::size_t i = 0; i < kMaxTensionPoints; ++i) {
      tension[i] = 0.0f;
      tensionPeak[i] = false;
    }
    voiceCount = 0;
    tensionCount = 0;
    totalTicks = 0;
    valid = false;
  }
};

struct TensionSculptorSettings {
  float tensionTarget = 0.45f;
  float complexity = 0.5f;
  float harmonicRhythm = 0.5f;
  float registerSpread = 0.5f;
  int presetIndex = 0;
};

struct TensionSculptorReport {
  HarmonyEngine::ReharmonizationResult reharmonization;
  HarmonyEngine::TensionCurve curve;
  HarmonyEngine::VoicingResult voicing;
};

struct CounterpointSettings {
  int speciesZone = 0;
  bool strict = true;
  HarmonyEngine::Key key = HarmonyEngine::Key(0, HarmonyEngine::ScaleType::Major);
  std::uint32_t seed = 1;
};

struct CounterpointReport {
  HarmonyEngine::Voice cantus;
  std::vector<HarmonyEngine::Voice> voices;
  HarmonyEngine::CounterpointAnalysis analysis;
};

inline int positiveMod(int value, int modulus) {
  const int result = value % modulus;
  return result < 0 ? result + modulus : result;
}

inline int chordIntervals(HarmonyEngine::ChordType type, int out[5]) {
  switch (type) {
    case HarmonyEngine::ChordType::Major: out[0] = 0; out[1] = 4; out[2] = 7; return 3;
    case HarmonyEngine::ChordType::Minor: out[0] = 0; out[1] = 3; out[2] = 7; return 3;
    case HarmonyEngine::ChordType::Diminished: out[0] = 0; out[1] = 3; out[2] = 6; return 3;
    case HarmonyEngine::ChordType::Augmented: out[0] = 0; out[1] = 4; out[2] = 8; return 3;
    case HarmonyEngine::ChordType::Sus2: out[0] = 0; out[1] = 2; out[2] = 7; return 3;
    case HarmonyEngine::ChordType::Sus4: out[0] = 0; out[1] = 5; out[2] = 7; return 3;
    case HarmonyEngine::ChordType::Major7: out[0] = 0; out[1] = 4; out[2] = 7; out[3] = 11; return 4;
    case HarmonyEngine::ChordType::Minor7: out[0] = 0; out[1] = 3; out[2] = 7; out[3] = 10; return 4;
    case HarmonyEngine::ChordType::Dominant7: out[0] = 0; out[1] = 4; out[2] = 7; out[3] = 10; return 4;
    case HarmonyEngine::ChordType::Diminished7: out[0] = 0; out[1] = 3; out[2] = 6; out[3] = 9; return 4;
    case HarmonyEngine::ChordType::HalfDiminished7: out[0] = 0; out[1] = 3; out[2] = 6; out[3] = 10; return 4;
    case HarmonyEngine::ChordType::MinorMajor7: out[0] = 0; out[1] = 3; out[2] = 7; out[3] = 11; return 4;
    case HarmonyEngine::ChordType::Augmented7: out[0] = 0; out[1] = 4; out[2] = 8; out[3] = 10; return 4;
    case HarmonyEngine::ChordType::Add9: out[0] = 0; out[1] = 4; out[2] = 7; out[3] = 14; return 4;
    case HarmonyEngine::ChordType::Major9: out[0] = 0; out[1] = 4; out[2] = 7; out[3] = 11; out[4] = 14; return 5;
    case HarmonyEngine::ChordType::Minor9: out[0] = 0; out[1] = 3; out[2] = 7; out[3] = 10; out[4] = 14; return 5;
    case HarmonyEngine::ChordType::Dominant9: out[0] = 0; out[1] = 4; out[2] = 7; out[3] = 10; out[4] = 14; return 5;
    default: out[0] = 0; out[1] = 4; out[2] = 7; return 3;
  }
}

inline HarmonyEngine::Chord materializeChord(HarmonyEngine::Chord chord, int baseMidi = 48) {
  const int rootPc = positiveMod(chord.rootNote, 12);
  int intervals[5] = {};
  const int count = chordIntervals(chord.type, intervals);

  chord.rootNote = rootPc;
  chord.notes.clear();
  for (int i = 0; i < count; ++i) {
    int midi = baseMidi + rootPc + intervals[i];
    while (midi < 36) {
      midi += 12;
    }
    while (midi > 96) {
      midi -= 12;
    }
    chord.notes.push_back(HarmonyEngine::Note(midi, 1.0f, 0.8f, 0.0f));
  }
  return chord;
}

inline std::vector<HarmonyEngine::Chord> seedProgressionC() {
  using namespace HarmonyEngine;
  std::vector<Chord> progression;
  progression.push_back(Chord(ChordType::Major7, 0));
  progression.back().function = ChordFunction::Tonic;
  progression.push_back(Chord(ChordType::Minor7, 9));
  progression.back().function = ChordFunction::Submediant;
  progression.push_back(Chord(ChordType::Minor7, 2));
  progression.back().function = ChordFunction::Predominant;
  progression.push_back(Chord(ChordType::Dominant7, 7));
  progression.back().function = ChordFunction::Dominant;
  progression.push_back(Chord(ChordType::Major7, 0));
  progression.back().function = ChordFunction::Tonic;
  progression.push_back(Chord(ChordType::Major7, 5));
  progression.back().function = ChordFunction::Subdominant;
  progression.push_back(Chord(ChordType::Minor7, 2));
  progression.back().function = ChordFunction::Predominant;
  progression.push_back(Chord(ChordType::Dominant7, 7));
  progression.back().function = ChordFunction::Dominant;
  return progression;
}

inline void copyTensionCurve(const HarmonyEngine::TensionCurve& curve, PlanBuffer& plan) {
  plan.tensionCount = std::min(curve.tensionValues.size(), kMaxTensionPoints);
  for (std::size_t i = 0; i < plan.tensionCount; ++i) {
    plan.tension[i] = std::max(0.0f, std::min(1.0f, curve.tensionValues[i]));
  }
  for (const auto& peak : curve.peaks) {
    const int index = static_cast<int>(peak.timePosition / curve.timeResolution);
    if (index >= 0 && static_cast<std::size_t>(index) < plan.tensionCount) {
      plan.tensionPeak[index] = true;
    }
  }
}

inline bool buildTensionSculptorPlan(const TensionSculptorSettings& settings,
                                     PlanBuffer& plan,
                                     TensionSculptorReport* report = nullptr) {
  using namespace HarmonyEngine;
  plan.clear();

  std::vector<Chord> seed = seedProgressionC();
  for (auto& chord : seed) {
    chord = materializeChord(chord);
  }

  BasicReharmonizationEngine reharmonizer;
  std::vector<ReharmonizationPreset> presets = reharmonizer.getAvailablePresets();
  ReharmonizationPreset preset = presets.empty()
      ? ReharmonizationPreset()
      : presets[static_cast<std::size_t>(std::max(0, settings.presetIndex)) % presets.size()];
  preset.complexityLevel = std::max(0.0f, std::min(1.0f, settings.complexity));
  preset.substitutionProbability = 0.25f + (settings.complexity * 0.65f);
  preset.maxSimultaneousSubstitutions = 1 + static_cast<int>(settings.complexity * 4.0f);

  const Key key(0, ScaleType::Major);
  ReharmonizationResult reharm = reharmonizer.applyPreset(seed, preset, key);
  std::vector<Chord> progression = reharm.isValid() ? reharm.reharmonizedProgression : seed;
  for (auto& chord : progression) {
    chord = materializeChord(chord);
  }

  BasicTensionAnalysisEngine tensionEngine;
  TensionCurve curve = tensionEngine.calculateBeatTension(progression, 4.0f);

  VoicingConstraints constraints;
  constraints.strictParallelRules = false;
  constraints.maxVoiceLeap = 7.0f + (settings.registerSpread * 10.0f);
  constraints.prioritizeCommonTones = true;
  BasicVoiceLeadingEngine voiceLeading;
  VoicingResult voicing = voiceLeading.generateVoicing(progression, constraints);

  const std::uint32_t ticksPerChord = settings.harmonicRhythm > 0.7f ? 2u : (settings.harmonicRhythm < 0.3f ? 8u : 4u);
  plan.voiceCount = std::min<std::size_t>(voicing.voices.size(), kMaxPlanVoices);
  if (plan.voiceCount == 0) {
    plan.voiceCount = 4;
  }
  plan.totalTicks = static_cast<std::uint32_t>(progression.size()) * ticksPerChord;
  copyTensionCurve(curve, plan);

  if (!voicing.voices.empty()) {
    for (std::size_t v = 0; v < plan.voiceCount; ++v) {
      const Voice& voice = voicing.voices[v];
      const std::size_t count = std::min(voice.notes.size(), kMaxPlanEvents);
      for (std::size_t i = 0; i < count; ++i) {
        const float t = curve.getTensionAtTime(static_cast<float>(i));
        NoteEvent event;
        event.midi = voice.notes[i].midiNumber;
        event.velocity = std::max(0.25f, std::min(1.0f, voice.notes[i].velocity + (t * 0.15f)));
        event.startTick = static_cast<std::uint32_t>(i) * ticksPerChord;
        event.lenTicks = ticksPerChord;
        event.tension = t;
        event.accent = t >= std::max(0.55f, settings.tensionTarget);
        plan.voices[v].push(event);
      }
    }
  } else {
    for (std::size_t i = 0; i < progression.size() && i < kMaxPlanEvents; ++i) {
      int intervals[5] = {};
      const int count = chordIntervals(progression[i].type, intervals);
      for (std::size_t v = 0; v < plan.voiceCount; ++v) {
        NoteEvent event;
        event.midi = 48 + positiveMod(progression[i].rootNote, 12) + intervals[v % static_cast<std::size_t>(count)];
        event.startTick = static_cast<std::uint32_t>(i) * ticksPerChord;
        event.lenTicks = ticksPerChord;
        event.tension = curve.getTensionAtTime(static_cast<float>(i));
        plan.voices[v].push(event);
      }
    }
  }

  plan.valid = plan.voiceCount > 0 && plan.totalTicks > 0 && plan.voices[0].eventCount > 0;
  if (report) {
    report->reharmonization = reharm;
    report->curve = curve;
    report->voicing = voicing;
  }
  return plan.valid;
}

inline std::uint32_t nextRand(std::uint32_t& state) {
  state = (state * 1664525u) + 1013904223u;
  return state;
}

inline HarmonyEngine::Voice generateCantusFirmus(const HarmonyEngine::Key& key,
                                                 std::uint32_t seed,
                                                 int length = 8) {
  HarmonyEngine::Voice cantus(HarmonyEngine::VoiceType::Tenor);
  const int clampedLength = std::max(8, std::min(12, length));
  const int tonic = key.tonicNote + 60;
  const std::vector<int> degrees = key.getScaleDegrees();
  const int climaxIndex = 2 + static_cast<int>(seed % static_cast<std::uint32_t>(clampedLength - 4));

  int previous = tonic;
  for (int i = 0; i < clampedLength; ++i) {
    int midi = tonic;
    if (i == 0 || i == clampedLength - 1) {
      midi = tonic;
    } else if (i == climaxIndex) {
      midi = tonic + 12;
    } else {
      const int contour = i < climaxIndex ? 1 : -1;
      const int degreeIndex = static_cast<int>(nextRand(seed) % degrees.size());
      int pc = degrees[static_cast<std::size_t>(degreeIndex)];
      midi = 48 + pc + (contour > 0 ? 12 : 0);
      while (midi <= previous - 5) {
        midi += 12;
      }
      while (midi >= previous + 8) {
        midi -= 12;
      }
      if (i < climaxIndex && midi < previous) {
        midi += 2;
      }
      if (i > climaxIndex && midi > previous) {
        midi -= 2;
      }
      if (midi >= tonic + 12) {
        midi = tonic + 11;
      }
    }
    midi = std::max(cantus.range.lowestNote, std::min(cantus.range.highestNote, midi));
    cantus.addNote(HarmonyEngine::Note(midi, 4.0f, 0.78f, static_cast<float>(i) * 4.0f));
    previous = midi;
  }
  return cantus;
}

inline bool hasSingleClimax(const HarmonyEngine::Voice& voice) {
  if (voice.notes.empty()) {
    return false;
  }
  int highest = voice.notes.front().midiNumber;
  for (const auto& note : voice.notes) {
    highest = std::max(highest, note.midiNumber);
  }
  int count = 0;
  for (const auto& note : voice.notes) {
    if (note.midiNumber == highest) {
      ++count;
    }
  }
  return count == 1;
}

inline bool buildCounterpointPlan(const CounterpointSettings& settings,
                                  PlanBuffer& plan,
                                  CounterpointReport* report = nullptr) {
  using namespace HarmonyEngine;
  plan.clear();

  Voice cantus = generateCantusFirmus(settings.key, settings.seed);
  CounterpointRules rules(settings.strict ? CounterpointStyle::Strict : CounterpointStyle::Renaissance);
  if (!settings.strict) {
    rules.allowDissonantPassing = true;
    rules.maxMelodicLeap = 10.0f;
  }

  BasicCounterpointGenerator generator;
  std::vector<Voice> voices;
  voices.push_back(cantus);
  if (settings.speciesZone >= 3) {
    std::vector<Voice> generated = generator.generateCounterpoint(cantus, 3, rules);
    if (generated.size() >= 2) {
      voices = generated;
    }
  } else if (settings.speciesZone == 2) {
    voices.push_back(generator.generateThirdSpecies(cantus, VoiceType::Alto, rules));
  } else if (settings.speciesZone == 1) {
    voices.push_back(generator.generateSecondSpecies(cantus, VoiceType::Alto, rules));
  } else {
    voices.push_back(generator.generateFirstSpecies(cantus, VoiceType::Alto, rules));
  }

  CounterpointAnalysis analysis = generator.analyzeCounterpoint(voices, rules);
  plan.voiceCount = std::min(voices.size(), kMaxPlanVoices);
  for (std::size_t v = 0; v < plan.voiceCount; ++v) {
    for (const auto& note : voices[v].notes) {
      NoteEvent event;
      event.midi = note.midiNumber;
      event.velocity = note.velocity;
      event.startTick = static_cast<std::uint32_t>(note.startTime);
      event.lenTicks = static_cast<std::uint32_t>(std::max(1.0f, note.duration));
      event.accent = (static_cast<int>(note.startTime) % 4) == 0;
      plan.voices[v].push(event);
      plan.totalTicks = std::max(plan.totalTicks, event.startTick + event.lenTicks);
    }
  }

  plan.valid = plan.voiceCount >= 2 && plan.totalTicks > 0 && plan.voices[0].eventCount >= 8;
  if (report) {
    report->cantus = cantus;
    report->voices = voices;
    report->analysis = analysis;
  }
  return plan.valid;
}

}  // namespace harmony_examples
