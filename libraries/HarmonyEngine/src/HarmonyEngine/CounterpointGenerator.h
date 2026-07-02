/*
  ==============================================================================

    CounterpointGenerator.h
    Created: Counterpoint generation engine interface and implementation
    
    This file contains the CounterpointGenerator interface and BasicCounterpointGenerator
    implementation that provides species counterpoint generation with proper voice
    independence and contrapuntal rules.

  ==============================================================================
*/

#pragma once

// Implementation includes (header-only)
#include <algorithm>
#include <random>
#include <cmath>

#include "MusicTheory.h"
#include "VoiceLeadingEngine.h"
#include <vector>
#include <memory>

namespace HarmonyEngine
{
    //==============================================================================
    // Counterpoint Configuration and Rules
    
    enum class CounterpointSpecies
    {
        First,      // Note against note
        Second,     // Two notes against one
        Third,      // Four notes against one
        Fourth,     // Syncopation
        Fifth       // Florid counterpoint (combination of all species)
    };
    
    enum class CounterpointStyle
    {
        Strict,         // Strict Fuxian rules
        Renaissance,    // Renaissance polyphony style
        Baroque,        // Baroque counterpoint style
        Contemporary    // Modern counterpoint with relaxed rules
    };
    
    struct CounterpointRules
    {
        bool allowParallelFifths;
        bool allowParallelOctaves;
        bool allowParallelUnisons;
        bool requireConsonantDownbeats;
        bool allowDissonantPassing;
        bool enforceSpeciesRhythm;
        float maxMelodicLeap; // In semitones
        bool requireClimax; // Melodic high point
        bool allowCrossing;
        
        CounterpointRules(CounterpointStyle style = CounterpointStyle::Strict)
        {
            switch (style)
            {
                case CounterpointStyle::Strict:
                    allowParallelFifths = false;
                    allowParallelOctaves = false;
                    allowParallelUnisons = false;
                    requireConsonantDownbeats = true;
                    allowDissonantPassing = false;
                    enforceSpeciesRhythm = true;
                    maxMelodicLeap = 8.0f; // Minor sixth
                    requireClimax = true;
                    allowCrossing = false;
                    break;
                    
                case CounterpointStyle::Renaissance:
                    allowParallelFifths = false;
                    allowParallelOctaves = false;
                    allowParallelUnisons = false;
                    requireConsonantDownbeats = true;
                    allowDissonantPassing = true;
                    enforceSpeciesRhythm = false;
                    maxMelodicLeap = 9.0f; // Major sixth
                    requireClimax = true;
                    allowCrossing = false;
                    break;
                    
                case CounterpointStyle::Baroque:
                    allowParallelFifths = false;
                    allowParallelOctaves = false;
                    allowParallelUnisons = false;
                    requireConsonantDownbeats = false;
                    allowDissonantPassing = true;
                    enforceSpeciesRhythm = false;
                    maxMelodicLeap = 12.0f; // Octave
                    requireClimax = false;
                    allowCrossing = true;
                    break;
                    
                case CounterpointStyle::Contemporary:
                    allowParallelFifths = true;
                    allowParallelOctaves = false;
                    allowParallelUnisons = false;
                    requireConsonantDownbeats = false;
                    allowDissonantPassing = true;
                    enforceSpeciesRhythm = false;
                    maxMelodicLeap = 15.0f; // Beyond octave
                    requireClimax = false;
                    allowCrossing = true;
                    break;
            }
        }
    };
    
    //==============================================================================
    // Counterpoint Analysis Results
    
    struct CounterpointAnalysis
    {
        bool isValidCounterpoint;
        std::vector<std::string> ruleViolations;
        float consonanceRatio; // Percentage of consonant intervals
        float melodicQuality; // 0.0 to 1.0, based on melodic contour
        float voiceIndependence; // 0.0 to 1.0, rhythmic and melodic independence
        std::vector<int> dissonantIntervals; // Indices of dissonant intervals
        std::vector<int> parallelMotionViolations; // Indices of parallel motion violations
        
        CounterpointAnalysis()
            : isValidCounterpoint(false), consonanceRatio(0.0f),
              melodicQuality(0.0f), voiceIndependence(0.0f) {}
    };
    
    struct VoiceLeadingPath
    {
        std::vector<int> targetNotes; // Sequence of target notes for voice leading
        std::vector<float> rhythmicValues; // Duration values for each note
        Key key; // Harmonic context
        bool allowChromaticPassing; // Allow chromatic passing tones
        float pathStrength; // How strictly to follow the path (0.0 = loose, 1.0 = strict)
        
        VoiceLeadingPath(const Key& k = Key()) : key(k), allowChromaticPassing(false), pathStrength(0.7f) {}
    };
    
    struct MelodicContourProfile
    {
        std::vector<float> directionChanges; // Points where melody changes direction
        std::vector<float> intervalSizes; // Size of melodic intervals
        float overallRange; // Total range of the melody
        float climaxPosition; // Position of melodic climax (0.0 to 1.0)
        bool isAscending; // Overall melodic direction
        
        MelodicContourProfile() : overallRange(0.0f), climaxPosition(0.5f), isAscending(true) {}
    };
    
    struct CounterMelodyConstraints
    {
        Key scale; // Scale to use for counter-melody
        VoiceType voiceType; // Voice range constraints
        bool useComplementaryContour; // Create contour that complements main melody
        bool restrictToScaleTones; // Only use scale tones (no chromatic passing)
        float rhythmicIndependence; // How rhythmically independent (0.0 to 1.0)
        std::vector<int> avoidNotes; // Notes to avoid at specific positions
        
        CounterMelodyConstraints(const Key& k = Key(), VoiceType vt = VoiceType::Alto)
            : scale(k), voiceType(vt), useComplementaryContour(true), 
              restrictToScaleTones(false), rhythmicIndependence(0.6f) {}
    };
    
    //==============================================================================
    // CounterpointGenerator Interface
    
    class CounterpointGenerator
    {
    public:
        virtual ~CounterpointGenerator() = default;
        
        // Core counterpoint generation
        virtual std::vector<Voice> generateCounterpoint(const Voice& cantusFirmus, 
                                                      int numVoices, 
                                                      const CounterpointRules& rules) = 0;
        
        // Species-specific generation
        virtual Voice generateFirstSpecies(const Voice& cantusFirmus, 
                                         VoiceType voiceType,
                                         const CounterpointRules& rules) = 0;
        virtual Voice generateSecondSpecies(const Voice& cantusFirmus, 
                                          VoiceType voiceType,
                                          const CounterpointRules& rules) = 0;
        virtual Voice generateThirdSpecies(const Voice& cantusFirmus, 
                                         VoiceType voiceType,
                                         const CounterpointRules& rules) = 0;
        
        // Counter-melody generation
        virtual Voice generateCounterMelody(const Voice& mainMelody, 
                                          const Key& key,
                                          const VoiceLeadingPath& path) = 0;
        
        // Enhanced counter-melody generation with constraints
        virtual Voice generateCounterMelodyWithConstraints(const Voice& mainMelody,
                                                         const CounterMelodyConstraints& constraints,
                                                         const VoiceLeadingPath& path) = 0;
        
        // Generate multiple counter-melodies that work together
        virtual std::vector<Voice> generateMultipleCounterMelodies(const Voice& mainMelody,
                                                                 const std::vector<CounterMelodyConstraints>& constraints,
                                                                 const VoiceLeadingPath& path) = 0;
        
        // Analyze melodic contour for complementarity
        virtual MelodicContourProfile analyzeMelodicContour(const Voice& melody) = 0;
        
        // Create complementary contour based on main melody
        virtual MelodicContourProfile createComplementaryContour(const MelodicContourProfile& mainContour) = 0;
        
        // Validation and analysis
        virtual bool validateCounterpoint(const std::vector<Voice>& voices, 
                                        const CounterpointRules& rules) = 0;
        virtual CounterpointAnalysis analyzeCounterpoint(const std::vector<Voice>& voices,
                                                       const CounterpointRules& rules) = 0;
        
