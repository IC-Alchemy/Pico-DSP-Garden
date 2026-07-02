/*
  ==============================================================================

    ChordAnalyzer.h
    Created: Chord recognition and analysis functionality
    
    This file contains the ChordAnalyzer class that provides chord recognition,
    analysis, and basic harmonic function identification.

  ==============================================================================
*/

#pragma once

#include "MusicTheory.h"
#include <algorithm>
#include <set>

namespace HarmonyEngine
{
    //==============================================================================
    struct ChordAnalysisResult
    {
        Chord identifiedChord;
        float confidence;
        std::vector<ChordType> alternativeInterpretations;
        std::string analysisNotes;
        
        ChordAnalysisResult() : confidence(0.0f) {}
        
        bool isValid() const
        {
            return confidence > 0.0f && identifiedChord.isValid();
        }
    };
    
    //==============================================================================
    class ChordAnalyzer
    {
    public:
        ChordAnalyzer() = default;
        ~ChordAnalyzer() = default;
        
        // Main analysis functions
        ChordAnalysisResult analyzeChord(const std::vector<int>& midiNotes);
        ChordAnalysisResult analyzeChord(const Chord& chord);
        
        // Chord recognition from MIDI input
        ChordType recognizeChordType(const std::vector<int>& intervals);
        ChordFunction identifyHarmonicFunction(const Chord& chord, const Key& key);
        
        // Validation functions
        bool isValidChordProgression(const std::vector<Chord>& progression, const Key& key);
        std::vector<std::string> validateChord(const Chord& chord);
        
        // Utility functions
        std::vector<int> normalizeToChromatic(const std::vector<int>& midiNotes);
        std::vector<int> getChordIntervals(const std::vector<int>& chromaticNotes);
        int findRoot(const std::vector<int>& chromaticNotes, ChordType type);
        
    private:
        // Chord pattern matching
        struct ChordPattern
        {
            ChordType type;
            std::vector<int> intervals;
            float weight;
            
            ChordPattern(ChordType t, std::vector<int> i, float w = 1.0f)
                : type(t), intervals(i), weight(w) {}
        };
        
        std::vector<ChordPattern> getChordPatterns();
        float calculatePatternMatch(const std::vector<int>& intervals, const ChordPattern& pattern);
        
        // Harmonic function analysis
        std::map<int, ChordFunction> getFunctionMap(const Key& key);
        ChordFunction getScaleDegreeFunction(int scaleDegree, const Key& key);
        
        // Validation helpers
        bool hasParallelFifths(const Chord& chord1, const Chord& chord2);
        bool hasParallelOctaves(const Chord& chord1, const Chord& chord2);
        bool isValidVoiceLeading(const Chord& chord1, const Chord& chord2);
    };
    
    //==============================================================================
    // Implementation
    
    inline ChordAnalysisResult ChordAnalyzer::analyzeChord(const std::vector<int>& midiNotes)
    {
        ChordAnalysisResult result;
        
        if (midiNotes.size() < 2)
        {
            result.analysisNotes = "Insufficient notes for chord analysis";
            return result;
        }
        
        // Normalize to chromatic pitch classes
        auto chromaticNotes = normalizeToChromatic(midiNotes);
        
        // Get intervals from bass note
        auto intervals = getChordIntervals(chromaticNotes);
        
        // Recognize chord type
        ChordType recognizedType = recognizeChordType(intervals);
        
        // Find root note
        int root = findRoot(chromaticNotes, recognizedType);
        
        // Create chord object
        Chord chord(recognizedType, root);
        chord.notes.reserve(midiNotes.size());
        for (int midiNote : midiNotes)
            chord.notes.emplace_back(midiNote);
        
        result.identifiedChord = chord;
        result.confidence = 0.8f; // Basic confidence for now
        result.analysisNotes = "Recognized as " + chord.getChordSymbol();
        
        return result;
    }
    
    inline ChordAnalysisResult ChordAnalyzer::analyzeChord(const Chord& chord)
    {
        std::vector<int> midiNotes;
        midiNotes.reserve(chord.notes.size());
        for (const auto& note : chord.notes)
            midiNotes.push_back(note.midiNumber);
        return analyzeChord(midiNotes);
    }
    
    inline ChordType ChordAnalyzer::recognizeChordType(const std::vector<int>& intervals)
    {
        auto patterns = getChordPatterns();
        float bestMatch = 0.0f;
        ChordType bestType = ChordType::Major;
        
        for (const auto& pattern : patterns)
        {
            float match = calculatePatternMatch(intervals, pattern);
            if (match > bestMatch)
            {
                bestMatch = match;
                bestType = pattern.type;
            }
        }
        
        return bestType;
    }
    
    inline ChordFunction ChordAnalyzer::identifyHarmonicFunction(const Chord& chord, const Key& key)
    {
        auto functionMap = getFunctionMap(key);
        int chordRoot = chord.rootNote % 12;
        int keyTonic = key.tonicNote % 12;
        
        // Calculate scale degree
        int scaleDegree = (chordRoot - keyTonic + 12) % 12;
        
        return getScaleDegreeFunction(scaleDegree, key);
    }
    
    inline bool ChordAnalyzer::isValidChordProgression(const std::vector<Chord>& progression, const Key& key)
    {
        if (progression.size() < 2)
            return true;
        
        for (size_t i = 1; i < progression.size(); ++i)
        {
            if (!isValidVoiceLeading(progression[i-1], progression[i]))
                return false;
        }
        
        return true;
    }
    
