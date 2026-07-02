/*
  ==============================================================================

    VoiceLeadingEngine.h
    Created: Voice leading engine interface and implementation
    
    This file contains the VoiceLeadingEngine interface and BasicVoiceLeadingEngine
    implementation that provides voice leading algorithms, voice range validation,
    voice crossing detection, and parallel motion avoidance.

  ==============================================================================
*/

#pragma once

// Implementation includes (header-only)
#include <algorithm>
#include <cmath>
#include <numeric>
#include <set>

#include "MusicTheory.h"
#include <vector>
#include <memory>

namespace HarmonyEngine
{
    //==============================================================================
    // Voice Leading Constraints and Configuration
    
    struct VoicingConstraints
    {
        bool allowVoiceCrossing;
        bool strictParallelRules; // Strict classical rules vs. contemporary flexibility
        float maxVoiceLeap; // Maximum interval leap in semitones
        bool prioritizeCommonTones;
        bool enforceVoiceRanges;
        
        VoicingConstraints()
            : allowVoiceCrossing(false), strictParallelRules(true), 
              maxVoiceLeap(12.0f), prioritizeCommonTones(true), 
              enforceVoiceRanges(true) {}
    };
    
    //==============================================================================
    // Voice Leading Analysis Results
    
    struct VoiceMovement
    {
        int fromNote;
        int toNote;
        int interval; // Signed interval in semitones
        bool isCommonTone;
        bool isStepwise; // Within 2 semitones
        
        VoiceMovement(int from = 0, int to = 0)
            : fromNote(from), toNote(to), interval(to - from),
              isCommonTone(from == to), isStepwise(std::abs(interval) <= 2) {}
    };
    
    struct VoicingResult
    {
        std::vector<Voice> voices;
        std::vector<VoiceMovement> movements; // Movement for each voice
        float smoothnessScore; // 0.0 to 1.0, higher is smoother
        bool hasVoiceCrossing;
        bool hasParallelFifths;
        bool hasParallelOctaves;
        std::vector<std::string> ruleViolations;
        
        VoicingResult() 
            : smoothnessScore(0.0f), hasVoiceCrossing(false),
              hasParallelFifths(false), hasParallelOctaves(false) {}
        
        bool isValid() const
        {
            return !voices.empty() && ruleViolations.empty();
        }
        
        size_t getVoiceCount() const { return voices.size(); }
    };
    
    struct MelodicContour
    {
        std::vector<int> intervalVector; // Sequence of melodic intervals
        std::vector<int> directionChanges; // Indices where direction changes
        float stepwiseMotionRatio; // Percentage of stepwise motion
        float averageLeapSize; // Average size of leaps (intervals > 2 semitones)
        int highestNote;
        int lowestNote;
        int range; // Ambitus in semitones
        
        // Enhanced contour analysis
        float melodicComplexity; // 0.0 to 1.0, based on interval variety and direction changes
        float leapRecoveryRatio; // Percentage of leaps followed by stepwise motion in opposite direction
        std::vector<int> peakIndices; // Indices of melodic peaks (local maxima)
        std::vector<int> valleyIndices; // Indices of melodic valleys (local minima)
        float overallDirection; // -1.0 (descending) to 1.0 (ascending), 0.0 (balanced)
        int largestLeap; // Largest interval in semitones
        int largestLeapIndex; // Index of the largest leap
        float rhythmicVariety; // Variety in note durations (0.0 to 1.0)
        
        MelodicContour()
            : stepwiseMotionRatio(0.0f), averageLeapSize(0.0f),
              highestNote(0), lowestNote(127), range(0),
              melodicComplexity(0.0f), leapRecoveryRatio(0.0f),
              overallDirection(0.0f), largestLeap(0), largestLeapIndex(-1),
              rhythmicVariety(0.0f) {}
    };
    
    //==============================================================================
    // Parallel Motion Detection Results
    
    enum class MotionType
    {
        Parallel,    // Same direction, same interval
        Similar,     // Same direction, different interval
        Contrary,    // Opposite directions
        Oblique      // One voice static, other moves
    };
    
    struct ParallelMotionAnalysis
    {
        MotionType motionType;
        int interval1; // Harmonic interval in first chord
        int interval2; // Harmonic interval in second chord
        bool isParallelFifth;
        bool isParallelOctave;
        bool isParallelUnison;
        int voiceIndex1; // Index of first voice in parallel motion
        int voiceIndex2; // Index of second voice in parallel motion
        
        ParallelMotionAnalysis()
            : motionType(MotionType::Oblique), interval1(0), interval2(0),
              isParallelFifth(false), isParallelOctave(false), isParallelUnison(false),
              voiceIndex1(-1), voiceIndex2(-1) {}
    };
    
    //==============================================================================
    // VoiceLeadingEngine Interface
    
    class VoiceLeadingEngine
    {
    public:
        virtual ~VoiceLeadingEngine() = default;
        
        // Core voice leading generation
        virtual VoicingResult generateVoicing(const std::vector<Chord>& progression, 
                                            const VoicingConstraints& constraints) = 0;
        
        // Voice analysis
        virtual MelodicContour analyzeMelodicMotion(const Voice& voice) = 0;
        virtual float calculateSmoothness(const VoicingResult& voicing) = 0;
        
        // Enhanced melodic contour analysis
        virtual std::vector<int> findMelodicPeaksAndValleys(const Voice& voice, bool findPeaks = true) = 0;
        virtual float calculateMelodicComplexity(const Voice& voice) = 0;
        virtual float calculateLeapRecoveryRatio(const Voice& voice) = 0;
        virtual float calculateOverallDirection(const Voice& voice) = 0;
        virtual bool validateMelodicIntegrity(const Voice& voice, float maxComplexity = 0.8f) = 0;
        