        // Utility functions
        virtual bool isConsonantInterval(int interval) = 0;
        virtual bool isDissonantInterval(int interval) = 0;
        virtual std::vector<int> getConsonantIntervals() = 0;
        virtual std::vector<int> getDissonantIntervals() = 0;
    };
    
    //==============================================================================
    // Basic Implementation
    
    class BasicCounterpointGenerator : public CounterpointGenerator
    {
    public:
        BasicCounterpointGenerator();
        ~BasicCounterpointGenerator() override = default;
        
        // CounterpointGenerator interface implementation
        std::vector<Voice> generateCounterpoint(const Voice& cantusFirmus, 
                                              int numVoices, 
                                              const CounterpointRules& rules) override;
        
        Voice generateFirstSpecies(const Voice& cantusFirmus, 
                                 VoiceType voiceType,
                                 const CounterpointRules& rules) override;
        Voice generateSecondSpecies(const Voice& cantusFirmus, 
                                  VoiceType voiceType,
                                  const CounterpointRules& rules) override;
        Voice generateThirdSpecies(const Voice& cantusFirmus, 
                                 VoiceType voiceType,
                                 const CounterpointRules& rules) override;
        
        Voice generateCounterMelody(const Voice& mainMelody, 
                                  const Key& key,
                                  const VoiceLeadingPath& path) override;
        
        Voice generateCounterMelodyWithConstraints(const Voice& mainMelody,
                                                 const CounterMelodyConstraints& constraints,
                                                 const VoiceLeadingPath& path) override;
        
        std::vector<Voice> generateMultipleCounterMelodies(const Voice& mainMelody,
                                                         const std::vector<CounterMelodyConstraints>& constraints,
                                                         const VoiceLeadingPath& path) override;
        
        MelodicContourProfile analyzeMelodicContour(const Voice& melody) override;
        
        MelodicContourProfile createComplementaryContour(const MelodicContourProfile& mainContour) override;
        
        bool validateCounterpoint(const std::vector<Voice>& voices, 
                                const CounterpointRules& rules) override;
        CounterpointAnalysis analyzeCounterpoint(const std::vector<Voice>& voices,
                                               const CounterpointRules& rules) override;
        
        bool isConsonantInterval(int interval) override;
        bool isDissonantInterval(int interval) override;
        std::vector<int> getConsonantIntervals() override;
        std::vector<int> getDissonantIntervals() override;
        
    private:
        // Voice leading engine for analysis
        std::unique_ptr<VoiceLeadingEngine> voiceLeadingEngine;
        
        // Counterpoint generation helpers
        Voice generateCounterpointVoice(const Voice& cantusFirmus,
                                      VoiceType voiceType,
                                      CounterpointSpecies species,
                                      const CounterpointRules& rules);
        
        // First species helpers
        std::vector<int> generateFirstSpeciesNotes(const Voice& cantusFirmus,
                                                 VoiceType voiceType,
                                                 const CounterpointRules& rules);
        int selectBestNote(const std::vector<int>& candidates,
                          int previousNote,
                          int cantusFirmusNote,
                          const CounterpointRules& rules);
        
        // Second species helpers
        std::vector<Note> generateSecondSpeciesNotes(const Voice& cantusFirmus,
                                                   VoiceType voiceType,
                                                   const CounterpointRules& rules);
        
        // Third species helpers
        std::vector<Note> generateThirdSpeciesNotes(const Voice& cantusFirmus,
                                                  VoiceType voiceType,
                                                  const CounterpointRules& rules);
        
        // Counter-melody generation helpers
        std::vector<int> generateMelodicLine(const Voice& mainMelody,
                                           const Key& key,
                                           VoiceType voiceType,
                                           const VoiceLeadingPath& path);
        bool followsVoiceLeadingPath(const std::vector<int>& melody,
                                   const VoiceLeadingPath& path);
        
        // Enhanced counter-melody helpers
        std::vector<int> generateConstrainedMelodicLine(const Voice& mainMelody,
                                                      const CounterMelodyConstraints& constraints,
                                                      const VoiceLeadingPath& path);
        std::vector<int> createComplementaryMelody(const Voice& mainMelody,
                                                 const MelodicContourProfile& targetContour,
                                                 const CounterMelodyConstraints& constraints);
        bool validateCounterMelodyHarmony(const std::vector<Voice>& melodies,
                                        const Key& key);
        
        // Melodic contour analysis helpers
        std::vector<float> calculateDirectionChanges(const Voice& melody);
        std::vector<float> calculateIntervalSizes(const Voice& melody);
        float findMelodicClimax(const Voice& melody, bool returnPosition = true);
        bool isOverallAscending(const Voice& melody);
        
        // Scale and harmony utilities
        std::vector<int> getScaleTonesInRange(const Key& scale, int lowestNote, int highestNote);
        std::vector<int> getPassingTonesInRange(const Key& scale, int lowestNote, int highestNote, bool chromatic = false);
        bool isScaleTone(int note, const Key& scale);
        bool isPassingTone(int note, int previousNote, int nextNote, const Key& scale);
        int getClosestScaleTone(int note, const Key& scale, bool preferUp = true);
        
        // Interval analysis
        int calculateInterval(int note1, int note2);
        int normalizeInterval(int interval); // Reduce to within octave
        bool isPerfectConsonance(int interval);
        bool isImperfectConsonance(int interval);
        
        // Melodic validation
        bool validateMelodicLine(const Voice& voice, const CounterpointRules& rules);
        bool hasValidMelodicContour(const Voice& voice);
        bool hasAppropriateClimax(const Voice& voice);
        int findMelodicClimax(const Voice& voice);
        
        // Harmonic validation
        bool validateHarmonicIntervals(const std::vector<Voice>& voices,
                                     const CounterpointRules& rules);
        bool validateParallelMotion(const std::vector<Voice>& voices,
                                  const CounterpointRules& rules);
        
        // Voice independence validation
        float calculateRhythmicIndependence(const std::vector<Voice>& voices);
        float calculateMelodicIndependence(const std::vector<Voice>& voices);
        bool hasProperVoiceIndependence(const std::vector<Voice>& voices,
                                      const CounterpointRules& rules);
        
        // Utility functions
        std::vector<int> getCandidateNotes(int cantusFirmusNote,
                                         VoiceType voiceType,
                                         const CounterpointRules& rules);
        float evaluateNoteChoice(int note,
                                int previousNote,
                                int cantusFirmusNote,
                                const CounterpointRules& rules);
        
        // Scale and key utilities
        std::vector<int> getScaleNotes(const Key& key, int octave);
        bool isInKey(int note, const Key& key);
        int getClosestScaleDegree(int note, const Key& key);
        
        // Constants for interval classification
        static const std::vector<int>& perfectConsonances();
        static const std::vector<int>& imperfectConsonances();
        static const std::vector<int>& dissonances();
    };
    
} // namespace HarmonyEngine

// ============================================================================
// Implementation (merged from .cpp - header-only library)
// ============================================================================

namespace HarmonyEngine
{
    //==============================================================================
    // Static interval constants
    
    inline const std::vector<int>& BasicCounterpointGenerator::perfectConsonances()
    {
        static const std::vector<int> intervals = {0, 7, 12}; // Unison, fifth, octave
        return intervals;
    }

    inline const std::vector<int>& BasicCounterpointGenerator::imperfectConsonances()
    {
        static const std::vector<int> intervals = {3, 4, 8, 9}; // Minor/major third, minor/major sixth
        return intervals;
    }

    inline const std::vector<int>& BasicCounterpointGenerator::dissonances()
    {
        static const std::vector<int> intervals = {1, 2, 5, 6, 10, 11}; // All other intervals
        return intervals;
    }
    
    //==============================================================================
    // BasicCounterpointGenerator Implementation
    
  inline   BasicCounterpointGenerator::BasicCounterpointGenerator()
    {
        voiceLeadingEngine = std::make_unique<BasicVoiceLeadingEngine>();
    }
    
