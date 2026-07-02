/*
  ==============================================================================

    MusicTheoryEngine.h
    Created: Basic functional harmony engine interface and implementation
    
    This file contains the MusicTheoryEngine interface and BasicMusicTheoryEngine
    implementation that provides diatonic chord generation, Roman numeral analysis,
    and chord progression validation.

  ==============================================================================
*/

#pragma once

// Implementation includes (header-only)
#include <algorithm>
#include <cmath>

#include "MusicTheory.h"
#include "ChordAnalyzer.h"
#include <map>
#include <memory>

namespace HarmonyEngine
{
    //==============================================================================
    // Roman Numeral Analysis Structures
    
    enum class RomanNumeralType
    {
        I, II, III, IV, V, VI, VII,
        i, ii, iii, iv, v, vi, vii
    };
    
    enum class RomanNumeralQuality
    {
        Major,
        Minor,
        Diminished,
        Augmented,
        Dominant7,
        Major7,
        Minor7,
        HalfDiminished7,
        Diminished7
    };
    
    struct RomanNumeralAnalysis
    {
        RomanNumeralType numeral;
        RomanNumeralQuality quality;
        ChordFunction function;
        int scaleDegree; // 1-7
        std::string symbol; // "I", "ii7", "V7/vi", etc.
        bool isSecondary; // For secondary dominants
        int secondaryTarget; // Scale degree of secondary target
        
        RomanNumeralAnalysis() 
            : numeral(RomanNumeralType::I), quality(RomanNumeralQuality::Major),
              function(ChordFunction::Tonic), scaleDegree(1), 
              isSecondary(false), secondaryTarget(0) {}
        
        std::string toString() const { return symbol; }
    };
    
    //==============================================================================
    // Diatonic Chord Generation Results
    
    struct DiatonicChordSet
    {
        Key key;
        std::vector<Chord> chords; // 7 diatonic chords
        std::vector<RomanNumeralAnalysis> analysis; // Roman numeral for each chord
        
        DiatonicChordSet(const Key& k) : key(k) {}
        
        bool isValid() const
        {
            return chords.size() == 7 && analysis.size() == 7 && key.isValid();
        }
        
        Chord getChordByDegree(int degree) const
        {
            if (degree >= 1 && degree <= 7)
                return chords[degree - 1];
            return Chord();
        }
        
        RomanNumeralAnalysis getAnalysisByDegree(int degree) const
        {
            if (degree >= 1 && degree <= 7)
                return analysis[degree - 1];
            return RomanNumeralAnalysis();
        }
    };
    
    //==============================================================================
    // Chord Progression Validation Results
    
    struct ProgressionValidationResult
    {
        bool isValid;
        float harmonyScore; // 0.0 to 1.0
        std::vector<std::string> issues;
        std::vector<std::string> suggestions;
        std::vector<RomanNumeralAnalysis> analysis;
        
        ProgressionValidationResult() : isValid(false), harmonyScore(0.0f) {}
    };
    
    //==============================================================================
    // MusicTheoryEngine Interface
    
    class MusicTheoryEngine
    {
    public:
        virtual ~MusicTheoryEngine() = default;
        
        // Core functionality
        virtual DiatonicChordSet generateDiatonicChords(const Key& key) = 0;
        virtual RomanNumeralAnalysis analyzeChordInKey(const Chord& chord, const Key& key) = 0;
        virtual ProgressionValidationResult validateProgression(const std::vector<Chord>& progression, const Key& key) = 0;
        
        // Harmonic function analysis
        virtual ChordFunction identifyFunction(const Chord& chord, const Key& key) = 0;
        virtual std::vector<Chord> getChordsWithFunction(ChordFunction function, const Key& key) = 0;
        
        // Progression utilities
        virtual std::vector<Chord> suggestNextChords(const std::vector<Chord>& progression, const Key& key) = 0;
        virtual bool isStrongProgression(const Chord& from, const Chord& to, const Key& key) = 0;
        