        // Voice validation
        virtual bool validateVoiceRanges(const std::vector<Voice>& voices) = 0;
        virtual bool detectVoiceCrossing(const std::vector<Voice>& voices, int chordIndex) = 0;
        
        // Parallel motion detection
        virtual std::vector<ParallelMotionAnalysis> detectParallelMotion(
            const std::vector<Voice>& voices, int fromChordIndex, int toChordIndex) = 0;
        virtual bool hasParallelFifths(const std::vector<Voice>& voices, 
                                     int fromChordIndex, int toChordIndex) = 0;
        virtual bool hasParallelOctaves(const std::vector<Voice>& voices, 
                                      int fromChordIndex, int toChordIndex) = 0;
        
        // Voice leading utilities
        virtual std::vector<int> findCommonTones(const Chord& chord1, const Chord& chord2) = 0;
        virtual VoiceMovement calculateMovement(int fromNote, int toNote) = 0;
        virtual float calculateVoiceIndependence(const std::vector<Voice>& voices) = 0;
    };
    
    //==============================================================================
    // Basic Implementation
    
    class BasicVoiceLeadingEngine : public VoiceLeadingEngine
    {
    public:
        BasicVoiceLeadingEngine();
        ~BasicVoiceLeadingEngine() override = default;
        
        // VoiceLeadingEngine interface implementation
        VoicingResult generateVoicing(const std::vector<Chord>& progression, 
                                    const VoicingConstraints& constraints) override;
        
        MelodicContour analyzeMelodicMotion(const Voice& voice) override;
        float calculateSmoothness(const VoicingResult& voicing) override;
        
        // Enhanced melodic contour analysis
        std::vector<int> findMelodicPeaksAndValleys(const Voice& voice, bool findPeaks = true) override;
        float calculateMelodicComplexity(const Voice& voice) override;
        float calculateLeapRecoveryRatio(const Voice& voice) override;
        float calculateOverallDirection(const Voice& voice) override;
        bool validateMelodicIntegrity(const Voice& voice, float maxComplexity = 0.8f) override;
        
        bool validateVoiceRanges(const std::vector<Voice>& voices) override;
        bool detectVoiceCrossing(const std::vector<Voice>& voices, int chordIndex) override;
        
        std::vector<ParallelMotionAnalysis> detectParallelMotion(
            const std::vector<Voice>& voices, int fromChordIndex, int toChordIndex) override;
        bool hasParallelFifths(const std::vector<Voice>& voices, 
                             int fromChordIndex, int toChordIndex) override;
        bool hasParallelOctaves(const std::vector<Voice>& voices, 
                              int fromChordIndex, int toChordIndex) override;
        
        std::vector<int> findCommonTones(const Chord& chord1, const Chord& chord2) override;
        VoiceMovement calculateMovement(int fromNote, int toNote) override;
        float calculateVoiceIndependence(const std::vector<Voice>& voices) override;
        
    private:
        // Voice leading generation helpers
        VoicingResult generateSATBVoicing(const std::vector<Chord>& progression, 
                                        const VoicingConstraints& constraints);
        std::vector<Voice> initializeVoices(const Chord& firstChord, 
                                          const VoicingConstraints& constraints);
        void voiceChord(const Chord& chord, std::vector<Voice>& voices, 
                       const VoicingConstraints& constraints, int chordIndex);
        
        // Voice assignment algorithms
        std::vector<int> assignNotesToVoices(const Chord& chord, 
                                           const std::vector<Voice>& currentVoices,
                                           const VoicingConstraints& constraints);
        float calculateAssignmentCost(const std::vector<int>& assignment,
                                    const Chord& chord,
                                    const std::vector<Voice>& currentVoices);
        
        // Common tone prioritization
        std::vector<int> prioritizeCommonTones(const Chord& fromChord, const Chord& toChord,
                                             const std::vector<Voice>& voices);
        
        // Voice range utilities
        Range getDefaultVoiceRange(VoiceType voiceType);
        bool isNoteInVoiceRange(int midiNote, VoiceType voiceType);
        
        // Parallel motion detection helpers
        MotionType classifyMotion(int voice1From, int voice1To, int voice2From, int voice2To);
        int calculateHarmonicInterval(int note1, int note2);
        bool isParallelInterval(int interval1, int interval2, int targetInterval);
        
        // Smoothness calculation helpers
        float calculateMelodicSmoothness(const Voice& voice);
        float calculateHarmonicSmoothness(const std::vector<Voice>& voices);
        float calculateRuleCompliance(const VoicingResult& voicing);
        
        // Voice independence measurement
        float calculateRhythmicIndependence(const std::vector<Voice>& voices);
        float calculateMelodicIndependence(const std::vector<Voice>& voices);
        float calculateContourIndependence(const std::vector<Voice>& voices);
        
        // Validation helpers
        std::vector<std::string> validateVoiceLeadingRules(const VoicingResult& voicing,
                                                          const VoicingConstraints& constraints);
    };
    
} // namespace HarmonyEngine

// ============================================================================
// Implementation (merged from .cpp - header-only library)
// ============================================================================

namespace HarmonyEngine
{
    //==============================================================================
    // BasicVoiceLeadingEngine Implementation
    
    inline BasicVoiceLeadingEngine::BasicVoiceLeadingEngine()
    {
    }
    
