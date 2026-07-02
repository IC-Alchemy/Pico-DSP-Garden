/*
  ==============================================================================

    TensionAnalysisEngine.h
    Created: Tension analysis engine for harmonic tension calculation
    
    This file contains the TensionAnalysisEngine interface and implementation
    that provides beat-level tension calculation, harmonic dissonance measurement,
    phrase-level tension curves, and real-time tension updates.

  ==============================================================================
*/

#pragma once

// Implementation includes (header-only)
#include <algorithm>
#include <cmath>

#include "MusicTheory.h"
#include <vector>
#include <map>
#include <memory>


// ---- header-only helpers ----
#include <sstream>
#include <iomanip>
namespace HarmonyEngine {
    inline std::string fixed(float v, int prec) {
        std::ostringstream os; os << std::fixed << std::setprecision(prec) << v; return os.str();
    }
}

namespace HarmonyEngine
{
    //==============================================================================
    // Tension Analysis Data Structures
    
    struct TensionPeak
    {
        float timePosition;     // Time position in beats
        float tensionValue;     // Tension value (0.0 to 1.0)
        int chordIndex;         // Index of chord causing peak
        std::string description; // Description of tension source
        
        TensionPeak(float time = 0.0f, float tension = 0.0f, int index = -1)
            : timePosition(time), tensionValue(tension), chordIndex(index) {}
        
        bool operator>(const TensionPeak& other) const
        {
            return tensionValue > other.tensionValue;
        }
    };
    
    struct TensionCurve
    {
        std::vector<float> tensionValues;  // Tension values over time
        float timeResolution;              // Time resolution in beats
        std::vector<TensionPeak> peaks;    // Identified tension peaks
        float averageTension;              // Average tension across curve
        float maxTension;                  // Maximum tension value
        float minTension;                  // Minimum tension value
        
        TensionCurve(float resolution = 0.25f) 
            : timeResolution(resolution), averageTension(0.0f), 
              maxTension(0.0f), minTension(1.0f) {}
        
        bool isValid() const
        {
            return !tensionValues.empty() && timeResolution > 0.0f;
        }
        
        float getTensionAtTime(float time) const
        {
            if (tensionValues.empty()) return 0.0f;
            
            int index = static_cast<int>(time / timeResolution);
            if (index < 0) return tensionValues[0];
            if (index >= static_cast<int>(tensionValues.size())) 
                return tensionValues.back();
            
            return tensionValues[index];
        }
        
        void calculateStatistics()
        {
            if (tensionValues.empty()) return;
            
            float sum = 0.0f;
            maxTension = tensionValues[0];
            minTension = tensionValues[0];
            
            for (float value : tensionValues)
            {
                sum += value;
                maxTension = std::max(maxTension, value);
                minTension = std::min(minTension, value);
            }
            
            averageTension = sum / static_cast<float>(tensionValues.size());
        }
    };
    
    struct Phrase
    {
        std::vector<Chord> chords;
        float startTime;        // Start time in beats
        float duration;         // Duration in beats
        Key key;               // Key context for the phrase
        
        Phrase(float start = 0.0f, float dur = 4.0f) 
            : startTime(start), duration(dur) {}
        
        bool isValid() const
        {
            return !chords.empty() && duration > 0.0f && key.isValid();
        }
        
        float getEndTime() const
        {
            return startTime + duration;
        }
    };
    
    //==============================================================================
    // Interval Tension Values
    
    enum class IntervalType
    {
        Unison = 0,
        MinorSecond = 1,
        MajorSecond = 2,
        MinorThird = 3,
        MajorThird = 4,
        PerfectFourth = 5,
        Tritone = 6,
        PerfectFifth = 7,
        MinorSixth = 8,
        MajorSixth = 9,
        MinorSeventh = 10,
        MajorSeventh = 11,
        Octave = 12
    };
    
    struct IntervalTensionMap
    {
        std::map<IntervalType, float> tensionValues;
        