        // Scale and mode support
        virtual std::vector<ScaleType> getSupportedScales() = 0;
        virtual bool supportsScale(ScaleType scale) = 0;
    };
    
    //==============================================================================
    // Basic Implementation
    
    class BasicMusicTheoryEngine : public MusicTheoryEngine
    {
    public:
        BasicMusicTheoryEngine();
        ~BasicMusicTheoryEngine() override = default;
        
        // MusicTheoryEngine interface implementation
        DiatonicChordSet generateDiatonicChords(const Key& key) override;
        RomanNumeralAnalysis analyzeChordInKey(const Chord& chord, const Key& key) override;
        ProgressionValidationResult validateProgression(const std::vector<Chord>& progression, const Key& key) override;
        
        ChordFunction identifyFunction(const Chord& chord, const Key& key) override;
        std::vector<Chord> getChordsWithFunction(ChordFunction function, const Key& key) override;
        
        std::vector<Chord> suggestNextChords(const std::vector<Chord>& progression, const Key& key) override;
        bool isStrongProgression(const Chord& from, const Chord& to, const Key& key) override;
        
        std::vector<ScaleType> getSupportedScales() override;
        bool supportsScale(ScaleType scale) override;
        
    private:
        ChordAnalyzer analyzer;
        
        // Helper methods for chord generation
        Chord buildTriadOnDegree(int degree, const Key& key);
        ChordType getTriadTypeForDegree(int degree, const Key& key);
        ChordType getSeventhTypeForDegree(int degree, const Key& key);
        
        // Roman numeral analysis helpers
        RomanNumeralType getRomanNumeralForDegree(int degree, const Key& key);
        RomanNumeralQuality getQualityForChordType(ChordType type);
        std::string formatRomanNumeral(const RomanNumeralAnalysis& analysis);
        
        // Progression validation helpers
        float calculateHarmonyScore(const std::vector<Chord>& progression, const Key& key);
        std::vector<std::string> findProgressionIssues(const std::vector<Chord>& progression, const Key& key);
        std::vector<std::string> generateSuggestions(const std::vector<Chord>& progression, const Key& key);
        
        // Functional harmony rules
        bool isValidFunctionalProgression(ChordFunction from, ChordFunction to);
        std::vector<ChordFunction> getStrongProgressions(ChordFunction from);
        
        // Scale degree utilities
        int getScaleDegreeFromRoot(int chordRoot, const Key& key);
        int getNoteFromScaleDegree(int degree, const Key& key);
        
        // Supported scales
        std::vector<ScaleType> supportedScales;
        void initializeSupportedScales();
    };
    
} // namespace HarmonyEngine

// ============================================================================
// Implementation (merged from .cpp - header-only library)
// ============================================================================

namespace HarmonyEngine
{
    //==============================================================================
    // BasicMusicTheoryEngine Implementation
    
    inline BasicMusicTheoryEngine::BasicMusicTheoryEngine()
    {
        initializeSupportedScales();
    }
    
    inline DiatonicChordSet BasicMusicTheoryEngine::generateDiatonicChords(const Key& key)
    {
        DiatonicChordSet chordSet(key);
        
        if (!key.isValid() || !supportsScale(key.scaleType))
        {
            return chordSet; // Return empty set for invalid keys
        }
        
        // Generate triads for each scale degree
        chordSet.chords.reserve(7);
        chordSet.analysis.reserve(7);
        for (int degree = 1; degree <= 7; ++degree)
        {
            Chord chord = buildTriadOnDegree(degree, key);
            chordSet.chords.push_back(chord);
            
            RomanNumeralAnalysis analysis = analyzeChordInKey(chord, key);
            chordSet.analysis.push_back(analysis);
        }
        
        return chordSet;
    }
    