    inline VoicingResult BasicVoiceLeadingEngine::generateVoicing(const std::vector<Chord>& progression, 
                                                         const VoicingConstraints& constraints)
    {
        VoicingResult result;
        
        if (progression.empty())
        {
            result.ruleViolations.push_back("Empty chord progression");
            return result;
        }
        
        // Generate SATB voicing for the progression
        result = generateSATBVoicing(progression, constraints);
        
        // Calculate smoothness score
        result.smoothnessScore = calculateSmoothness(result);
        
        // Validate voice leading rules
        result.ruleViolations = validateVoiceLeadingRules(result, constraints);
        
        return result;
    }
    
    inline VoicingResult BasicVoiceLeadingEngine::generateSATBVoicing(const std::vector<Chord>& progression, 
                                                             const VoicingConstraints& constraints)
    {
        VoicingResult result;
        
        // Initialize four voices (SATB)
        result.voices = initializeVoices(progression[0], constraints);
        
        // Voice each chord in the progression
        for (size_t i = 0; i < progression.size(); ++i)
        {
            voiceChord(progression[i], result.voices, constraints, static_cast<int>(i));
        }
        
        // Analyze voice movements
        if (progression.size() > 1)
        {
            for (const auto& voice : result.voices)
            {
                if (voice.notes.size() >= 2)
                {
                    result.movements.reserve(result.movements.size() + voice.notes.size() - 1);
                    for (size_t noteIndex = 1; noteIndex < voice.notes.size(); ++noteIndex)
                    {
                        result.movements.emplace_back(
                            voice.notes[noteIndex - 1].midiNumber,
                            voice.notes[noteIndex].midiNumber);
                    }
                }
            }
        }
        
        // Check for voice crossing and parallel motion
        for (size_t i = 1; i < progression.size(); ++i)
        {
            if (detectVoiceCrossing(result.voices, static_cast<int>(i)))
                result.hasVoiceCrossing = true;
                
            if (hasParallelFifths(result.voices, static_cast<int>(i-1), static_cast<int>(i)))
                result.hasParallelFifths = true;
                
            if (hasParallelOctaves(result.voices, static_cast<int>(i-1), static_cast<int>(i)))
                result.hasParallelOctaves = true;
        }
        
        return result;
    }
    
    inline std::vector<Voice> BasicVoiceLeadingEngine::initializeVoices(const Chord& firstChord, 
                                                               const VoicingConstraints& constraints)
    {
        std::vector<Voice> voices;
        
        // Create SATB voices with appropriate ranges
        voices.emplace_back(VoiceType::Soprano);
        voices.emplace_back(VoiceType::Alto);
        voices.emplace_back(VoiceType::Tenor);
        voices.emplace_back(VoiceType::Bass);
        
        // Voice the first chord with close position
        if (firstChord.notes.size() >= 3)
        {
            // Sort chord notes by pitch
            auto sortedNotes = firstChord.notes;
            std::sort(sortedNotes.begin(), sortedNotes.end(), 
                     [](const Note& a, const Note& b) { return a.midiNumber < b.midiNumber; });
            
            // Assign notes to voices, starting from bass
            int bassNote = sortedNotes[0].midiNumber;
            int tenorNote = sortedNotes.size() > 1 ? sortedNotes[1].midiNumber : bassNote + 4;
            int altoNote = sortedNotes.size() > 2 ? sortedNotes[2].midiNumber : tenorNote + 3;
            int sopranoNote = sortedNotes.size() > 3 ? sortedNotes[3].midiNumber : altoNote + 4;
            
            // Adjust notes to fit voice ranges
            while (!isNoteInVoiceRange(bassNote, VoiceType::Bass))
                bassNote += 12;
            while (!isNoteInVoiceRange(tenorNote, VoiceType::Tenor))
                tenorNote += 12;
            while (!isNoteInVoiceRange(altoNote, VoiceType::Alto))
                altoNote += 12;
            while (!isNoteInVoiceRange(sopranoNote, VoiceType::Soprano))
                sopranoNote += 12;
            
            // Add notes to voices
            voices[3].addNote(Note(bassNote, 1.0f, 0.8f, 0.0f));      // Bass
            voices[2].addNote(Note(tenorNote, 1.0f, 0.8f, 0.0f));     // Tenor
            voices[1].addNote(Note(altoNote, 1.0f, 0.8f, 0.0f));      // Alto
            voices[0].addNote(Note(sopranoNote, 1.0f, 0.8f, 0.0f));   // Soprano
        }
        
        return voices;
    }
    
    inline void BasicVoiceLeadingEngine::voiceChord(const Chord& chord, std::vector<Voice>& voices,
                                           const VoicingConstraints& constraints, int chordIndex)
    {
        if (chordIndex == 0 || voices.empty())
            return; // First chord already voiced in initialization

        // Find optimal note assignment for this chord
        auto assignment = assignNotesToVoices(chord, voices, constraints);
        
        // Add notes to voices
        float startTime = static_cast<float>(chordIndex);
        for (size_t i = 0; i < voices.size() && i < assignment.size(); ++i)
        {
            voices[i].addNote(Note(assignment[i], 1.0f, 0.8f, startTime));
        }
    }
    