        IntervalTensionMap()
        {
            // Initialize interval tension values based on harmonic series and dissonance
            tensionValues[IntervalType::Unison] = 0.0f;        // Perfect consonance
            tensionValues[IntervalType::MinorSecond] = 0.95f;   // Maximum dissonance
            tensionValues[IntervalType::MajorSecond] = 0.7f;    // Strong dissonance
            tensionValues[IntervalType::MinorThird] = 0.2f;     // Mild consonance
            tensionValues[IntervalType::MajorThird] = 0.15f;    // Strong consonance
            tensionValues[IntervalType::PerfectFourth] = 0.3f;  // Mild dissonance
            tensionValues[IntervalType::Tritone] = 1.0f;        // Maximum tension
            tensionValues[IntervalType::PerfectFifth] = 0.05f;  // Strong consonance
            tensionValues[IntervalType::MinorSixth] = 0.4f;     // Mild dissonance
            tensionValues[IntervalType::MajorSixth] = 0.25f;    // Mild consonance
            tensionValues[IntervalType::MinorSeventh] = 0.8f;   // Strong dissonance
            tensionValues[IntervalType::MajorSeventh] = 0.9f;   // Very strong dissonance
            tensionValues[IntervalType::Octave] = 0.0f;         // Perfect consonance
        }
        
        float getTensionForInterval(int semitones) const
        {
            int normalizedInterval = semitones % 12;
            if (normalizedInterval < 0) normalizedInterval += 12;
            
            auto intervalType = static_cast<IntervalType>(normalizedInterval);
            auto it = tensionValues.find(intervalType);
            return (it != tensionValues.end()) ? it->second : 0.5f;
        }
    };
    
    //==============================================================================
    // TensionAnalysisEngine Interface
    
    class TensionAnalysisEngine
    {
    public:
        virtual ~TensionAnalysisEngine() = default;
        
        // Core tension calculation methods
        virtual TensionCurve calculateBeatTension(const std::vector<Chord>& progression, 
                                                 float beatsPerMeasure = 4.0f) = 0;
        virtual TensionCurve calculatePhraseTension(const std::vector<Phrase>& phrases) = 0;
        virtual std::vector<TensionPeak> identifyTensionPeaks(const TensionCurve& curve, 
                                                             float threshold = 0.7f) = 0;
        
        // Harmonic dissonance measurement
        virtual float calculateChordTension(const Chord& chord) = 0;
        virtual float calculateIntervalTension(int semitones) = 0;
        virtual float calculateVoiceLeadingTension(const Chord& from, const Chord& to) = 0;
        
        // Real-time updates
        virtual void updateTensionForChordChange(TensionCurve& curve, int chordIndex, 
                                               const Chord& newChord) = 0;
        virtual TensionCurve recalculateTensionCurve(const std::vector<Chord>& progression, 
                                                   const TensionCurve& previousCurve) = 0;
        
        // Functional harmony tension
        virtual float calculateFunctionalTension(ChordFunction function, 
                                               const Key& key) = 0;
        virtual float calculateProgressionTension(const Chord& from, const Chord& to, 
                                                const Key& key) = 0;
        
        // Analysis utilities
        virtual std::vector<std::string> analyzeTensionSources(const Chord& chord) = 0;
        virtual std::string describeTensionPeak(const TensionPeak& peak, 
                                               const std::vector<Chord>& progression) = 0;
    };
    
    //==============================================================================
    // Basic Implementation
    
    class BasicTensionAnalysisEngine : public TensionAnalysisEngine
    {
    public:
        BasicTensionAnalysisEngine();
        ~BasicTensionAnalysisEngine() override = default;
        
        // TensionAnalysisEngine interface implementation
        TensionCurve calculateBeatTension(const std::vector<Chord>& progression, 
                                        float beatsPerMeasure = 4.0f) override;
        TensionCurve calculatePhraseTension(const std::vector<Phrase>& phrases) override;
        std::vector<TensionPeak> identifyTensionPeaks(const TensionCurve& curve, 
                                                     float threshold = 0.7f) override;
        