    inline RomanNumeralAnalysis BasicMusicTheoryEngine::analyzeChordInKey(const Chord& chord, const Key& key)
    {
        RomanNumeralAnalysis analysis;
        
        if (!chord.isValid() || !key.isValid())
        {
            return analysis;
        }
        
        // Determine scale degree
        int scaleDegree = getScaleDegreeFromRoot(chord.rootNote, key);
        if (scaleDegree == 0) // Not in key
        {
            analysis.symbol = "?";
            return analysis;
        }
        
        analysis.scaleDegree = scaleDegree;
        analysis.numeral = getRomanNumeralForDegree(scaleDegree, key);
        analysis.quality = getQualityForChordType(chord.type);
        analysis.function = identifyFunction(chord, key);
        analysis.symbol = formatRomanNumeral(analysis);
        
        return analysis;
    }
    
    inline ProgressionValidationResult BasicMusicTheoryEngine::validateProgression(const std::vector<Chord>& progression, const Key& key)
    {
        ProgressionValidationResult result;
        
        if (progression.empty() || !key.isValid())
        {
            result.issues.push_back("Invalid progression or key");
            return result;
        }
        
        // Analyze each chord
        result.analysis.reserve(progression.size());
        for (const auto& chord : progression)
        {
            result.analysis.push_back(analyzeChordInKey(chord, key));
        }
        
        // Calculate harmony score
        result.harmonyScore = calculateHarmonyScore(progression, key);
        
        // Find issues
        result.issues = findProgressionIssues(progression, key);
        
        // Generate suggestions
        result.suggestions = generateSuggestions(progression, key);
        
        // Determine overall validity
        result.isValid = result.harmonyScore >= 0.6f && result.issues.size() <= 2;
        
        return result;
    }
    
    inline ChordFunction BasicMusicTheoryEngine::identifyFunction(const Chord& chord, const Key& key)
    {
        return analyzer.identifyHarmonicFunction(chord, key);
    }
    
    inline std::vector<Chord> BasicMusicTheoryEngine::getChordsWithFunction(ChordFunction function, const Key& key)
    {
        std::vector<Chord> chords;
        auto diatonicSet = generateDiatonicChords(key);
        chords.reserve(diatonicSet.chords.size());
        
        for (size_t i = 0; i < diatonicSet.chords.size(); ++i)
        {
            if (diatonicSet.analysis[i].function == function)
            {
                chords.push_back(diatonicSet.chords[i]);
            }
        }
        
        return chords;
    }
    
    inline std::vector<Chord> BasicMusicTheoryEngine::suggestNextChords(const std::vector<Chord>& progression, const Key& key)
    {
        std::vector<Chord> suggestions;
        
        if (progression.empty() || !key.isValid())
        {
            // Suggest tonic chord for empty progression
            auto tonicChords = getChordsWithFunction(ChordFunction::Tonic, key);
            if (!tonicChords.empty())
                suggestions.push_back(tonicChords[0]);
            return suggestions;
        }
        
        // Get function of last chord
        ChordFunction lastFunction = identifyFunction(progression.back(), key);
        
        // Get strong progressions from this function
        auto strongProgressions = getStrongProgressions(lastFunction);
        
        // Find chords with those functions
        for (ChordFunction targetFunction : strongProgressions)
        {
            auto functionChords = getChordsWithFunction(targetFunction, key);
            suggestions.insert(suggestions.end(), functionChords.begin(), functionChords.end());
        }
        
        // Limit to 4 suggestions
        if (suggestions.size() > 4)
        {
            suggestions.resize(4);
        }
        
        return suggestions;
    }
    
    inline bool BasicMusicTheoryEngine::isStrongProgression(const Chord& from, const Chord& to, const Key& key)
    {
        ChordFunction fromFunction = identifyFunction(from, key);
        ChordFunction toFunction = identifyFunction(to, key);
        
        return isValidFunctionalProgression(fromFunction, toFunction);
    }
    
    inline std::vector<ScaleType> BasicMusicTheoryEngine::getSupportedScales()
    {
        return supportedScales;
    }
    
    inline bool BasicMusicTheoryEngine::supportsScale(ScaleType scale)
    {
        return std::find(supportedScales.begin(), supportedScales.end(), scale) != supportedScales.end();
    }
    
    //==============================================================================
    // Private Helper Methods
    