    inline std::vector<int> BasicVoiceLeadingEngine::assignNotesToVoices(const Chord& chord, 
                                                                const std::vector<Voice>& currentVoices,
                                                                const VoicingConstraints& constraints)
    {
        std::vector<int> assignment(currentVoices.size());
        
        if (chord.notes.empty())
        {
            // Return current positions if no chord notes
            for (size_t i = 0; i < currentVoices.size(); ++i)
            {
                if (!currentVoices[i].notes.empty())
                    assignment[i] = currentVoices[i].notes.back().midiNumber;
                else
                    assignment[i] = 60; // Default
            }
            return assignment;
        }
        
        // Get available chord tones in multiple octaves
        std::vector<int> availableNotes;
        availableNotes.reserve(chord.notes.size() * 5);
        for (int octave = 2; octave <= 6; ++octave)
        {
            for (const auto& note : chord.notes)
            {
                int noteClass = note.midiNumber % 12;
                int midiNote = noteClass + (octave * 12);
                if (midiNote >= 24 && midiNote <= 96) // Reasonable range
                    availableNotes.push_back(midiNote);
            }
        }

        // Find common tones if prioritizing them
        std::vector<int> commonTones;
        if (constraints.prioritizeCommonTones && !currentVoices.empty())
        {
            commonTones.reserve(currentVoices.size());
            for (const auto& voice : currentVoices)
            {
                if (!voice.notes.empty())
                {
                    int currentNote = voice.notes.back().midiNumber;
                    int currentNoteClass = currentNote % 12;

                    // Keep the current note if its pitch class is in the new chord
                    bool isCommon = std::any_of(chord.notes.begin(), chord.notes.end(),
                        [currentNoteClass](const Note& chordNote) {
                            return (chordNote.midiNumber % 12) == currentNoteClass;
                        });

                    if (isCommon)
                        commonTones.push_back(currentNote);
                }
            }
        }
        
        // Simple assignment algorithm: minimize total voice leading distance
        std::vector<int> currentNotes;
        for (const auto& voice : currentVoices)
        {
            if (!voice.notes.empty())
                currentNotes.push_back(voice.notes.back().midiNumber);
            else
                currentNotes.push_back(60);
        }
        
        // Assign notes to minimize movement
        for (size_t voiceIndex = 0; voiceIndex < currentVoices.size(); ++voiceIndex)
        {
            int bestNote = currentNotes[voiceIndex];
            float minDistance = 1000.0f;
            
            // First try common tones
            if (constraints.prioritizeCommonTones)
            {
                for (int commonTone : commonTones)
                {
                    if (isNoteInVoiceRange(commonTone, currentVoices[voiceIndex].type))
                    {
                        float distance = std::abs(commonTone - currentNotes[voiceIndex]);
                        if (distance < minDistance)
                        {
                            minDistance = distance;
                            bestNote = commonTone;
                        }
                    }
                }
            }
            
            // If no suitable common tone found, try all available notes
            if (minDistance > 0.5f) // No common tone was found
            {
                for (int note : availableNotes)
                {
                    if (isNoteInVoiceRange(note, currentVoices[voiceIndex].type))
                    {
                        float distance = std::abs(note - currentNotes[voiceIndex]);
                        if (distance < minDistance && distance <= constraints.maxVoiceLeap)
                        {
                            minDistance = distance;
                            bestNote = note;
                        }
                    }
                }
            }
            
            assignment[voiceIndex] = bestNote;
        }
        
        return assignment;
    }
    
    inline MelodicContour BasicVoiceLeadingEngine::analyzeMelodicMotion(const Voice& voice)
    {
        MelodicContour contour;
        
        if (voice.notes.size() < 2)
            return contour;
        
        // Calculate interval vector
        contour.intervalVector = voice.getMelodicIntervals();

        // Find direction changes
        contour.directionChanges.reserve(contour.intervalVector.size());
        for (size_t i = 1; i < contour.intervalVector.size(); ++i)
        {
            int prev = contour.intervalVector[i-1];
            int curr = contour.intervalVector[i];

            if ((prev > 0 && curr < 0) || (prev < 0 && curr > 0))
            {
                contour.directionChanges.push_back(static_cast<int>(i));
            }
        }

        // Calculate stepwise motion ratio
        int stepwiseCount = static_cast<int>(std::count_if(
            contour.intervalVector.begin(), contour.intervalVector.end(),
            [](int interval) { return std::abs(interval) <= 2; }));
        contour.stepwiseMotionRatio = static_cast<float>(stepwiseCount) / contour.intervalVector.size();

        // Calculate average leap size
        std::vector<int> leaps;
        for (int interval : contour.intervalVector)
        {
            if (std::abs(interval) > 2)
                leaps.push_back(std::abs(interval));
        }
        if (!leaps.empty())
        {
            contour.averageLeapSize = static_cast<float>(std::accumulate(leaps.begin(), leaps.end(), 0)) / leaps.size();
        }

        // Find highest and lowest notes
        auto minmaxIt = std::minmax_element(voice.notes.begin(), voice.notes.end(),
            [](const Note& a, const Note& b) { return a.midiNumber < b.midiNumber; });
        contour.lowestNote = minmaxIt.first->midiNumber;
        contour.highestNote = minmaxIt.second->midiNumber;
        contour.range = contour.highestNote - contour.lowestNote;
        
        // Enhanced analysis
        contour.melodicComplexity = calculateMelodicComplexity(voice);
        contour.leapRecoveryRatio = calculateLeapRecoveryRatio(voice);
        contour.peakIndices = findMelodicPeaksAndValleys(voice, true);
        contour.valleyIndices = findMelodicPeaksAndValleys(voice, false);
        contour.overallDirection = calculateOverallDirection(voice);
        
        // Find largest leap
        contour.largestLeap = 0;
        contour.largestLeapIndex = -1;
        for (size_t i = 0; i < contour.intervalVector.size(); ++i)
        {
            int absInterval = std::abs(contour.intervalVector[i]);
            if (absInterval > contour.largestLeap)
            {
                contour.largestLeap = absInterval;
                contour.largestLeapIndex = static_cast<int>(i);
            }
        }
        
        // Calculate rhythmic variety
        if (voice.notes.size() > 1)
        {
            std::vector<float> durations;
            durations.reserve(voice.notes.size());
            for (const auto& note : voice.notes)
            {
                durations.push_back(note.duration);
            }
            
            // Calculate coefficient of variation for rhythmic variety
            float meanDuration = std::accumulate(durations.begin(), durations.end(), 0.0f) / durations.size();
            float variance = 0.0f;
            for (float duration : durations)
            {
                variance += (duration - meanDuration) * (duration - meanDuration);
            }
            variance /= durations.size();
            float stdDev = std::sqrt(variance);
            
            contour.rhythmicVariety = (meanDuration > 0.0f) ? std::min(1.0f, stdDev / meanDuration) : 0.0f;
        }
        
        return contour;
    }
    