        float calculateChordTension(const Chord& chord) override;
        float calculateIntervalTension(int semitones) override;
        float calculateVoiceLeadingTension(const Chord& from, const Chord& to) override;
        
        void updateTensionForChordChange(TensionCurve& curve, int chordIndex, 
                                       const Chord& newChord) override;
        TensionCurve recalculateTensionCurve(const std::vector<Chord>& progression, 
                                           const TensionCurve& previousCurve) override;
        
        float calculateFunctionalTension(ChordFunction function, const Key& key) override;
        float calculateProgressionTension(const Chord& from, const Chord& to, 
                                        const Key& key) override;
        
        std::vector<std::string> analyzeTensionSources(const Chord& chord) override;
        std::string describeTensionPeak(const TensionPeak& peak, 
                                       const std::vector<Chord>& progression) override;
        
    private:
        IntervalTensionMap intervalTensions;
        std::map<ChordFunction, float> functionalTensions;
        
        // Helper methods
        void initializeFunctionalTensions();
        float calculateRhythmicTension(float beatPosition, float beatsPerMeasure);
        
        // Peak detection helpers
        bool isLocalMaximum(const std::vector<float>& values, int index, int windowSize = 2);
        std::vector<int> findLocalMaxima(const std::vector<float>& values, float threshold);
        
        // Voice leading analysis
        std::vector<int> calculateVoiceMovements(const Chord& from, const Chord& to);
        float calculateMovementTension(const std::vector<int>& movements);
        
        // Utility methods
        float clampTension(float tension) const { return std::max(0.0f, std::min(1.0f, tension)); }
    };
    
} // namespace HarmonyEngine

// ============================================================================
// Implementation (merged from .cpp - header-only library)
// ============================================================================

namespace HarmonyEngine
{
    //==============================================================================
    // BasicTensionAnalysisEngine Implementation
    
    inline BasicTensionAnalysisEngine::BasicTensionAnalysisEngine()
    {
        initializeFunctionalTensions();
    }
    
    inline void BasicTensionAnalysisEngine::initializeFunctionalTensions()
    {
        // Initialize functional harmony tension values
        functionalTensions[ChordFunction::Tonic] = 0.1f;        // Low tension (resolution)
        functionalTensions[ChordFunction::Subdominant] = 0.4f;  // Moderate tension
        functionalTensions[ChordFunction::Dominant] = 0.9f;     // High tension (needs resolution)
        functionalTensions[ChordFunction::Predominant] = 0.6f;  // Moderate-high tension
        functionalTensions[ChordFunction::Mediant] = 0.3f;      // Low-moderate tension
        functionalTensions[ChordFunction::Submediant] = 0.35f;  // Low-moderate tension
        functionalTensions[ChordFunction::LeadingTone] = 0.85f; // High tension
        functionalTensions[ChordFunction::Secondary] = 0.8f;    // High tension (chromatic)
    }
    
    //==============================================================================
    // Core Tension Calculation Methods
    