    inline Chord BasicMusicTheoryEngine::buildTriadOnDegree(int degree, const Key& key)
    {
        if (degree < 1 || degree > 7)
        {
            return Chord(); // Invalid degree
        }
        
        int rootNote = getNoteFromScaleDegree(degree, key);
        ChordType chordType = getTriadTypeForDegree(degree, key);
        
        Chord chord(chordType, rootNote);
        
        // Build triad notes
        auto scaleDegrees = key.getScaleDegrees();
        int rootIndex = (degree - 1) % 7;
        int thirdIndex = (degree + 1) % 7;
        int fifthIndex = (degree + 3) % 7;
        
        int rootPitch = rootNote;
        int thirdPitch = (rootNote / 12) * 12 + scaleDegrees[thirdIndex];
        int fifthPitch = (rootNote / 12) * 12 + scaleDegrees[fifthIndex];
        
        // Ensure proper octave placement
        if (thirdPitch <= rootPitch) thirdPitch += 12;
        if (fifthPitch <= thirdPitch) fifthPitch += 12;
        
        chord.notes = {
            Note(rootPitch, 1.0f, 0.8f, 0.0f),
            Note(thirdPitch, 1.0f, 0.8f, 0.0f),
            Note(fifthPitch, 1.0f, 0.8f, 0.0f)
        };
        
        return chord;
    }

    inline ChordType BasicMusicTheoryEngine::getTriadTypeForDegree(int degree, const Key& key)
    {
        // Calculate chord type based on scale intervals
        auto scaleDegrees = key.getScaleDegrees();
        if (scaleDegrees.size() < 7 || degree < 1 || degree > 7)
        {
            return ChordType::Major; // Default fallback
        }
        
        // Get the intervals for the triad (root, third, fifth)
        int rootIndex = (degree - 1) % 7;
        int thirdIndex = (degree + 1) % 7;
        int fifthIndex = (degree + 3) % 7;
        
        int rootNote = scaleDegrees[rootIndex];
        int thirdNote = scaleDegrees[thirdIndex];
        int fifthNote = scaleDegrees[fifthIndex];
        
        // Calculate intervals from root
        int thirdInterval = (thirdNote - rootNote + 12) % 12;
        int fifthInterval = (fifthNote - rootNote + 12) % 12;
        
        // Determine chord type based on intervals
        if (thirdInterval == 4 && fifthInterval == 7)
        {
            return ChordType::Major;
        }
        else if (thirdInterval == 3 && fifthInterval == 7)
        {
            return ChordType::Minor;
        }
        else if (thirdInterval == 3 && fifthInterval == 6)
        {
            return ChordType::Diminished;
        }
        else if (thirdInterval == 4 && fifthInterval == 8)
        {
            return ChordType::Augmented;
        }
        
        // Default fallback based on third interval
        return (thirdInterval == 4) ? ChordType::Major : ChordType::Minor;
    }
    