    inline float BasicVoiceLeadingEngine::calculateSmoothness(const VoicingResult& voicing)
    {
        if (voicing.voices.empty())
            return 0.0f;
        
        float melodicSmoothness = 0.0f;
        for (const auto& voice : voicing.voices)
        {
            melodicSmoothness += calculateMelodicSmoothness(voice);
        }
        melodicSmoothness /= voicing.voices.size();
        
        float harmonicSmoothness = calculateHarmonicSmoothness(voicing.voices);
        float ruleCompliance = calculateRuleCompliance(voicing);
        
        // Weighted combination
        return (melodicSmoothness * 0.4f) + (harmonicSmoothness * 0.4f) + (ruleCompliance * 0.2f);
    }
    
    inline bool BasicVoiceLeadingEngine::validateVoiceRanges(const std::vector<Voice>& voices)
    {
        for (const auto& voice : voices)
        {
            if (!voice.isValid())
                return false;
        }
        return true;
    }
    
    inline bool BasicVoiceLeadingEngine::detectVoiceCrossing(const std::vector<Voice>& voices, int chordIndex)
    {
        if (voices.size() < 2 || chordIndex < 0)
            return false;
        
        std::vector<int> noteHeights;
        noteHeights.reserve(voices.size());
        for (const auto& voice : voices)
        {
            if (chordIndex < static_cast<int>(voice.notes.size()))
                noteHeights.push_back(voice.notes[chordIndex].midiNumber);
            else
                return false; // Invalid chord index
        }
        
        // Check if voices are in proper order (soprano highest, bass lowest)
        for (size_t i = 1; i < noteHeights.size(); ++i)
        {
            if (noteHeights[i-1] < noteHeights[i])
                return true; // Voice crossing detected
        }
        
        return false;
    }
    
    inline std::vector<ParallelMotionAnalysis> BasicVoiceLeadingEngine::detectParallelMotion(
        const std::vector<Voice>& voices, int fromChordIndex, int toChordIndex)
    {
        std::vector<ParallelMotionAnalysis> analyses;
        
        if (voices.size() < 2 || fromChordIndex < 0 || toChordIndex < 0)
            return analyses;

        const size_t voiceCount = voices.size();
        analyses.reserve((voiceCount * (voiceCount - 1)) / 2);
        
        // Check all voice pairs
        for (size_t i = 0; i < voices.size(); ++i)
        {
            for (size_t j = i + 1; j < voices.size(); ++j)
            {
                const auto& voice1 = voices[i];
                const auto& voice2 = voices[j];
                
                if (fromChordIndex >= static_cast<int>(voice1.notes.size()) ||
                    toChordIndex >= static_cast<int>(voice1.notes.size()) ||
                    fromChordIndex >= static_cast<int>(voice2.notes.size()) ||
                    toChordIndex >= static_cast<int>(voice2.notes.size()))
                    continue;
                
                int v1From = voice1.notes[fromChordIndex].midiNumber;
                int v1To = voice1.notes[toChordIndex].midiNumber;
                int v2From = voice2.notes[fromChordIndex].midiNumber;
                int v2To = voice2.notes[toChordIndex].midiNumber;
                
                ParallelMotionAnalysis analysis;
                analysis.motionType = classifyMotion(v1From, v1To, v2From, v2To);
                analysis.interval1 = calculateHarmonicInterval(v1From, v2From);
                analysis.interval2 = calculateHarmonicInterval(v1To, v2To);
                analysis.voiceIndex1 = static_cast<int>(i);
                analysis.voiceIndex2 = static_cast<int>(j);
                
                // Check for parallel fifths, octaves, and unisons
                if (analysis.motionType == MotionType::Parallel)
                {
                    analysis.isParallelFifth = isParallelInterval(analysis.interval1, analysis.interval2, 7);
                    analysis.isParallelOctave = isParallelInterval(analysis.interval1, analysis.interval2, 0);
                    analysis.isParallelUnison = (analysis.interval1 == 0 && analysis.interval2 == 0);
                }
                
                analyses.push_back(analysis);
            }
        }
        
        return analyses;
    }
    
    inline bool BasicVoiceLeadingEngine::hasParallelFifths(const std::vector<Voice>& voices,
                                                  int fromChordIndex, int toChordIndex)
    {
        auto analyses = detectParallelMotion(voices, fromChordIndex, toChordIndex);
        return std::any_of(analyses.begin(), analyses.end(),
            [](const ParallelMotionAnalysis& a) { return a.isParallelFifth; });
    }