    inline std::vector<std::string> ChordAnalyzer::validateChord(const Chord& chord)
    {
        std::vector<std::string> issues;
        
        if (!chord.isValid())
        {
            issues.push_back("Invalid chord structure");
        }
        
        if (chord.notes.size() < 3)
        {
            issues.push_back("Chord should have at least 3 notes for complete harmony");
        }
        
        if (chord.notes.size() > 6)
        {
            issues.push_back("Chord may be too dense for clear harmonic perception");
        }
        
        return issues;
    }
    
    inline std::vector<int> ChordAnalyzer::normalizeToChromatic(const std::vector<int>& midiNotes)
    {
        std::set<int> uniqueNotes;
        for (int note : midiNotes)
        {
            uniqueNotes.insert(note % 12);
        }
        
        std::vector<int> result(uniqueNotes.begin(), uniqueNotes.end());
        std::sort(result.begin(), result.end());
        return result;
    }
    
    inline std::vector<int> ChordAnalyzer::getChordIntervals(const std::vector<int>& chromaticNotes)
    {
        std::vector<int> intervals;
        if (chromaticNotes.empty()) return intervals;
        
        int bass = chromaticNotes[0];
        for (size_t i = 1; i < chromaticNotes.size(); ++i)
        {
            intervals.push_back((chromaticNotes[i] - bass + 12) % 12);
        }
        
        return intervals;
    }
    
    inline int ChordAnalyzer::findRoot(const std::vector<int>& chromaticNotes, ChordType type)
    {
        if (chromaticNotes.empty()) return 0;
        
        // For now, assume bass note is root (can be enhanced later for inversions)
        return chromaticNotes[0];
    }
    
    inline std::vector<ChordAnalyzer::ChordPattern> ChordAnalyzer::getChordPatterns()
    {
        return {
            ChordPattern(ChordType::Major, {4, 7}, 1.0f),
            ChordPattern(ChordType::Minor, {3, 7}, 1.0f),
            ChordPattern(ChordType::Diminished, {3, 6}, 1.0f),
            ChordPattern(ChordType::Augmented, {4, 8}, 1.0f),
            ChordPattern(ChordType::Major7, {4, 7, 11}, 1.0f),
            ChordPattern(ChordType::Minor7, {3, 7, 10}, 1.0f),
            ChordPattern(ChordType::Dominant7, {4, 7, 10}, 1.0f),
            ChordPattern(ChordType::Diminished7, {3, 6, 9}, 1.0f),
            ChordPattern(ChordType::HalfDiminished7, {3, 6, 10}, 1.0f),
            ChordPattern(ChordType::Sus2, {2, 7}, 0.8f),
            ChordPattern(ChordType::Sus4, {5, 7}, 0.8f)
        };
    }
    
    inline float ChordAnalyzer::calculatePatternMatch(const std::vector<int>& intervals, const ChordPattern& pattern)
    {
        if (intervals.size() != pattern.intervals.size())
            return 0.0f;

        size_t matches = 0;
        for (size_t i = 0; i < intervals.size(); ++i)
        {
            if (intervals[i] == pattern.intervals[i])
                ++matches;
        }

        return static_cast<float>(matches) / static_cast<float>(pattern.intervals.size()) * pattern.weight;
    }
    
    inline std::map<int, ChordFunction> ChordAnalyzer::getFunctionMap(const Key& key)
    {
        std::map<int, ChordFunction> functionMap;
        
        // Major key functions
        if (key.scaleType == ScaleType::Major)
        {
            functionMap[0] = ChordFunction::Tonic;      // I
            functionMap[2] = ChordFunction::Predominant; // ii
            functionMap[4] = ChordFunction::Mediant;     // iii
            functionMap[5] = ChordFunction::Subdominant; // IV
            functionMap[7] = ChordFunction::Dominant;    // V
            functionMap[9] = ChordFunction::Submediant;  // vi
            functionMap[11] = ChordFunction::LeadingTone; // vii°
        }
        // Minor key functions
        else if (key.scaleType == ScaleType::NaturalMinor || key.scaleType == ScaleType::HarmonicMinor)
        {
            functionMap[0] = ChordFunction::Tonic;      // i
            functionMap[2] = ChordFunction::Predominant; // ii°
            functionMap[3] = ChordFunction::Mediant;     // III
            functionMap[5] = ChordFunction::Subdominant; // iv
            functionMap[7] = ChordFunction::Dominant;    // v/V
            functionMap[8] = ChordFunction::Submediant;  // VI
            functionMap[10] = ChordFunction::LeadingTone; // VII
        }
        
        return functionMap;
    }
    
    inline ChordFunction ChordAnalyzer::getScaleDegreeFunction(int scaleDegree, const Key& key)
    {
        auto functionMap = getFunctionMap(key);
        auto it = functionMap.find(scaleDegree);
        return (it != functionMap.end()) ? it->second : ChordFunction::Tonic;
    }
    
    inline bool ChordAnalyzer::hasParallelFifths(const Chord& chord1, const Chord& chord2)
    {
        // Simplified check - would need more sophisticated voice tracking
        return false; // Placeholder implementation
    }
    
    inline bool ChordAnalyzer::hasParallelOctaves(const Chord& chord1, const Chord& chord2)
    {
        // Simplified check - would need more sophisticated voice tracking
        return false; // Placeholder implementation
    }
    
    inline bool ChordAnalyzer::isValidVoiceLeading(const Chord& chord1, const Chord& chord2)
    {
        // Basic validation - no parallel fifths or octaves
        return !hasParallelFifths(chord1, chord2) && !hasParallelOctaves(chord1, chord2);
    }
    
} // namespace HarmonyEngine