    inline TensionCurve BasicTensionAnalysisEngine::calculateBeatTension(const std::vector<Chord>& progression, 
                                                                float beatsPerMeasure)
    {
        TensionCurve curve(0.25f); // Quarter note resolution
        
        if (progression.empty())
            return curve;
        
        // Calculate total duration assuming each chord lasts one beat
        float totalBeats = static_cast<float>(progression.size());
        int numSamples = static_cast<int>(totalBeats / curve.timeResolution);
        curve.tensionValues.resize(numSamples, 0.0f);
        
        // Calculate tension for each time point
        for (int i = 0; i < numSamples; ++i)
        {
            float currentTime = i * curve.timeResolution;
            int chordIndex = static_cast<int>(currentTime);
            
            if (chordIndex >= static_cast<int>(progression.size()))
                chordIndex = static_cast<int>(progression.size()) - 1;
            
            const Chord& currentChord = progression[chordIndex];
            
            // Calculate multiple tension components
            float harmonicTension = calculateChordTension(currentChord);
            float rhythmicTension = calculateRhythmicTension(currentTime, beatsPerMeasure);
            float functionalTension = calculateFunctionalTension(currentChord.function, Key());
            
            // Voice leading tension (if not first chord)
            float voiceLeadingTension = 0.0f;
            if (chordIndex > 0)
            {
                voiceLeadingTension = calculateVoiceLeadingTension(progression[chordIndex - 1], 
                                                                 currentChord);
            }
            
            // Combine tension components with weights
            float totalTension = (harmonicTension * 0.4f) + 
                               (rhythmicTension * 0.2f) + 
                               (functionalTension * 0.3f) + 
                               (voiceLeadingTension * 0.1f);
            
            curve.tensionValues[i] = clampTension(totalTension);
        }
        
        curve.calculateStatistics();
        curve.peaks = identifyTensionPeaks(curve);
        
        return curve;
    }
    
    inline TensionCurve BasicTensionAnalysisEngine::calculatePhraseTension(const std::vector<Phrase>& phrases)
    {
        TensionCurve curve(0.25f);
        
        if (phrases.empty())
            return curve;
        
        // Calculate total duration of all phrases
        float totalDuration = 0.0f;
        for (const auto& phrase : phrases)
        {
            totalDuration = std::max(totalDuration, phrase.getEndTime());
        }
        
        int numSamples = static_cast<int>(totalDuration / curve.timeResolution);
        curve.tensionValues.resize(numSamples, 0.0f);
        
        // Process each phrase
        for (const auto& phrase : phrases)
        {
            if (!phrase.isValid()) continue;
            
            // Calculate tension curve for this phrase
            TensionCurve phraseCurve = calculateBeatTension(phrase.chords, 4.0f);
            
            // Map phrase tension to global timeline
            int startSample = static_cast<int>(phrase.startTime / curve.timeResolution);
            int phraseSamples = static_cast<int>(phrase.duration / curve.timeResolution);
            
            for (int i = 0; i < phraseSamples && (startSample + i) < numSamples; ++i)
            {
                if (i < static_cast<int>(phraseCurve.tensionValues.size()))
                {
                    // Combine tensions (take maximum for overlapping phrases)
                    curve.tensionValues[startSample + i] = std::max(
                        curve.tensionValues[startSample + i],
                        phraseCurve.tensionValues[i]
                    );
                }
            }
        }
        
        curve.calculateStatistics();
        curve.peaks = identifyTensionPeaks(curve);
        
        return curve;
    }
    
    inline std::vector<TensionPeak> BasicTensionAnalysisEngine::identifyTensionPeaks(const TensionCurve& curve, 
                                                                            float threshold)
    {
        std::vector<TensionPeak> peaks;
        
        if (curve.tensionValues.empty())
            return peaks;
        
        // Find local maxima above threshold
        std::vector<int> peakIndices = findLocalMaxima(curve.tensionValues, threshold);
        peaks.reserve(peakIndices.size());
        
        for (int index : peakIndices)
        {
            TensionPeak peak;
            peak.timePosition = index * curve.timeResolution;
            peak.tensionValue = curve.tensionValues[index];
            peak.chordIndex = static_cast<int>(peak.timePosition); // Assuming 1 beat per chord
            peak.description = "Tension peak at " + fixed(peak.timePosition, 2) + " beats";
            
            peaks.push_back(peak);
        }
        
        // Sort peaks by tension value (highest first)
        std::sort(peaks.begin(), peaks.end(), std::greater<TensionPeak>());
        
        return peaks;
    }
    
    //==============================================================================
    // Harmonic Dissonance Measurement
    