    inline ChordType BasicMusicTheoryEngine::getSeventhTypeForDegree(int degree, const Key& key)
    {
        // Calculate seventh chord type based on scale intervals
        auto scaleDegrees = key.getScaleDegrees();
        if (scaleDegrees.size() < 7 || degree < 1 || degree > 7)
        {
            ChordType triadType = getTriadTypeForDegree(degree, key);
            return (triadType == ChordType::Major) ? ChordType::Major7 : ChordType::Minor7;
        }
        
        // Get the intervals for the seventh chord (root, third, fifth, seventh)
        int rootIndex = (degree - 1) % 7;
        int thirdIndex = (degree + 1) % 7;
        int fifthIndex = (degree + 3) % 7;
        int seventhIndex = (degree + 5) % 7;
        
        int rootNote = scaleDegrees[rootIndex];
        int thirdNote = scaleDegrees[thirdIndex];
        int fifthNote = scaleDegrees[fifthIndex];
        int seventhNote = scaleDegrees[seventhIndex];
        
        // Calculate intervals from root
        int thirdInterval = (thirdNote - rootNote + 12) % 12;
        int fifthInterval = (fifthNote - rootNote + 12) % 12;
        int seventhInterval = (seventhNote - rootNote + 12) % 12;
        
        // Determine seventh chord type based on intervals
        if (thirdInterval == 4 && fifthInterval == 7 && seventhInterval == 11)
        {
            return ChordType::Major7;
        }
        else if (thirdInterval == 3 && fifthInterval == 7 && seventhInterval == 10)
        {
            return ChordType::Minor7;
        }
        else if (thirdInterval == 4 && fifthInterval == 7 && seventhInterval == 10)
        {
            return ChordType::Dominant7;
        }
        else if (thirdInterval == 3 && fifthInterval == 6 && seventhInterval == 10)
        {
            return ChordType::HalfDiminished7;
        }
        else if (thirdInterval == 3 && fifthInterval == 6 && seventhInterval == 9)
        {
            return ChordType::Diminished7;
        }
        else if (thirdInterval == 3 && fifthInterval == 7 && seventhInterval == 11)
        {
            return ChordType::MinorMajor7;
        }
        else if (thirdInterval == 4 && fifthInterval == 8 && seventhInterval == 10)
        {
            return ChordType::Augmented7;
        }
        
        // Default fallback based on triad type
        ChordType triadType = getTriadTypeForDegree(degree, key);
        if (triadType == ChordType::Major)
        {
            return (seventhInterval == 11) ? ChordType::Major7 : ChordType::Dominant7;
        }
        else if (triadType == ChordType::Minor)
        {
            return (seventhInterval == 11) ? ChordType::MinorMajor7 : ChordType::Minor7;
        }
        else if (triadType == ChordType::Diminished)
        {
            return (seventhInterval == 9) ? ChordType::Diminished7 : ChordType::HalfDiminished7;
        }
        
        return ChordType::Minor7; // Final fallback
    }
    
    inline RomanNumeralType BasicMusicTheoryEngine::getRomanNumeralForDegree(int degree, const Key& key)
    {
        ChordType chordType = getTriadTypeForDegree(degree, key);
        bool isMinorOrDim = (chordType == ChordType::Minor || chordType == ChordType::Diminished);
        
        switch (degree)
        {
            case 1: return isMinorOrDim ? RomanNumeralType::i : RomanNumeralType::I;
            case 2: return isMinorOrDim ? RomanNumeralType::ii : RomanNumeralType::II;
            case 3: return isMinorOrDim ? RomanNumeralType::iii : RomanNumeralType::III;
            case 4: return isMinorOrDim ? RomanNumeralType::iv : RomanNumeralType::IV;
            case 5: return isMinorOrDim ? RomanNumeralType::v : RomanNumeralType::V;
            case 6: return isMinorOrDim ? RomanNumeralType::vi : RomanNumeralType::VI;
            case 7: return isMinorOrDim ? RomanNumeralType::vii : RomanNumeralType::VII;
            default: return RomanNumeralType::I;
        }
    }
    
    inline RomanNumeralQuality BasicMusicTheoryEngine::getQualityForChordType(ChordType type)
    {
        switch (type)
        {
            case ChordType::Major: return RomanNumeralQuality::Major;
            case ChordType::Minor: return RomanNumeralQuality::Minor;
            case ChordType::Diminished: return RomanNumeralQuality::Diminished;
            case ChordType::Augmented: return RomanNumeralQuality::Augmented;
            case ChordType::Major7: return RomanNumeralQuality::Major7;
            case ChordType::Minor7: return RomanNumeralQuality::Minor7;
            case ChordType::Dominant7: return RomanNumeralQuality::Dominant7;
            case ChordType::HalfDiminished7: return RomanNumeralQuality::HalfDiminished7;
            case ChordType::Diminished7: return RomanNumeralQuality::Diminished7;
            default: return RomanNumeralQuality::Major;
        }
    }
    