    inline std::vector<Voice> BasicCounterpointGenerator::generateCounterpoint(const Voice& cantusFirmus, 
                                                                      int numVoices, 
                                                                      const CounterpointRules& rules)
    {
        std::vector<Voice> voices;
        voices.push_back(cantusFirmus); // Add cantus firmus as first voice
        
        // Generate additional voices one by one
        for (int i = 1; i < numVoices && i < 4; ++i) // Limit to 4 voices total
        {
            VoiceType voiceType;
            switch (i)
            {
                case 1: voiceType = VoiceType::Alto; break;
                case 2: voiceType = VoiceType::Tenor; break;
                case 3: voiceType = VoiceType::Bass; break;
                default: voiceType = VoiceType::Soprano; break;
            }
            
            // Generate first species counterpoint for each additional voice
            Voice newVoice = generateFirstSpecies(cantusFirmus, voiceType, rules);
            
            // Validate against existing voices
            std::vector<Voice> tempVoices = voices;
            tempVoices.push_back(newVoice);
            
            if (validateCounterpoint(tempVoices, rules))
            {
                voices.push_back(newVoice);
            }
            else
            {
                // If validation fails, try with relaxed rules
                CounterpointRules relaxedRules = rules;
                relaxedRules.allowParallelFifths = true;
                relaxedRules.maxMelodicLeap += 2.0f;
                
                newVoice = generateFirstSpecies(cantusFirmus, voiceType, relaxedRules);
                voices.push_back(newVoice);
            }
        }
        
        return voices;
    }
    
    inline Voice BasicCounterpointGenerator::generateFirstSpecies(const Voice& cantusFirmus, 
                                                         VoiceType voiceType,
                                                         const CounterpointRules& rules)
    {
        return generateCounterpointVoice(cantusFirmus, voiceType, CounterpointSpecies::First, rules);
    }
    
    inline Voice BasicCounterpointGenerator::generateSecondSpecies(const Voice& cantusFirmus, 
                                                          VoiceType voiceType,
                                                          const CounterpointRules& rules)
    {
        return generateCounterpointVoice(cantusFirmus, voiceType, CounterpointSpecies::Second, rules);
    }
    
    inline Voice BasicCounterpointGenerator::generateThirdSpecies(const Voice& cantusFirmus, 
                                                         VoiceType voiceType,
                                                         const CounterpointRules& rules)
    {
        return generateCounterpointVoice(cantusFirmus, voiceType, CounterpointSpecies::Third, rules);
    }
    
    inline Voice BasicCounterpointGenerator::generateCounterMelody(const Voice& mainMelody, 
                                                          const Key& key,
                                                          const VoiceLeadingPath& path)
    {
        Voice counterMelody(VoiceType::Alto); // Default to alto range
        
        if (mainMelody.notes.empty())
            return counterMelody;
        
        // Generate melodic line following the voice leading path
        std::vector<int> melodicLine = generateMelodicLine(mainMelody, key, VoiceType::Alto, path);
        
        // Convert to notes with appropriate timing
        for (size_t i = 0; i < melodicLine.size() && i < mainMelody.notes.size(); ++i)
        {
            float duration = (i < path.rhythmicValues.size()) ? path.rhythmicValues[i] : mainMelody.notes[i].duration;
            float startTime = mainMelody.notes[i].startTime;
            
            Note note(melodicLine[i], duration, 0.7f, startTime);
            counterMelody.addNote(note);
        }
        
        return counterMelody;
    }
    
    inline bool BasicCounterpointGenerator::validateCounterpoint(const std::vector<Voice>& voices, 
                                                        const CounterpointRules& rules)
    {
        if (voices.size() < 2)
            return false;

        // Validate each voice individually
        if (!std::all_of(voices.begin(), voices.end(),
                         [&rules, this](const Voice& v) { return validateMelodicLine(v, rules); }))
            return false;

        // Validate harmonic intervals, parallel motion, and voice independence
        return validateHarmonicIntervals(voices, rules) &&
               validateParallelMotion(voices, rules) &&
               hasProperVoiceIndependence(voices, rules);
    }
    
    inline CounterpointAnalysis BasicCounterpointGenerator::analyzeCounterpoint(const std::vector<Voice>& voices,
                                                                       const CounterpointRules& rules)
    {
        CounterpointAnalysis analysis;
        
        if (voices.size() < 2)
        {
            analysis.ruleViolations.push_back("Insufficient voices for counterpoint analysis");
            return analysis;
        }
        
        // Calculate consonance ratio
        int totalIntervals = 0;
        int consonantIntervals = 0;
        
        for (size_t i = 0; i < voices[0].notes.size(); ++i)
        {
            for (size_t v1 = 0; v1 < voices.size(); ++v1)
            {
                for (size_t v2 = v1 + 1; v2 < voices.size(); ++v2)
                {
                    if (i < voices[v1].notes.size() && i < voices[v2].notes.size())
                    {
                        int interval = calculateInterval(voices[v1].notes[i].midiNumber, 
                                                       voices[v2].notes[i].midiNumber);
                        totalIntervals++;
                        
                        if (isConsonantInterval(interval))
                        {
                            consonantIntervals++;
                        }
                        else
                        {
                            analysis.dissonantIntervals.push_back(static_cast<int>(i));
                        }
                    }
                }
            }
        }
        
        analysis.consonanceRatio = (totalIntervals > 0) ? 
            static_cast<float>(consonantIntervals) / totalIntervals : 0.0f;
        
        // Calculate melodic quality (average of all voices)
        float totalMelodicQuality = 0.0f;
        for (const auto& voice : voices)
        {
            MelodicContour contour = voiceLeadingEngine->analyzeMelodicMotion(voice);
            totalMelodicQuality += contour.stepwiseMotionRatio * 0.6f + 
                                 (1.0f - contour.melodicComplexity) * 0.4f;
        }
        analysis.melodicQuality = totalMelodicQuality / voices.size();
        
        // Calculate voice independence
        analysis.voiceIndependence = calculateRhythmicIndependence(voices) * 0.5f +
                                   calculateMelodicIndependence(voices) * 0.5f;
        
        // Check for rule violations
        if (!validateCounterpoint(voices, rules))
        {
            if (!validateHarmonicIntervals(voices, rules))
                analysis.ruleViolations.push_back("Harmonic interval violations");
            if (!validateParallelMotion(voices, rules))
                analysis.ruleViolations.push_back("Parallel motion violations");
            if (!hasProperVoiceIndependence(voices, rules))
                analysis.ruleViolations.push_back("Insufficient voice independence");
        }
        
        analysis.isValidCounterpoint = analysis.ruleViolations.empty();
        
        return analysis;
    }
    
    inline bool BasicCounterpointGenerator::isConsonantInterval(int interval)
    {
        int normalizedInterval = normalizeInterval(interval);
        
        const auto& perfect = perfectConsonances();
        const auto& imperfect = imperfectConsonances();
        return std::find(perfect.begin(), perfect.end(), normalizedInterval) != perfect.end() ||
               std::find(imperfect.begin(), imperfect.end(), normalizedInterval) != imperfect.end();
    }
    
    inline bool BasicCounterpointGenerator::isDissonantInterval(int interval)
    {
        return !isConsonantInterval(interval);
    }
    
    inline std::vector<int> BasicCounterpointGenerator::getConsonantIntervals()
    {
        const auto& perfect = perfectConsonances();
        const auto& imperfect = imperfectConsonances();
        std::vector<int> consonant = perfect;
        consonant.insert(consonant.end(), imperfect.begin(), imperfect.end());
        return consonant;
    }
    
    inline std::vector<int> BasicCounterpointGenerator::getDissonantIntervals()
    {
        return dissonances();
    }
    
    //==============================================================================
    // Private Implementation Methods
    