    inline float BasicTensionAnalysisEngine::calculateChordTension(const Chord& chord)
    {
        if (chord.notes.size() < 2)
            return 0.0f;
        
        float totalTension = 0.0f;
        int intervalCount = 0;
        
        // Calculate tension from all intervals in the chord
        for (size_t i = 0; i < chord.notes.size(); ++i)
        {
            for (size_t j = i + 1; j < chord.notes.size(); ++j)
            {
                int interval = std::abs(chord.notes[j].midiNumber - chord.notes[i].midiNumber);
                totalTension += calculateIntervalTension(interval);
                intervalCount++;
            }
        }
        
        // Average the interval tensions
        float averageIntervalTension = (intervalCount > 0) ? totalTension / intervalCount : 0.0f;
        
        // Add chord type specific tension
        float chordTypeTension = 0.0f;
        switch (chord.type)
        {
            case ChordType::Major:
            case ChordType::Minor:
                chordTypeTension = 0.1f;
                break;
            case ChordType::Diminished:
            case ChordType::Diminished7:
                chordTypeTension = 0.8f;
                break;
            case ChordType::Augmented:
                chordTypeTension = 0.7f;
                break;
            case ChordType::Dominant7:
                chordTypeTension = 0.6f;
                break;
            case ChordType::Major7:
            case ChordType::Minor7:
                chordTypeTension = 0.3f;
                break;
            case ChordType::HalfDiminished7:
                chordTypeTension = 0.7f;
                break;
            default:
                chordTypeTension = 0.2f;
                break;
        }
        
        // Combine interval and chord type tensions
        float combinedTension = (averageIntervalTension * 0.7f) + (chordTypeTension * 0.3f);
        
        return clampTension(combinedTension);
    }
    
    inline float BasicTensionAnalysisEngine::calculateIntervalTension(int semitones)
    {
        return intervalTensions.getTensionForInterval(semitones);
    }
    
    inline float BasicTensionAnalysisEngine::calculateVoiceLeadingTension(const Chord& from, const Chord& to)
    {
        if (from.notes.empty() || to.notes.empty())
            return 0.0f;
        
        std::vector<int> movements = calculateVoiceMovements(from, to);
        return calculateMovementTension(movements);
    }
    
    //==============================================================================
    // Real-time Updates
    
    inline void BasicTensionAnalysisEngine::updateTensionForChordChange(TensionCurve& curve, 
                                                               int chordIndex, 
                                                               const Chord& newChord)
    {
        if (curve.tensionValues.empty() || chordIndex < 0)
            return;
        
        // Calculate the time range affected by this chord
        float chordStartTime = static_cast<float>(chordIndex);
        float chordEndTime = chordStartTime + 1.0f; // Assuming 1 beat per chord
        
        int startSample = static_cast<int>(chordStartTime / curve.timeResolution);
        int endSample = static_cast<int>(chordEndTime / curve.timeResolution);
        
        // Update tension values for the affected time range
        for (int i = startSample; i < endSample && i < static_cast<int>(curve.tensionValues.size()); ++i)
        {
            float currentTime = i * curve.timeResolution;
            
            // Recalculate tension components for this time point
            float harmonicTension = calculateChordTension(newChord);
            float rhythmicTension = calculateRhythmicTension(currentTime, 4.0f);
            float functionalTension = calculateFunctionalTension(newChord.function, Key());
            
            // Combine tensions
            float totalTension = (harmonicTension * 0.4f) + 
                               (rhythmicTension * 0.2f) + 
                               (functionalTension * 0.4f);
            
            curve.tensionValues[i] = clampTension(totalTension);
        }
        
        // Recalculate statistics and peaks
        curve.calculateStatistics();
        curve.peaks = identifyTensionPeaks(curve);
    }
    
    inline TensionCurve BasicTensionAnalysisEngine::recalculateTensionCurve(const std::vector<Chord>& progression, 
                                                                   const TensionCurve& previousCurve)
    {
        // For now, just recalculate the entire curve
        // In a more sophisticated implementation, we could optimize by only
        // recalculating changed sections
        return calculateBeatTension(progression, 4.0f);
    }
    
    //==============================================================================
    // Functional Harmony Tension
    