    inline bool BasicVoiceLeadingEngine::hasParallelOctaves(const std::vector<Voice>& voices,
                                                   int fromChordIndex, int toChordIndex)
    {
        auto analyses = detectParallelMotion(voices, fromChordIndex, toChordIndex);
        return std::any_of(analyses.begin(), analyses.end(),
            [](const ParallelMotionAnalysis& a) { return a.isParallelOctave; });
    }
    
    inline std::vector<int> BasicVoiceLeadingEngine::findCommonTones(const Chord& chord1, const Chord& chord2)
    {
        std::vector<int> commonTones;
        
        for (const auto& note1 : chord1.notes)
        {
            int noteClass1 = note1.midiNumber % 12;
            for (const auto& note2 : chord2.notes)
            {
                int noteClass2 = note2.midiNumber % 12;
                if (noteClass1 == noteClass2)
                {
                    commonTones.push_back(noteClass1);
                    break;
                }
            }
        }
        
        // Remove duplicates
        std::sort(commonTones.begin(), commonTones.end());
        commonTones.erase(std::unique(commonTones.begin(), commonTones.end()), commonTones.end());
        
        return commonTones;
    }
    
    inline VoiceMovement BasicVoiceLeadingEngine::calculateMovement(int fromNote, int toNote)
    {
        return VoiceMovement(fromNote, toNote);
    }
    
    inline float BasicVoiceLeadingEngine::calculateVoiceIndependence(const std::vector<Voice>& voices)
    {
        if (voices.size() < 2)
            return 1.0f;
        
        float rhythmicIndependence = calculateRhythmicIndependence(voices);
        float melodicIndependence = calculateMelodicIndependence(voices);
        float contourIndependence = calculateContourIndependence(voices);
        
        return (rhythmicIndependence + melodicIndependence + contourIndependence) / 3.0f;
    }
    
    //==============================================================================
    // Private Helper Methods
    
    inline Range BasicVoiceLeadingEngine::getDefaultVoiceRange(VoiceType voiceType)
    {
        switch (voiceType)
        {
            case VoiceType::Soprano: return Range(60, 84); // C4 to C6
            case VoiceType::Alto: return Range(53, 77);    // F3 to F5
            case VoiceType::Tenor: return Range(48, 72);   // C3 to C5
            case VoiceType::Bass: return Range(40, 64);    // E2 to E4
            default: return Range(36, 96);                 // Wide range for other types
        }
    }
    
    inline bool BasicVoiceLeadingEngine::isNoteInVoiceRange(int midiNote, VoiceType voiceType)
    {
        Range range = getDefaultVoiceRange(voiceType);
        return range.contains(midiNote);
    }
    
    inline MotionType BasicVoiceLeadingEngine::classifyMotion(int voice1From, int voice1To, int voice2From, int voice2To)
    {
        int motion1 = voice1To - voice1From;
        int motion2 = voice2To - voice2From;
        
        if (motion1 == 0 && motion2 == 0)
            return MotionType::Oblique; // Both static
        
        if (motion1 == 0 || motion2 == 0)
            return MotionType::Oblique; // One static
        
        if ((motion1 > 0 && motion2 > 0) || (motion1 < 0 && motion2 < 0))
        {
            // Same direction
            if (std::abs(motion1) == std::abs(motion2))
                return MotionType::Parallel;
            else
                return MotionType::Similar;
        }
        else
        {
            return MotionType::Contrary; // Opposite directions
        }
    }
    
    inline int BasicVoiceLeadingEngine::calculateHarmonicInterval(int note1, int note2)
    {
        return std::abs(note1 - note2) % 12;
    }
    
    inline bool BasicVoiceLeadingEngine::isParallelInterval(int interval1, int interval2, int targetInterval)
    {
        return (interval1 % 12) == targetInterval && (interval2 % 12) == targetInterval;
    }
    
    inline float BasicVoiceLeadingEngine::calculateMelodicSmoothness(const Voice& voice)
    {
        if (voice.notes.size() < 2)
            return 1.0f;
        
        auto intervals = voice.getMelodicIntervals();
        float totalDistance = 0.0f;
        int stepwiseCount = 0;
        
        for (int interval : intervals)
        {
            totalDistance += std::abs(interval);
            if (std::abs(interval) <= 2)
                stepwiseCount++;
        }
        
        float averageDistance = totalDistance / intervals.size();
        float stepwiseRatio = static_cast<float>(stepwiseCount) / intervals.size();
        
        // Prefer smaller intervals and more stepwise motion
        float distanceScore = std::max(0.0f, 1.0f - (averageDistance / 12.0f));
        return (distanceScore * 0.6f) + (stepwiseRatio * 0.4f);
    }
    
    inline float BasicVoiceLeadingEngine::calculateHarmonicSmoothness(const std::vector<Voice>& voices)
    {
        if (voices.size() < 2)
            return 1.0f;
        
        float totalMovement = 0.0f;
        int movementCount = 0;
        
        // Calculate total voice leading movement between adjacent chords
        size_t minNotes = voices[0].notes.size();
        for (const auto& voice : voices)
        {
            minNotes = std::min(minNotes, voice.notes.size());
        }
        
        for (size_t chordIndex = 1; chordIndex < minNotes; ++chordIndex)
        {
            for (const auto& voice : voices)
            {
                int movement = std::abs(voice.notes[chordIndex].midiNumber - 
                                      voice.notes[chordIndex-1].midiNumber);
                totalMovement += movement;
                movementCount++;
            }
        }
        
        if (movementCount == 0)
            return 1.0f;
        
        float averageMovement = totalMovement / movementCount;
        return std::max(0.0f, 1.0f - (averageMovement / 12.0f));
    }
    