    inline Voice BasicCounterpointGenerator::generateCounterpointVoice(const Voice& cantusFirmus,
                                                              VoiceType voiceType,
                                                              CounterpointSpecies species,
                                                              const CounterpointRules& rules)
    {
        Voice counterpoint(voiceType);
        
        if (cantusFirmus.notes.empty())
            return counterpoint;
        
        switch (species)
        {
            case CounterpointSpecies::Second:
            {
                std::vector<Note> notes = generateSecondSpeciesNotes(cantusFirmus, voiceType, rules);
                for (const auto& note : notes)
                {
                    counterpoint.addNote(note);
                }
                break;
            }

            case CounterpointSpecies::Third:
            {
                std::vector<Note> notes = generateThirdSpeciesNotes(cantusFirmus, voiceType, rules);
                for (const auto& note : notes)
                {
                    counterpoint.addNote(note);
                }
                break;
            }

            case CounterpointSpecies::First:
            default:
                // First species and any unknown species default to first species
            {
                std::vector<int> notes = generateFirstSpeciesNotes(cantusFirmus, voiceType, rules);
                for (size_t i = 0; i < notes.size() && i < cantusFirmus.notes.size(); ++i)
                {
                    Note note(notes[i], cantusFirmus.notes[i].duration, 0.7f, cantusFirmus.notes[i].startTime);
                    counterpoint.addNote(note);
                }
                break;
            }
        }

        return counterpoint;
    }
    
    inline std::vector<int> BasicCounterpointGenerator::generateFirstSpeciesNotes(const Voice& cantusFirmus,
                                                                         VoiceType voiceType,
                                                                         const CounterpointRules& rules)
    {
        std::vector<int> notes;
        
        if (cantusFirmus.notes.empty())
            return notes;
        
        // Start with a consonant interval (unison, third, fifth, or octave)
        std::vector<int> startingCandidates = getCandidateNotes(cantusFirmus.notes[0].midiNumber, voiceType, rules);
        std::vector<int> consonantStarts;
        
        for (int candidate : startingCandidates)
        {
            int interval = calculateInterval(cantusFirmus.notes[0].midiNumber, candidate);
            if (isConsonantInterval(interval))
            {
                consonantStarts.push_back(candidate);
            }
        }
        
        if (!consonantStarts.empty())
        {
            // Prefer perfect consonances for opening
            int startNote = consonantStarts[0];
            for (int candidate : consonantStarts)
            {
                int interval = normalizeInterval(calculateInterval(cantusFirmus.notes[0].midiNumber, candidate));
                if (isPerfectConsonance(interval))
                {
                    startNote = candidate;
                    break;
                }
            }
            notes.push_back(startNote);
        }
        else
        {
            // Fallback to any candidate
            notes.push_back(startingCandidates.empty() ? 60 : startingCandidates[0]);
        }
        
        // Generate subsequent notes
        for (size_t i = 1; i < cantusFirmus.notes.size(); ++i)
        {
            std::vector<int> candidates = getCandidateNotes(cantusFirmus.notes[i].midiNumber, voiceType, rules);
            
            int bestNote = selectBestNote(candidates, notes.back(), cantusFirmus.notes[i].midiNumber, rules);
            notes.push_back(bestNote);
        }
        
        // Ensure final note is consonant (preferably unison or octave)
        if (notes.size() > 1)
        {
            int finalCF = cantusFirmus.notes.back().midiNumber;
            std::vector<int> finalCandidates = getCandidateNotes(finalCF, voiceType, rules);
            
            for (int candidate : finalCandidates)
            {
                int interval = normalizeInterval(calculateInterval(finalCF, candidate));
                if (interval == 0 || interval == 12) // Unison or octave
                {
                    notes.back() = candidate;
                    break;
                }
            }
        }
        
        return notes;
    }
    
    inline int BasicCounterpointGenerator::selectBestNote(const std::vector<int>& candidates,
                                                 int previousNote,
                                                 int cantusFirmusNote,
                                                 const CounterpointRules& rules)
    {
        if (candidates.empty())
            return previousNote; // Stay on same note if no candidates

        float bestScore = -1.0f;
        int bestNote = candidates[0];

        for (int candidate : candidates)
        {
            float score = evaluateNoteChoice(candidate, previousNote, cantusFirmusNote, rules);
            if (score > bestScore)
            {
                bestScore = score;
                bestNote = candidate;
            }
        }

        return bestNote;
    }
    
    inline std::vector<Note> BasicCounterpointGenerator::generateSecondSpeciesNotes(const Voice& cantusFirmus,
                                                                           VoiceType voiceType,
                                                                           const CounterpointRules& rules)
    {
        std::vector<Note> notes;
        
        for (size_t i = 0; i < cantusFirmus.notes.size(); ++i)
        {
            float cfDuration = cantusFirmus.notes[i].duration;
            float cfStartTime = cantusFirmus.notes[i].startTime;
            int cfNote = cantusFirmus.notes[i].midiNumber;
            
            // First note of the measure (downbeat) - must be consonant
            std::vector<int> candidates = getCandidateNotes(cfNote, voiceType, rules);
            std::vector<int> consonantCandidates;
            
            for (int candidate : candidates)
            {
                if (isConsonantInterval(calculateInterval(cfNote, candidate)))
                {
                    consonantCandidates.push_back(candidate);
                }
            }
            
            if (!consonantCandidates.empty())
            {
                int downbeatNote = consonantCandidates[0];
                if (i > 0 && !notes.empty())
                {
                    // Choose note with best voice leading from previous
                    downbeatNote = selectBestNote(consonantCandidates, notes.back().midiNumber, cfNote, rules);
                }
                
                Note downbeat(downbeatNote, cfDuration / 2.0f, 0.7f, cfStartTime);
                notes.push_back(downbeat);
                
                // Second note (upbeat) - can be dissonant if passing
                int upbeatNote = downbeatNote;
                if (rules.allowDissonantPassing && i < cantusFirmus.notes.size() - 1)
                {
                    // Create passing tone between downbeat and next downbeat
                    int nextCfNote = cantusFirmus.notes[i + 1].midiNumber;
                    int direction = (nextCfNote > cfNote) ? 1 : -1;
                    upbeatNote = downbeatNote + direction;
                    
                    // Ensure it's in voice range
                    Voice tempVoice(voiceType);
                    if (!tempVoice.range.contains(upbeatNote))
                    {
                        upbeatNote = downbeatNote; // Stay on same note
                    }
                }
                
                Note upbeat(upbeatNote, cfDuration / 2.0f, 0.6f, cfStartTime + cfDuration / 2.0f);
                notes.push_back(upbeat);
            }
        }
        
        return notes;
    }
    
    inline std::vector<Note> BasicCounterpointGenerator::generateThirdSpeciesNotes(const Voice& cantusFirmus,
                                                                          VoiceType voiceType,
                                                                          const CounterpointRules& rules)
    {
        std::vector<Note> notes;
        
        for (size_t i = 0; i < cantusFirmus.notes.size(); ++i)
        {
            float cfDuration = cantusFirmus.notes[i].duration;
            float cfStartTime = cantusFirmus.notes[i].startTime;
            int cfNote = cantusFirmus.notes[i].midiNumber;
            float noteDuration = cfDuration / 4.0f;
            
            std::vector<int> candidates = getCandidateNotes(cfNote, voiceType, rules);
            
            // Generate four notes against one cantus firmus note
            for (int j = 0; j < 4; ++j)
            {
                float startTime = cfStartTime + j * noteDuration;
                bool isDownbeat = (j == 0);
                
                int noteChoice;
                if (isDownbeat || !rules.allowDissonantPassing)
                {
                    // Downbeat or strict rules - must be consonant
                    std::vector<int> consonantCandidates;
                    for (int candidate : candidates)
                    {
                        if (isConsonantInterval(calculateInterval(cfNote, candidate)))
                        {
                            consonantCandidates.push_back(candidate);
                        }
                    }
                    
                    if (!consonantCandidates.empty())
                    {
                        noteChoice = consonantCandidates[0];
                        if (!notes.empty())
                        {
                            noteChoice = selectBestNote(consonantCandidates, notes.back().midiNumber, cfNote, rules);
                        }
                    }
                    else
                    {
                        noteChoice = candidates.empty() ? 60 : candidates[0];
                    }
                }
                else
                {
                    // Upbeat - can be dissonant passing tone
                    if (!notes.empty())
                    {
                        int previousNote = notes.back().midiNumber;
                        // Create stepwise motion
                        int direction = (j == 2) ? 1 : -1; // Alternate direction
                        noteChoice = previousNote + direction;
                        
                        // Ensure it's in voice range
                        Voice tempVoice(voiceType);
                        if (!tempVoice.range.contains(noteChoice))
                        {
                            noteChoice = previousNote;
                        }
                    }
                    else
                    {
                        noteChoice = candidates.empty() ? 60 : candidates[0];
                    }
                }
                
                Note note(noteChoice, noteDuration, 0.7f - j * 0.1f, startTime);
                notes.push_back(note);
            }
        }
        
        return notes;
    }
    