    inline float BasicTensionAnalysisEngine::calculateFunctionalTension(ChordFunction function, const Key& key)
    {
        auto it = functionalTensions.find(function);
        return (it != functionalTensions.end()) ? it->second : 0.5f;
    }
    
    inline float BasicTensionAnalysisEngine::calculateProgressionTension(const Chord& from, 
                                                                const Chord& to, 
                                                                const Key& key)
    {
        // Calculate tension based on functional progression
        float functionalTension = 0.0f;
        
        // Strong progressions (V-I, IV-I) create resolution (lower tension)
        if ((from.function == ChordFunction::Dominant && to.function == ChordFunction::Tonic) ||
            (from.function == ChordFunction::Subdominant && to.function == ChordFunction::Tonic))
        {
            functionalTension = 0.2f; // Resolution reduces tension
        }
        // Weak progressions (I-IV, I-V) create tension
        else if (from.function == ChordFunction::Tonic && 
                (to.function == ChordFunction::Subdominant || to.function == ChordFunction::Dominant))
        {
            functionalTension = 0.7f; // Moving away from tonic increases tension
        }
        else
        {
            functionalTension = 0.5f; // Neutral progression
        }
        
        // Add voice leading tension
        float voiceLeadingTension = calculateVoiceLeadingTension(from, to);
        
        // Combine tensions
        return clampTension((functionalTension * 0.6f) + (voiceLeadingTension * 0.4f));
    }
    
    //==============================================================================
    // Analysis Utilities
    
    inline std::vector<std::string> BasicTensionAnalysisEngine::analyzeTensionSources(const Chord& chord)
    {
        std::vector<std::string> sources;
        
        if (chord.notes.size() < 2)
        {
            sources.push_back("Insufficient notes for tension analysis");
            return sources;
        }
        
        // Analyze intervals for dissonance
        for (size_t i = 0; i < chord.notes.size(); ++i)
        {
            for (size_t j = i + 1; j < chord.notes.size(); ++j)
            {
                int interval = std::abs(chord.notes[j].midiNumber - chord.notes[i].midiNumber) % 12;
                float tension = calculateIntervalTension(interval);
                
                if (tension > 0.6f)
                {
                    std::string intervalName;
                    switch (interval)
                    {
                        case 1: intervalName = "minor second"; break;
                        case 2: intervalName = "major second"; break;
                        case 6: intervalName = "tritone"; break;
                        case 10: intervalName = "minor seventh"; break;
                        case 11: intervalName = "major seventh"; break;
                        default: intervalName = "dissonant interval"; break;
                    }
                    
                    sources.push_back("Dissonant " + intervalName + " (tension: " + 
                                    fixed(tension, 2) + ")");
                }
            }
        }
        
        // Analyze chord type
        switch (chord.type)
        {
            case ChordType::Diminished:
            case ChordType::Diminished7:
                sources.push_back("Diminished chord quality creates instability");
                break;
            case ChordType::Augmented:
                sources.push_back("Augmented chord quality creates tension");
                break;
            case ChordType::Dominant7:
                sources.push_back("Dominant seventh requires resolution");
                break;
            case ChordType::HalfDiminished7:
                sources.push_back("Half-diminished seventh creates mild tension");
                break;
            default:
                break;
        }
        
        // Analyze functional tension
        switch (chord.function)
        {
            case ChordFunction::Dominant:
                sources.push_back("Dominant function creates strong pull to tonic");
                break;
            case ChordFunction::LeadingTone:
                sources.push_back("Leading tone function creates strong tension");
                break;
            case ChordFunction::Secondary:
                sources.push_back("Secondary function adds chromatic tension");
                break;
            default:
                break;
        }
        
        if (sources.empty())
        {
            sources.push_back("Low tension - stable consonant harmony");
        }
        
        return sources;
    }
    