    inline float BasicVoiceLeadingEngine::calculateRuleCompliance(const VoicingResult& voicing)
    {
        float score = 1.0f;
        
        if (voicing.hasVoiceCrossing)
            score -= 0.3f;
        
        if (voicing.hasParallelFifths)
            score -= 0.4f;
        
        if (voicing.hasParallelOctaves)
            score -= 0.3f;
        
        return std::max(0.0f, score);
    }
    
    inline float BasicVoiceLeadingEngine::calculateRhythmicIndependence(const std::vector<Voice>& voices)
    {
        // For now, assume all voices move together (homophonic texture)
        // In a more advanced implementation, this would analyze rhythmic patterns
        return 0.5f; // Moderate rhythmic independence
    }
    
    inline float BasicVoiceLeadingEngine::calculateMelodicIndependence(const std::vector<Voice>& voices)
    {
        if (voices.size() < 2)
            return 1.0f;
        
        float totalIndependence = 0.0f;
        int comparisons = 0;
        
        // Compare melodic contours between voice pairs
        for (size_t i = 0; i < voices.size(); ++i)
        {
            for (size_t j = i + 1; j < voices.size(); ++j)
            {
                auto contour1 = analyzeMelodicMotion(voices[i]);
                auto contour2 = analyzeMelodicMotion(voices[j]);
                
                // Calculate similarity (lower similarity = higher independence)
                float similarity = 0.0f;
                if (!contour1.intervalVector.empty() && !contour2.intervalVector.empty())
                {
                    size_t minSize = std::min(contour1.intervalVector.size(), contour2.intervalVector.size());
                    int matchingDirections = 0;
                    
                    for (size_t k = 0; k < minSize; ++k)
                    {
                        bool dir1 = contour1.intervalVector[k] > 0;
                        bool dir2 = contour2.intervalVector[k] > 0;
                        if (dir1 == dir2)
                            matchingDirections++;
                    }
                    
                    similarity = static_cast<float>(matchingDirections) / minSize;
                }
                
                totalIndependence += (1.0f - similarity);
                comparisons++;
            }
        }
        
        return comparisons > 0 ? (totalIndependence / comparisons) : 1.0f;
    }
    
    inline float BasicVoiceLeadingEngine::calculateContourIndependence(const std::vector<Voice>& voices)
    {
        if (voices.size() < 2)
            return 1.0f;
        
        // Analyze how different the overall contours are
        std::vector<MelodicContour> contours;
        for (const auto& voice : voices)
        {
            contours.push_back(analyzeMelodicMotion(voice));
        }
        
        float totalIndependence = 0.0f;
        int comparisons = 0;
        
        for (size_t i = 0; i < contours.size(); ++i)
        {
            for (size_t j = i + 1; j < contours.size(); ++j)
            {
                // Compare range and stepwise motion ratios
                float rangeDiff = std::abs(contours[i].range - contours[j].range) / 24.0f; // Normalize by 2 octaves
                float stepwiseDiff = std::abs(contours[i].stepwiseMotionRatio - contours[j].stepwiseMotionRatio);
                
                float independence = (rangeDiff + stepwiseDiff) / 2.0f;
                totalIndependence += std::min(1.0f, independence);
                comparisons++;
            }
        }
        
        return comparisons > 0 ? (totalIndependence / comparisons) : 1.0f;
    }
    
    inline std::vector<std::string> BasicVoiceLeadingEngine::validateVoiceLeadingRules(const VoicingResult& voicing,
                                                                               const VoicingConstraints& constraints)
    {
        std::vector<std::string> violations;
        
        if (voicing.hasVoiceCrossing && !constraints.allowVoiceCrossing)
        {
            violations.push_back("Voice crossing detected");
        }
        
        if (voicing.hasParallelFifths && constraints.strictParallelRules)
        {
            violations.push_back("Parallel fifths detected");
        }
        
        if (voicing.hasParallelOctaves && constraints.strictParallelRules)
        {
            violations.push_back("Parallel octaves detected");
        }
        
        // Check for large leaps
        for (const auto& movement : voicing.movements)
        {
            if (std::abs(movement.interval) > constraints.maxVoiceLeap)
            {
                violations.push_back(std::string("Large leap detected: ") + std::to_string(movement.interval) + std::string(" semitones"));
            }
        }
        
        // Check voice ranges
        if (constraints.enforceVoiceRanges && !validateVoiceRanges(voicing.voices))
        {
            violations.push_back("Voice range violation detected");
        }
        
        return violations;
    }
    
    //==============================================================================
    // Enhanced Melodic Contour Analysis Methods
    
    inline std::vector<int> BasicVoiceLeadingEngine::findMelodicPeaksAndValleys(const Voice& voice, bool findPeaks)
    {
        std::vector<int> indices;
        
        if (voice.notes.size() < 3)
            return indices;
        
        // Find local maxima (peaks) or minima (valleys)
        for (size_t i = 1; i < voice.notes.size() - 1; ++i)
        {
            int prevNote = voice.notes[i-1].midiNumber;
            int currNote = voice.notes[i].midiNumber;
            int nextNote = voice.notes[i+1].midiNumber;
            
            if (findPeaks)
            {
                // Peak: current note higher than both neighbors
                if (currNote > prevNote && currNote > nextNote)
                {
                    indices.push_back(static_cast<int>(i));
                }
            }
            else
            {
                // Valley: current note lower than both neighbors
                if (currNote < prevNote && currNote < nextNote)
                {
                    indices.push_back(static_cast<int>(i));
                }
            }
        }
        
        return indices;
    }
    