    inline std::vector<int> BasicCounterpointGenerator::generateMelodicLine(const Voice& mainMelody,
                                                                   const Key& key,
                                                                   VoiceType voiceType,
                                                                   const VoiceLeadingPath& path)
    {
        std::vector<int> melodicLine;
        Voice tempVoice(voiceType);
        
        if (mainMelody.notes.empty())
            return melodicLine;
        
        // Use path target notes if available, otherwise generate complementary line
        if (!path.targetNotes.empty())
        {
            for (size_t i = 0; i < path.targetNotes.size() && i < mainMelody.notes.size(); ++i)
            {
                int targetNote = path.targetNotes[i];
                if (tempVoice.range.contains(targetNote))
                {
                    melodicLine.push_back(targetNote);
                }
                else
                {
                    // Transpose to appropriate octave
                    while (targetNote < tempVoice.range.lowestNote)
                        targetNote += 12;
                    while (targetNote > tempVoice.range.highestNote)
                        targetNote -= 12;
                    melodicLine.push_back(targetNote);
                }
            }
        }
        else
        {
            // Generate complementary melodic line
            std::vector<int> scaleNotes = getScaleNotes(key, 4); // Start in 4th octave
            
            for (size_t i = 0; i < mainMelody.notes.size(); ++i)
            {
                int mainNote = mainMelody.notes[i].midiNumber;
                
                // Find complementary note in scale
                int bestNote = scaleNotes[0];
                int bestInterval = 12; // Start with octave
                
                for (int scaleNote : scaleNotes)
                {
                    if (tempVoice.range.contains(scaleNote))
                    {
                        int interval = std::abs(calculateInterval(mainNote, scaleNote));
                        int normalizedInterval = normalizeInterval(interval);
                        
                        // Prefer consonant intervals (3rd, 5th, 6th)
                        if (isConsonantInterval(normalizedInterval) && 
                            normalizedInterval != 0 && normalizedInterval != 12) // Avoid unison/octave
                        {
                            if (normalizedInterval == 3 || normalizedInterval == 4 || 
                                normalizedInterval == 8 || normalizedInterval == 9) // 3rds and 6ths
                            {
                                bestNote = scaleNote;
                                break;
                            }
                            else if (normalizedInterval == 7) // 5th
                            {
                                bestNote = scaleNote;
                            }
                        }
                    }
                }
                
                melodicLine.push_back(bestNote);
            }
        }
        
        return melodicLine;
    }
    
    inline bool BasicCounterpointGenerator::followsVoiceLeadingPath(const std::vector<int>& melody,
                                                           const VoiceLeadingPath& path)
    {
        if (path.targetNotes.empty())
            return true; // No specific path to follow
        
        size_t minSize = std::min(melody.size(), path.targetNotes.size());
        
        for (size_t i = 0; i < minSize; ++i)
        {
            // Allow some flexibility (within a whole step)
            if (std::abs(melody[i] - path.targetNotes[i]) > 2)
                return false;
        }
        
        return true;
    }
    
    inline int BasicCounterpointGenerator::calculateInterval(int note1, int note2)
    {
        return std::abs(note2 - note1);
    }
    
    inline int BasicCounterpointGenerator::normalizeInterval(int interval)
    {
        return interval % 12;
    }
    
    inline bool BasicCounterpointGenerator::isPerfectConsonance(int interval)
    {
        int normalized = normalizeInterval(interval);
        const auto& perfect = perfectConsonances();
        return std::find(perfect.begin(), perfect.end(), normalized) != perfect.end();
    }
    
    inline bool BasicCounterpointGenerator::isImperfectConsonance(int interval)
    {
        int normalized = normalizeInterval(interval);
        const auto& imperfect = imperfectConsonances();
        return std::find(imperfect.begin(), imperfect.end(), normalized) != imperfect.end();
    }
    
    inline bool BasicCounterpointGenerator::validateMelodicLine(const Voice& voice, const CounterpointRules& rules)
    {
        if (voice.notes.size() < 2)
            return false;
        
        // Check melodic intervals
        for (size_t i = 1; i < voice.notes.size(); ++i)
        {
            int interval = std::abs(voice.notes[i].midiNumber - voice.notes[i-1].midiNumber);
            if (interval > rules.maxMelodicLeap)
                return false;
        }
        
        // Check for proper melodic contour
        if (!hasValidMelodicContour(voice))
            return false;
        
        // Check for climax if required
        if (rules.requireClimax && !hasAppropriateClimax(voice))
            return false;
        
        return true;
    }
    
    inline bool BasicCounterpointGenerator::hasValidMelodicContour(const Voice& voice)
    {
        if (voice.notes.size() < 3)
            return true;
        
        MelodicContour contour = voiceLeadingEngine->analyzeMelodicMotion(voice);
        
        // Require reasonable amount of stepwise motion
        return contour.stepwiseMotionRatio >= 0.6f;
    }
    
    inline bool BasicCounterpointGenerator::hasAppropriateClimax(const Voice& voice)
    {
        if (voice.notes.size() < 3)
            return true;
        
        // Pre-existing bug: findMelodicClimax has two overloads (float(Voice,bool=true)
        // and int(Voice)), making a single-arg call ambiguous. Disambiguate to the
        // int-index overload via an explicit member-function-pointer cast.
        int (BasicCounterpointGenerator::*climaxFn)(const Voice&) = &BasicCounterpointGenerator::findMelodicClimax;
        int climaxIndex = (this->*climaxFn)(voice);
        
        // Climax should not be at the beginning or end
        return climaxIndex > 0 && climaxIndex < static_cast<int>(voice.notes.size()) - 1;
    }
    
    inline int BasicCounterpointGenerator::findMelodicClimax(const Voice& voice)
    {
        if (voice.notes.empty())
            return -1;
        
        int highestNote = voice.notes[0].midiNumber;
        int climaxIndex = 0;
        
        for (size_t i = 1; i < voice.notes.size(); ++i)
        {
            if (voice.notes[i].midiNumber > highestNote)
            {
                highestNote = voice.notes[i].midiNumber;
                climaxIndex = static_cast<int>(i);
            }
        }
        
        return climaxIndex;
    }
    
    inline bool BasicCounterpointGenerator::validateHarmonicIntervals(const std::vector<Voice>& voices,
                                                             const CounterpointRules& rules)
    {
        if (voices.size() < 2)
            return true;
        
        size_t minNotes = voices[0].notes.size();
        for (const auto& voice : voices)
        {
            minNotes = std::min(minNotes, voice.notes.size());
        }
        
        for (size_t i = 0; i < minNotes; ++i)
        {
            for (size_t v1 = 0; v1 < voices.size(); ++v1)
            {
                for (size_t v2 = v1 + 1; v2 < voices.size(); ++v2)
                {
                    int interval = calculateInterval(voices[v1].notes[i].midiNumber, 
                                                   voices[v2].notes[i].midiNumber);
                    
                    // Check if downbeat requires consonance
                    if (rules.requireConsonantDownbeats && i % 4 == 0) // Assuming 4 beats per measure
                    {
                        if (isDissonantInterval(interval))
                            return false;
                    }
                }
            }
        }
        
        return true;
    }
    