    inline std::string BasicMusicTheoryEngine::formatRomanNumeral(const RomanNumeralAnalysis& analysis)
    {
        static const std::string numerals[] = {
            "I", "II", "III", "IV", "V", "VI", "VII",
            "i", "ii", "iii", "iv", "v", "vi", "vii"
        };
        
        std::string symbol = numerals[static_cast<int>(analysis.numeral)];
        
        // Add quality indicators
        switch (analysis.quality)
        {
            case RomanNumeralQuality::Diminished:
                symbol += "°";
                break;
            case RomanNumeralQuality::Augmented:
                symbol += "+";
                break;
            case RomanNumeralQuality::Dominant7:
                symbol += "7";
                break;
            case RomanNumeralQuality::Major7:
                symbol += "maj7";
                break;
            case RomanNumeralQuality::Minor7:
                symbol += "7";
                break;
            case RomanNumeralQuality::HalfDiminished7:
                symbol += "ø7";
                break;
            case RomanNumeralQuality::Diminished7:
                symbol += "°7";
                break;
            default:
                break;
        }
        
        return symbol;
    }
    
    inline float BasicMusicTheoryEngine::calculateHarmonyScore(const std::vector<Chord>& progression, const Key& key)
    {
        if (progression.empty())
        {
            return 0.0f;
        }
        
        float score = 0.0f;
        int validProgressions = 0;
        
        // Score individual chords (are they in key?)
        for (const auto& chord : progression)
        {
            int scaleDegree = getScaleDegreeFromRoot(chord.rootNote, key);
            if (scaleDegree > 0)
            {
                score += 0.3f; // Base score for being in key
            }
        }
        
        // Score chord progressions
        for (size_t i = 1; i < progression.size(); ++i)
        {
            if (isStrongProgression(progression[i-1], progression[i], key))
            {
                score += 0.4f; // Strong progression bonus
                validProgressions++;
            }
            else
            {
                ChordFunction from = identifyFunction(progression[i-1], key);
                ChordFunction to = identifyFunction(progression[i], key);
                if (isValidFunctionalProgression(from, to))
                {
                    score += 0.2f; // Weak but valid progression
                    validProgressions++;
                }
            }
        }
        
        // Normalize score
        float maxScore = progression.size() * 0.3f + (progression.size() - 1) * 0.4f;
        return (maxScore > 0.0f) ? std::min(1.0f, score / maxScore) : 0.0f;
    }
    
    inline std::vector<std::string> BasicMusicTheoryEngine::findProgressionIssues(const std::vector<Chord>& progression, const Key& key)
    {
        std::vector<std::string> issues;
        
        // Check for out-of-key chords
        for (size_t i = 0; i < progression.size(); ++i)
        {
            int scaleDegree = getScaleDegreeFromRoot(progression[i].rootNote, key);
            if (scaleDegree == 0)
            {
                issues.push_back("Chord " + std::to_string(static_cast<int>(i + 1)) + " (" + 
                               progression[i].getChordSymbol() + ") is not in key");
            }
        }
        
        // Check for weak progressions
        int weakProgressions = 0;
        for (size_t i = 1; i < progression.size(); ++i)
        {
            if (!isStrongProgression(progression[i-1], progression[i], key))
            {
                weakProgressions++;
            }
        }
        
        if (weakProgressions > progression.size() / 2)
        {
            issues.push_back("Many weak harmonic progressions detected");
        }
        
        return issues;
    }
    
    inline std::vector<std::string> BasicMusicTheoryEngine::generateSuggestions(const std::vector<Chord>& progression, const Key& key)
    {
        std::vector<std::string> suggestions;
        
        if (progression.empty())
        {
            suggestions.push_back("Start with a tonic chord (I or i)");
            return suggestions;
        }
        
        // Suggest strong next chords
        auto nextChords = suggestNextChords(progression, key);
        if (!nextChords.empty())
        {
            std::string suggestion = "Consider these next chords: ";
            for (size_t i = 0; i < nextChords.size(); ++i)
            {
                if (i > 0) suggestion += ", ";
                auto analysis = analyzeChordInKey(nextChords[i], key);
                suggestion += analysis.symbol;
            }
            suggestions.push_back(suggestion);
        }
        
        // Suggest ending with tonic if not already
        ChordFunction lastFunction = identifyFunction(progression.back(), key);
        if (lastFunction != ChordFunction::Tonic)
        {
            suggestions.push_back("Consider ending with a tonic chord for resolution");
        }
        
        return suggestions;
    }
    