    inline float BasicVoiceLeadingEngine::calculateMelodicComplexity(const Voice& voice)
    {
        if (voice.notes.size() < 2)
            return 0.0f;
        
        auto intervals = voice.getMelodicIntervals();
        
        // Calculate interval variety (number of different interval sizes)
        std::set<int> uniqueIntervals;
        for (int interval : intervals)
        {
            uniqueIntervals.insert(std::abs(interval));
        }
        float intervalVariety = static_cast<float>(uniqueIntervals.size()) / 12.0f; // Normalize by chromatic scale
        
        // Calculate direction change frequency
        int directionChanges = 0;
        for (size_t i = 1; i < intervals.size(); ++i)
        {
            int prev = intervals[i-1];
            int curr = intervals[i];
            
            if ((prev > 0 && curr < 0) || (prev < 0 && curr > 0))
            {
                directionChanges++;
            }
        }
        float directionChangeRatio = static_cast<float>(directionChanges) / intervals.size();
        
        // Calculate leap frequency
        int leapCount = 0;
        for (int interval : intervals)
        {
            if (std::abs(interval) > 2)
                leapCount++;
        }
        float leapRatio = static_cast<float>(leapCount) / intervals.size();
        
        // Combine factors (weighted average)
        float complexity = (intervalVariety * 0.4f) + (directionChangeRatio * 0.3f) + (leapRatio * 0.3f);
        return std::min(1.0f, complexity);
    }
    
    inline float BasicVoiceLeadingEngine::calculateLeapRecoveryRatio(const Voice& voice)
    {
        if (voice.notes.size() < 3)
            return 0.0f;
        
        auto intervals = voice.getMelodicIntervals();
        int leapRecoveries = 0;
        int totalLeaps = 0;
        
        // Check each leap to see if it's followed by stepwise motion in opposite direction
        for (size_t i = 0; i < intervals.size() - 1; ++i)
        {
            int currentInterval = intervals[i];
            int nextInterval = intervals[i + 1];
            
            // If current interval is a leap (> 2 semitones)
            if (std::abs(currentInterval) > 2)
            {
                totalLeaps++;
                
                // Check if next interval is stepwise and in opposite direction
                if (std::abs(nextInterval) <= 2)
                {
                    // Opposite direction check
                    if ((currentInterval > 0 && nextInterval < 0) || 
                        (currentInterval < 0 && nextInterval > 0))
                    {
                        leapRecoveries++;
                    }
                }
            }
        }
        
        return (totalLeaps > 0) ? static_cast<float>(leapRecoveries) / totalLeaps : 0.0f;
    }
    
    inline float BasicVoiceLeadingEngine::calculateOverallDirection(const Voice& voice)
    {
        if (voice.notes.size() < 2)
            return 0.0f;
        
        // Calculate the overall pitch trajectory
        int firstNote = voice.notes.front().midiNumber;
        int lastNote = voice.notes.back().midiNumber;
        int totalRange = lastNote - firstNote;
        
        // Normalize to -1.0 (descending) to 1.0 (ascending)
        // Use a reasonable range of 2 octaves (24 semitones) for normalization
        float direction = static_cast<float>(totalRange) / 24.0f;
        return std::max(-1.0f, std::min(1.0f, direction));
    }
    
    inline bool BasicVoiceLeadingEngine::validateMelodicIntegrity(const Voice& voice, float maxComplexity)
    {
        if (voice.notes.size() < 2)
            return true; // Single note or empty voice is valid
        
        // Check melodic complexity
        float complexity = calculateMelodicComplexity(voice);
        if (complexity > maxComplexity)
            return false;
        
        // Check for excessive leaps without recovery
        float leapRecovery = calculateLeapRecoveryRatio(voice);
        auto intervals = voice.getMelodicIntervals();
        
        // Count large leaps (> 7 semitones)
        int largeLeaps = 0;
        for (int interval : intervals)
        {
            if (std::abs(interval) > 7)
                largeLeaps++;
        }
        
        // Fail if more than 25% large leaps or poor leap recovery
        float largeLeapRatio = static_cast<float>(largeLeaps) / intervals.size();
        if (largeLeapRatio > 0.25f && leapRecovery < 0.5f)
            return false;
        
        // Check for excessive range (more than 2.5 octaves)
        int range = 0;
        if (!voice.notes.empty())
        {
            int highest = voice.notes[0].midiNumber;
            int lowest = voice.notes[0].midiNumber;
            for (const auto& note : voice.notes)
            {
                highest = std::max(highest, note.midiNumber);
                lowest = std::min(lowest, note.midiNumber);
            }
            range = highest - lowest;
        }
        
        if (range > 30) // 2.5 octaves
            return false;
        
        // Check for too many direction changes (indicates erratic melody)
        std::vector<int> peaks = findMelodicPeaksAndValleys(voice, true);
        std::vector<int> valleys = findMelodicPeaksAndValleys(voice, false);
        int totalTurningPoints = peaks.size() + valleys.size();
        
        // Fail if more than 50% of notes are turning points
        if (voice.notes.size() > 4 && totalTurningPoints > voice.notes.size() / 2)
            return false;
        
        return true;
    }
    
} // namespace HarmonyEngine