    inline bool BasicCounterpointGenerator::validateParallelMotion(const std::vector<Voice>& voices,
                                                          const CounterpointRules& rules)
    {
        if (voices.size() < 2)
            return true;
        
        size_t minNotes = voices[0].notes.size();
        for (const auto& voice : voices)
        {
            minNotes = std::min(minNotes, voice.notes.size());
        }
        
        for (size_t i = 1; i < minNotes; ++i)
        {
            for (size_t v1 = 0; v1 < voices.size(); ++v1)
            {
                for (size_t v2 = v1 + 1; v2 < voices.size(); ++v2)
                {
                    int interval1 = normalizeInterval(calculateInterval(
                        voices[v1].notes[i-1].midiNumber, voices[v2].notes[i-1].midiNumber));
                    int interval2 = normalizeInterval(calculateInterval(
                        voices[v1].notes[i].midiNumber, voices[v2].notes[i].midiNumber));
                    
                    // Check for parallel fifths
                    if (!rules.allowParallelFifths && interval1 == 7 && interval2 == 7)
                        return false;
                    
                    // Check for parallel octaves
                    if (!rules.allowParallelOctaves && 
                        (interval1 == 0 || interval1 == 12) && (interval2 == 0 || interval2 == 12))
                        return false;
                    
                    // Check for parallel unisons
                    if (!rules.allowParallelUnisons && interval1 == 0 && interval2 == 0)
                        return false;
                }
            }
        }
        
        return true;
    }
    
    inline float BasicCounterpointGenerator::calculateRhythmicIndependence(const std::vector<Voice>& voices)
    {
        if (voices.size() < 2)
            return 1.0f;
        
        // Simple implementation: check for simultaneous note attacks
        float totalAttacks = 0.0f;
        float independentAttacks = 0.0f;
        
        for (const auto& voice : voices)
        {
            for (const auto& note : voice.notes)
            {
                totalAttacks += 1.0f;
                
                // Check if any other voice has a note at the same time
                bool hasSimultaneous = false;
                for (const auto& otherVoice : voices)
                {
                    if (&otherVoice == &voice) continue;
                    
                    for (const auto& otherNote : otherVoice.notes)
                    {
                        if (std::abs(otherNote.startTime - note.startTime) < 0.01f)
                        {
                            hasSimultaneous = true;
                            break;
                        }
                    }
                    if (hasSimultaneous) break;
                }
                
                if (!hasSimultaneous)
                    independentAttacks += 1.0f;
            }
        }
        
        return (totalAttacks > 0) ? independentAttacks / totalAttacks : 1.0f;
    }
    
    inline float BasicCounterpointGenerator::calculateMelodicIndependence(const std::vector<Voice>& voices)
    {
        if (voices.size() < 2)
            return 1.0f;
        
        float totalIndependence = 0.0f;
        int comparisons = 0;
        
        for (size_t i = 0; i < voices.size(); ++i)
        {
            for (size_t j = i + 1; j < voices.size(); ++j)
            {
                // Calculate correlation between melodic contours
                MelodicContour contour1 = voiceLeadingEngine->analyzeMelodicMotion(voices[i]);
                MelodicContour contour2 = voiceLeadingEngine->analyzeMelodicMotion(voices[j]);
                
                // Simple independence measure based on different contour characteristics
                float independence = 1.0f - std::abs(contour1.overallDirection - contour2.overallDirection);
                independence *= 1.0f - std::abs(contour1.stepwiseMotionRatio - contour2.stepwiseMotionRatio);
                
                totalIndependence += independence;
                comparisons++;
            }
        }
        
        return (comparisons > 0) ? totalIndependence / comparisons : 1.0f;
    }
    
    inline bool BasicCounterpointGenerator::hasProperVoiceIndependence(const std::vector<Voice>& voices,
                                                              const CounterpointRules& rules)
    {
        float rhythmicIndep = calculateRhythmicIndependence(voices);
        float melodicIndep = calculateMelodicIndependence(voices);
        
        // Require at least 60% independence for strict counterpoint
        float minIndependence = (rules.enforceSpeciesRhythm) ? 0.6f : 0.4f;
        
        return (rhythmicIndep + melodicIndep) / 2.0f >= minIndependence;
    }
    
    inline std::vector<int> BasicCounterpointGenerator::getCandidateNotes(int cantusFirmusNote,
                                                                 VoiceType voiceType,
                                                                 const CounterpointRules& rules)
    {
        std::vector<int> candidates;
        Voice tempVoice(voiceType);
        
        // Generate candidates within voice range
        for (int note = tempVoice.range.lowestNote; note <= tempVoice.range.highestNote; ++note)
        {
            int interval = calculateInterval(cantusFirmusNote, note);
            
            // Skip notes that would create intervals larger than max leap
            if (interval <= rules.maxMelodicLeap)
            {
                candidates.push_back(note);
            }
        }
        
        return candidates;
    }
    
    inline float BasicCounterpointGenerator::evaluateNoteChoice(int note,
                                                       int previousNote,
                                                       int cantusFirmusNote,
                                                       const CounterpointRules& rules)
    {
        float score = 0.0f;
        
        // Prefer consonant intervals with cantus firmus
        int harmonicInterval = calculateInterval(cantusFirmusNote, note);
        if (isConsonantInterval(harmonicInterval))
        {
            score += 0.5f;
            
            // Prefer imperfect consonances over perfect
            if (isImperfectConsonance(harmonicInterval))
                score += 0.2f;
        }
        
        // Prefer stepwise melodic motion
        int melodicInterval = std::abs(note - previousNote);
        if (melodicInterval <= 2)
            score += 0.3f;
        else if (melodicInterval <= 4)
            score += 0.1f;
        
        // Penalize large leaps
        if (melodicInterval > rules.maxMelodicLeap)
            score -= 0.5f;
        
        return score;
    }
    
    inline std::vector<int> BasicCounterpointGenerator::getScaleNotes(const Key& key, int octave)
    {
        std::vector<int> scaleNotes;
        std::vector<int> scaleDegrees = key.getScaleDegrees();
        
        for (int degree : scaleDegrees)
        {
            scaleNotes.push_back(degree + octave * 12);
        }
        
        return scaleNotes;
    }
    
    inline bool BasicCounterpointGenerator::isInKey(int note, const Key& key)
    {
        return key.containsNote(note % 12);
    }
    
    inline int BasicCounterpointGenerator::getClosestScaleDegree(int note, const Key& key)
    {
        std::vector<int> scaleDegrees = key.getScaleDegrees();
        int noteClass = note % 12;
        
        int closestDegree = scaleDegrees[0];
        int minDistance = 12;
        
        for (int degree : scaleDegrees)
        {
            int distance = std::min(std::abs(noteClass - degree), 12 - std::abs(noteClass - degree));
            if (distance < minDistance)
            {
                minDistance = distance;
                closestDegree = degree;
            }
        }
        
        return closestDegree + (note / 12) * 12;
    }
    
    //==============================================================================
    // Enhanced Counter-melody Generation Implementation
    
    inline Voice BasicCounterpointGenerator::generateCounterMelodyWithConstraints(const Voice& mainMelody,
                                                                         const CounterMelodyConstraints& constraints,
                                                                         const VoiceLeadingPath& path)
    {
        Voice counterMelody(constraints.voiceType);
        
        if (mainMelody.notes.empty())
            return counterMelody;
        
        // Analyze main melody contour
        MelodicContourProfile mainContour = analyzeMelodicContour(mainMelody);
        
        // Create complementary contour if requested
        MelodicContourProfile targetContour = constraints.useComplementaryContour ? 
            createComplementaryContour(mainContour) : mainContour;
        
        // Generate melodic line with constraints
        std::vector<int> melodicLine = generateConstrainedMelodicLine(mainMelody, constraints, path);
        
        // Convert to notes with appropriate timing
        for (size_t i = 0; i < melodicLine.size() && i < mainMelody.notes.size(); ++i)
        {
            float duration = (i < path.rhythmicValues.size()) ? path.rhythmicValues[i] : mainMelody.notes[i].duration;
            float startTime = mainMelody.notes[i].startTime;
            
            // Add rhythmic independence if requested
            if (constraints.rhythmicIndependence > 0.5f && i > 0)
            {
                // Slightly offset timing for independence
                startTime += (constraints.rhythmicIndependence - 0.5f) * duration * 0.1f;
            }
            
            Note note(melodicLine[i], duration, 0.7f, startTime);
            counterMelody.addNote(note);
        }
        
        return counterMelody;
    }
    