    inline std::string BasicTensionAnalysisEngine::describeTensionPeak(const TensionPeak& peak, 
                                                               const std::vector<Chord>& progression)
    {
        std::string description = "Tension peak (";
        description += fixed(peak.tensionValue, 2) + ") at ";
        description += fixed(peak.timePosition, 2) + " beats";
        
        if (peak.chordIndex >= 0 && peak.chordIndex < static_cast<int>(progression.size()))
        {
            const Chord& chord = progression[peak.chordIndex];
            description += " - " + chord.getChordSymbol();
            
            // Add primary tension source
            std::vector<std::string> sources = analyzeTensionSources(chord);
            if (!sources.empty())
            {
                description += " (" + sources[0] + ")";
            }
        }
        
        return description;
    }
    
    //==============================================================================
    // Helper Methods

    inline float BasicTensionAnalysisEngine::calculateRhythmicTension(float beatPosition, float beatsPerMeasure)
    {
        // Calculate position within measure
        float measurePosition = std::fmod(beatPosition, beatsPerMeasure);
        
        // Strong beats (1, 3 in 4/4) have lower tension
        // Weak beats (2, 4) and off-beats have higher tension
        if (std::abs(measurePosition - 0.0f) < 0.1f || 
            std::abs(measurePosition - 2.0f) < 0.1f) // Beats 1 and 3
        {
            return 0.2f; // Low tension on strong beats
        }
        else if (std::abs(measurePosition - 1.0f) < 0.1f || 
                 std::abs(measurePosition - 3.0f) < 0.1f) // Beats 2 and 4
        {
            return 0.4f; // Moderate tension on weak beats
        }
        else
        {
            return 0.6f; // Higher tension on off-beats
        }
    }

    inline bool BasicTensionAnalysisEngine::isLocalMaximum(const std::vector<float>& values, 
                                                   int index, 
                                                   int windowSize)
    {
        if (index < windowSize || index >= static_cast<int>(values.size()) - windowSize)
            return false;
        
        float centerValue = values[index];

        // Check if center value is greater than all values in window
        for (int i = index - windowSize; i <= index + windowSize; ++i)
        {
            if (i != index && values[i] >= centerValue)
                return false;
        }

        return true;
    }
    
    inline std::vector<int> BasicTensionAnalysisEngine::findLocalMaxima(const std::vector<float>& values, 
                                                               float threshold)
    {
        std::vector<int> maxima;
        
        for (int i = 1; i < static_cast<int>(values.size()) - 1; ++i)
        {
            if (values[i] > threshold && isLocalMaximum(values, i, 1))
            {
                maxima.push_back(i);
            }
        }
        
        return maxima;
    }
    
    inline std::vector<int> BasicTensionAnalysisEngine::calculateVoiceMovements(const Chord& from, 
                                                                        const Chord& to)
    {
        std::vector<int> movements;
        movements.reserve(from.notes.size());

        // Simple voice leading: match closest notes
        for (const auto& fromNote : from.notes)
        {
            int closestDistance = 127; // Maximum MIDI range
            
            for (const auto& toNote : to.notes)
            {
                int distance = std::abs(toNote.midiNumber - fromNote.midiNumber);
                if (distance < closestDistance)
                {
                    closestDistance = distance;
                }
            }
            
            movements.push_back(closestDistance);
        }
        
        return movements;
    }
    
    inline float BasicTensionAnalysisEngine::calculateMovementTension(const std::vector<int>& movements)
    {
        if (movements.empty())
            return 0.0f;
        
        float totalTension = 0.0f;
        
        for (int movement : movements)
        {
            // Larger movements create more tension
            // Stepwise motion (1-2 semitones) = low tension
            // Leaps (3+ semitones) = higher tension
            if (movement == 0)
                totalTension += 0.0f; // Common tone
            else if (movement <= 2)
                totalTension += 0.1f; // Stepwise motion
            else if (movement <= 4)
                totalTension += 0.3f; // Small leap
            else if (movement <= 7)
                totalTension += 0.6f; // Large leap
            else
                totalTension += 0.9f; // Very large leap
        }
        
        return clampTension(totalTension / movements.size());
    }

} // namespace HarmonyEngine