    inline bool BasicMusicTheoryEngine::isValidFunctionalProgression(ChordFunction from, ChordFunction to)
    {
        // Basic functional harmony rules
        switch (from)
        {
            case ChordFunction::Tonic:
                return (to == ChordFunction::Subdominant || to == ChordFunction::Predominant || 
                       to == ChordFunction::Mediant || to == ChordFunction::Submediant);
                
            case ChordFunction::Subdominant:
            case ChordFunction::Predominant:
                return (to == ChordFunction::Dominant || to == ChordFunction::Tonic);
                
            case ChordFunction::Dominant:
                return (to == ChordFunction::Tonic || to == ChordFunction::Submediant);
                
            case ChordFunction::Mediant:
                return (to == ChordFunction::Submediant || to == ChordFunction::Subdominant);
                
            case ChordFunction::Submediant:
                return (to == ChordFunction::Subdominant || to == ChordFunction::Predominant || 
                       to == ChordFunction::Dominant);
                
            case ChordFunction::LeadingTone:
                return (to == ChordFunction::Tonic);
                
            default:
                return true; // Allow secondary functions
        }
    }
    
    inline std::vector<ChordFunction> BasicMusicTheoryEngine::getStrongProgressions(ChordFunction from)
    {
        switch (from)
        {
            case ChordFunction::Tonic:
                return {ChordFunction::Subdominant, ChordFunction::Predominant};
                
            case ChordFunction::Subdominant:
            case ChordFunction::Predominant:
                return {ChordFunction::Dominant};
                
            case ChordFunction::Dominant:
                return {ChordFunction::Tonic};
                
            case ChordFunction::Submediant:
                return {ChordFunction::Subdominant, ChordFunction::Predominant};
                
            case ChordFunction::LeadingTone:
                return {ChordFunction::Tonic};
                
            default:
                return {ChordFunction::Tonic};
        }
    }

    inline int BasicMusicTheoryEngine::getScaleDegreeFromRoot(int chordRoot, const Key& key)
    {
        auto scaleDegrees = key.getScaleDegrees();
        int normalizedRoot = chordRoot % 12;
        
        for (size_t i = 0; i < scaleDegrees.size(); ++i)
        {
            if (scaleDegrees[i] == normalizedRoot)
            {
                return static_cast<int>(i) + 1; // 1-based scale degrees
            }
        }
        
        return 0; // Not in scale
    }
    
    inline int BasicMusicTheoryEngine::getNoteFromScaleDegree(int degree, const Key& key)
    {
        if (degree < 1 || degree > 7)
        {
            return key.tonicNote; // Default to tonic
        }
        
        auto scaleDegrees = key.getScaleDegrees();
        return scaleDegrees[degree - 1] + 60; // Return in 4th octave
    }
    
    inline void BasicMusicTheoryEngine::initializeSupportedScales()
    {
        supportedScales = {
            // Major scale modes
            ScaleType::Major,
            ScaleType::Dorian,
            ScaleType::Phrygian,
            ScaleType::Lydian,
            ScaleType::Mixolydian,
            ScaleType::Aeolian,
            ScaleType::Locrian,
            
            // Minor scales
            ScaleType::NaturalMinor,
            ScaleType::HarmonicMinor,
            ScaleType::MelodicMinor,
            
            // Harmonic minor modes
            ScaleType::HarmonicMinorMode1,
            ScaleType::LocrianNat6,
            ScaleType::IonianAug5,
            ScaleType::DorianSharp4,
            ScaleType::PhrygianDominant,
            ScaleType::LydianSharp2,
            ScaleType::SuperLocrianDim7
        };
    }
    
} // namespace HarmonyEngine