    inline std::vector<Voice> BasicCounterpointGenerator::generateMultipleCounterMelodies(const Voice& mainMelody,
                                                                                 const std::vector<CounterMelodyConstraints>& constraints,
                                                                                 const VoiceLeadingPath& path)
    {
        std::vector<Voice> counterMelodies;
        
        if (mainMelody.notes.empty() || constraints.empty())
            return counterMelodies;
        
        // Generate each counter-melody
        for (const auto& constraint : constraints)
        {
            Voice counterMelody = generateCounterMelodyWithConstraints(mainMelody, constraint, path);
            counterMelodies.push_back(counterMelody);
        }
        
        // Validate that all melodies work together harmonically
        std::vector<Voice> allVoices = {mainMelody};
        allVoices.insert(allVoices.end(), counterMelodies.begin(), counterMelodies.end());
        
        if (!validateCounterMelodyHarmony(allVoices, path.key))
        {
            // If validation fails, regenerate with more conservative constraints
            counterMelodies.clear();
            for (const auto& constraint : constraints)
            {
                CounterMelodyConstraints conservativeConstraint = constraint;
                conservativeConstraint.restrictToScaleTones = true;
                conservativeConstraint.rhythmicIndependence *= 0.7f;
                
                Voice counterMelody = generateCounterMelodyWithConstraints(mainMelody, conservativeConstraint, path);
                counterMelodies.push_back(counterMelody);
            }
        }
        
        return counterMelodies;
    }
    
    inline MelodicContourProfile BasicCounterpointGenerator::analyzeMelodicContour(const Voice& melody)
    {
        MelodicContourProfile profile;
        
        if (melody.notes.size() < 2)
            return profile;
        
        // Calculate direction changes
        profile.directionChanges = calculateDirectionChanges(melody);
        
        // Calculate interval sizes
        profile.intervalSizes = calculateIntervalSizes(melody);
        
        // Calculate overall range
        auto minmax = std::minmax_element(melody.notes.begin(), melody.notes.end(),
            [](const Note& a, const Note& b) { return a.midiNumber < b.midiNumber; });
        int minNote = minmax.first->midiNumber;
        int maxNote = minmax.second->midiNumber;
        profile.overallRange = static_cast<float>(maxNote - minNote);
        
        // Find climax position
        profile.climaxPosition = findMelodicClimax(melody, true);
        
        // Determine overall direction
        profile.isAscending = isOverallAscending(melody);
        
        return profile;
    }
    
    inline MelodicContourProfile BasicCounterpointGenerator::createComplementaryContour(const MelodicContourProfile& mainContour)
    {
        MelodicContourProfile complementary = mainContour;
        
        // Invert overall direction
        complementary.isAscending = !mainContour.isAscending;
        
        // Place climax in opposite position
        complementary.climaxPosition = 1.0f - mainContour.climaxPosition;
        
        // Use smaller range for complementary melody
        complementary.overallRange = mainContour.overallRange * 0.7f;
        
        // Invert direction changes (where main goes up, complementary goes down)
        for (auto& change : complementary.directionChanges)
        {
            change = -change;
        }
        
        return complementary;
    }
    
    //==============================================================================
    // Enhanced Counter-melody Helper Methods
    
    inline std::vector<int> BasicCounterpointGenerator::generateConstrainedMelodicLine(const Voice& mainMelody,
                                                                              const CounterMelodyConstraints& constraints,
                                                                              const VoiceLeadingPath& path)
    {
        std::vector<int> melodicLine;
        Voice tempVoice(constraints.voiceType);
        
        if (mainMelody.notes.empty())
            return melodicLine;
        
        // Get available notes based on constraints
        std::vector<int> availableNotes;
        if (constraints.restrictToScaleTones)
        {
            availableNotes = getScaleTonesInRange(constraints.scale, 
                                                tempVoice.range.lowestNote, 
                                                tempVoice.range.highestNote);
        }
        else
        {
            availableNotes = getPassingTonesInRange(constraints.scale,
                                                  tempVoice.range.lowestNote,
                                                  tempVoice.range.highestNote,
                                                  path.allowChromaticPassing);
        }
        
        // Use path target notes if available and path strength is high
        if (!path.targetNotes.empty() && path.pathStrength > 0.5f)
        {
            for (size_t i = 0; i < path.targetNotes.size() && i < mainMelody.notes.size(); ++i)
            {
                int targetNote = path.targetNotes[i];
                
                // Adjust to voice range if necessary
                while (targetNote < tempVoice.range.lowestNote)
                    targetNote += 12;
                while (targetNote > tempVoice.range.highestNote)
                    targetNote -= 12;
                
                // If restricting to scale tones, find closest scale tone
                if (constraints.restrictToScaleTones && !isScaleTone(targetNote, constraints.scale))
                {
                    targetNote = getClosestScaleTone(targetNote, constraints.scale);
                }
                
                melodicLine.push_back(targetNote);
            }
        }
        else
        {
            // Generate complementary melodic line
            MelodicContourProfile mainContour = analyzeMelodicContour(mainMelody);
            MelodicContourProfile targetContour = constraints.useComplementaryContour ? 
                createComplementaryContour(mainContour) : mainContour;
            
            melodicLine = createComplementaryMelody(mainMelody, targetContour, constraints);
        }
        
        return melodicLine;
    }
    
    inline std::vector<int> BasicCounterpointGenerator::createComplementaryMelody(const Voice& mainMelody,
                                                                         const MelodicContourProfile& targetContour,
                                                                         const CounterMelodyConstraints& constraints)
    {
        std::vector<int> melody;
        Voice tempVoice(constraints.voiceType);
        
        if (mainMelody.notes.empty())
            return melody;
        
        // Start with a consonant interval from the first main melody note
        int firstMainNote = mainMelody.notes[0].midiNumber;
        std::vector<int> consonantIntervals = {3, 4, 7, 8, 9}; // 3rd, 5th, 6th
        
        int startNote = tempVoice.range.lowestNote + (tempVoice.range.highestNote - tempVoice.range.lowestNote) / 2;
        for (int interval : consonantIntervals)
        {
            int candidate = firstMainNote + (targetContour.isAscending ? -interval : interval);
            if (tempVoice.range.contains(candidate))
            {
                if (!constraints.restrictToScaleTones || isScaleTone(candidate, constraints.scale))
                {
                    startNote = candidate;
                    break;
                }
            }
        }
        melody.push_back(startNote);
        
        // Generate subsequent notes following the target contour
        for (size_t i = 1; i < mainMelody.notes.size(); ++i)
        {
            int previousNote = melody.back();
            int mainNote = mainMelody.notes[i].midiNumber;
            int previousMainNote = mainMelody.notes[i-1].midiNumber;
            
            // Determine direction based on complementary contour
            int mainDirection = (mainNote > previousMainNote) ? 1 : -1;
            int targetDirection = targetContour.isAscending ? mainDirection : -mainDirection;
            
            // Use direction changes from target contour
            if (i-1 < targetContour.directionChanges.size())
            {
                targetDirection = (targetContour.directionChanges[i-1] > 0) ? 1 : -1;
            }
            
            // Calculate step size (prefer stepwise motion)
            int stepSize = 1; // Default to stepwise
            if (i-1 < targetContour.intervalSizes.size())
            {
                stepSize = std::max(1, static_cast<int>(targetContour.intervalSizes[i-1] / 2));
            }
            
            int nextNote = previousNote + (targetDirection * stepSize);
            
            // Ensure note is in range
            if (!tempVoice.range.contains(nextNote))
            {
                nextNote = previousNote + (targetDirection * -stepSize); // Try opposite direction
                if (!tempVoice.range.contains(nextNote))
                {
                    nextNote = previousNote; // Stay on same note
                }
            }
            
            // Ensure note follows scale constraints
            if (constraints.restrictToScaleTones && !isScaleTone(nextNote, constraints.scale))
            {
                nextNote = getClosestScaleTone(nextNote, constraints.scale);
            }
            
            // Avoid notes specified in constraints
            if (i < constraints.avoidNotes.size() && nextNote == constraints.avoidNotes[i])
            {
                nextNote += (targetDirection > 0) ? 1 : -1;
                if (constraints.restrictToScaleTones && !isScaleTone(nextNote, constraints.scale))
                {
                    nextNote = getClosestScaleTone(nextNote, constraints.scale);
                }
            }
            
            melody.push_back(nextNote);
        }
        
        return melody;
    }
    
    inline bool BasicCounterpointGenerator::validateCounterMelodyHarmony(const std::vector<Voice>& melodies,
                                                                const Key& key)
    {
        if (melodies.size() < 2)
            return true;
        
        // Check harmonic intervals at each time point
        size_t minNotes = melodies[0].notes.size();
        for (const auto& melody : melodies)
        {
            minNotes = std::min(minNotes, melody.notes.size());
        }
        
        for (size_t i = 0; i < minNotes; ++i)
        {
            // Check all pairs of voices
            for (size_t v1 = 0; v1 < melodies.size(); ++v1)
            {
                for (size_t v2 = v1 + 1; v2 < melodies.size(); ++v2)
                {
                    int interval = calculateInterval(melodies[v1].notes[i].midiNumber,
                                                   melodies[v2].notes[i].midiNumber);
                    
                    // Require consonant intervals on strong beats
                    if (i % 4 == 0 && isDissonantInterval(interval))
                    {
                        return false;
                    }
                    
                    // Check for excessive dissonance
                    if (isDissonantInterval(interval))
                    {
                        // Allow passing dissonances but not too many in a row
                        int dissonantCount = 1;
                        for (size_t j = i + 1; j < minNotes && j < i + 3; ++j)
                        {
                            int nextInterval = calculateInterval(melodies[v1].notes[j].midiNumber,
                                                               melodies[v2].notes[j].midiNumber);
                            if (isDissonantInterval(nextInterval))
                                dissonantCount++;
                        }
                        
                        if (dissonantCount > 2) // Too many consecutive dissonances
                            return false;
                    }
                }
            }
        }
        
        return true;
    }
    
    //==============================================================================
    // Melodic Contour Analysis Helper Methods
    
    inline std::vector<float> BasicCounterpointGenerator::calculateDirectionChanges(const Voice& melody)
    {
        std::vector<float> changes;
        
        if (melody.notes.size() < 3)
            return changes;
        
        for (size_t i = 1; i < melody.notes.size() - 1; ++i)
        {
            int prevInterval = melody.notes[i].midiNumber - melody.notes[i-1].midiNumber;
            int nextInterval = melody.notes[i+1].midiNumber - melody.notes[i].midiNumber;
            
            // Detect direction change
            if ((prevInterval > 0 && nextInterval < 0) || (prevInterval < 0 && nextInterval > 0))
            {
                changes.push_back(static_cast<float>(nextInterval));
            }
            else
            {
                changes.push_back(0.0f); // No direction change
            }
        }
        
        return changes;
    }
    
    inline std::vector<float> BasicCounterpointGenerator::calculateIntervalSizes(const Voice& melody)
    {
        std::vector<float> intervals;
        
        for (size_t i = 1; i < melody.notes.size(); ++i)
        {
            int interval = std::abs(melody.notes[i].midiNumber - melody.notes[i-1].midiNumber);
            intervals.push_back(static_cast<float>(interval));
        }
        
        return intervals;
    }
    
    inline float BasicCounterpointGenerator::findMelodicClimax(const Voice& melody, bool returnPosition)
    {
        if (melody.notes.empty())
            return returnPosition ? 0.5f : 60.0f;
        
        int highestNote = melody.notes[0].midiNumber;
        size_t climaxIndex = 0;
        
        for (size_t i = 1; i < melody.notes.size(); ++i)
        {
            if (melody.notes[i].midiNumber > highestNote)
            {
                highestNote = melody.notes[i].midiNumber;
                climaxIndex = i;
            }
        }
        
        if (returnPosition)
        {
            return static_cast<float>(climaxIndex) / static_cast<float>(melody.notes.size() - 1);
        }
        else
        {
            return static_cast<float>(highestNote);
        }
    }
    
    inline bool BasicCounterpointGenerator::isOverallAscending(const Voice& melody)
    {
        if (melody.notes.size() < 2)
            return true;
        
        return melody.notes.back().midiNumber > melody.notes.front().midiNumber;
    }
    
    //==============================================================================
    // Scale and Harmony Utility Methods
    
    inline std::vector<int> BasicCounterpointGenerator::getScaleTonesInRange(const Key& scale, int lowestNote, int highestNote)
    {
        std::vector<int> scaleNotes;
        scaleNotes.reserve(static_cast<size_t>(highestNote - lowestNote + 1));

        for (int note = lowestNote; note <= highestNote; ++note)
        {
            if (isScaleTone(note, scale))
            {
                scaleNotes.push_back(note);
            }
        }

        return scaleNotes;
    }
    
    inline std::vector<int> BasicCounterpointGenerator::getPassingTonesInRange(const Key& scale, int lowestNote, int highestNote, bool chromatic)
    {
        std::vector<int> notes;
        notes.reserve(static_cast<size_t>(highestNote - lowestNote + 1));

        if (chromatic)
        {
            // Include all chromatic notes
            for (int note = lowestNote; note <= highestNote; ++note)
            {
                notes.push_back(note);
            }
        }
        else
        {
            // Include scale tones plus diatonic passing tones
            for (int note = lowestNote; note <= highestNote; ++note)
            {
                if (isScaleTone(note, scale))
                {
                    notes.push_back(note);
                }
                else
                {
                    // Check if it's a valid passing tone between scale tones
                    int prevScaleTone = getClosestScaleTone(note - 1, scale);
                    int nextScaleTone = getClosestScaleTone(note + 1, scale);
                    
                    if (std::abs(prevScaleTone - nextScaleTone) <= 2) // Within a whole step
                    {
                        notes.push_back(note);
                    }
                }
            }
        }
        
        return notes;
    }
    
    inline bool BasicCounterpointGenerator::isScaleTone(int note, const Key& scale)
    {
        return scale.containsNote(note % 12);
    }
    
    inline bool BasicCounterpointGenerator::isPassingTone(int note, int previousNote, int nextNote, const Key& scale)
    {
        // A passing tone connects two scale tones by step
        if (isScaleTone(note, scale))
            return false; // Scale tones are not passing tones
        
        bool prevIsScale = isScaleTone(previousNote, scale);
        bool nextIsScale = isScaleTone(nextNote, scale);
        
        if (!prevIsScale || !nextIsScale)
            return false;
        
        // Check if it's stepwise motion
        int prevInterval = std::abs(note - previousNote);
        int nextInterval = std::abs(nextNote - note);
        
        return (prevInterval <= 2 && nextInterval <= 2);
    }
    
    inline int BasicCounterpointGenerator::getClosestScaleTone(int note, const Key& scale, bool preferUp)
    {
        std::vector<int> scaleDegrees = scale.getScaleDegrees();
        int noteClass = note % 12;
        int octave = note / 12;
        
        // Find closest scale degree
        int closestDegree = scaleDegrees[0];
        int minDistance = 12;
        
        for (int degree : scaleDegrees)
        {
            int distance = std::abs(noteClass - degree);
            if (distance > 6) // Handle wrap-around
                distance = 12 - distance;
            
            if (distance < minDistance || (distance == minDistance && preferUp && degree > noteClass))
            {
                minDistance = distance;
                closestDegree = degree;
            }
        }
        
        return closestDegree + octave * 12;
    }
    
} // namespace HarmonyEngine
