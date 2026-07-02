/*
  ==============================================================================

    AdvancedHarmony.h
    Created: Advanced harmonic techniques interface and implementation
    
    This file contains the AdvancedHarmonyEngine interface and implementation
    that provides borrowed chords, secondary dominants, Neapolitan chords,
    and chromatic mediant functionality.

  ==============================================================================
*/

#pragma once

// Implementation includes (header-only)
#include <algorithm>
#include <cmath>

#include "MusicTheory.h"
#include "MusicTheoryEngine.h"
#include <map>
#include <memory>

namespace HarmonyEngine
{
    //==============================================================================
    // Advanced Harmony Structures
    
    enum class BorrowedChordSource
    {
        ParallelMajor,      // Borrowing from parallel major key
        ParallelMinor,      // Borrowing from parallel minor key
        RelativeMajor,      // Borrowing from relative major key
        RelativeMinor,      // Borrowing from relative minor key
        Dorian,             // Borrowing from dorian mode
        Mixolydian,         // Borrowing from mixolydian mode
        Lydian,             // Borrowing from lydian mode
        Phrygian            // Borrowing from phrygian mode
    };
    
    struct BorrowedChord
    {
        Chord chord;
        BorrowedChordSource source;
        Key sourceKey;
        int scaleDegree;        // Scale degree in original key
        std::string symbol;    // Roman numeral with accidentals (e.g., "bVI", "iv")
        
        BorrowedChord() : scaleDegree(1) {}
        
        bool isValid() const
        {
            return chord.isValid() && sourceKey.isValid() && 
                   scaleDegree >= 1 && scaleDegree <= 7;
        }
    };
    
    struct SecondaryDominant
    {
        Chord dominant;         // The secondary dominant chord (V7/x)
        Chord target;          // The target chord (x)
        int targetDegree;      // Scale degree of target in original key
        std::string symbol;   // Roman numeral (e.g., "V7/vi", "V/V")
        bool needsAccidentals; // Whether chord requires chromatic alterations
        
        SecondaryDominant() : targetDegree(1), needsAccidentals(false) {}
        
        bool isValid() const
        {
            return dominant.isValid() && target.isValid() && 
                   targetDegree >= 1 && targetDegree <= 7;
        }
    };
    
    struct NeapolitanChord
    {
        Chord chord;           // The Neapolitan chord (typically bII6)
        Inversion inversion;   // Usually first inversion (6)
        std::string symbol;   // Roman numeral (e.g., "N6", "bII6")
        std::vector<Chord> resolutionOptions; // Typical resolution chords
        
        NeapolitanChord() : inversion(Inversion::First) {}
        
        bool isValid() const
        {
            return chord.isValid();
        }
    };
    
    enum class ChromaticMediantType
    {
        FlatVI,     // bVI (flat sixth)
        FlatIII,    // bIII (flat third)
        SharpVI,    // #VI (sharp sixth - enharmonic bVII)
        FlatVII,    // bVII (flat seventh)
        FlatII,     // bII (flat second - Neapolitan relationship)
        SharpIV     // #IV (sharp fourth - tritone relationship)
    };
    
    struct ChromaticMediant
    {
        Chord chord;
        ChromaticMediantType type;
        int intervalFromTonic;  // Semitones from tonic
        std::string symbol;    // Roman numeral with accidentals
        bool isCommonTone;      // Whether it shares common tones with tonic
        
        ChromaticMediant() : intervalFromTonic(0), isCommonTone(false) {}
        
        bool isValid() const
        {
            return chord.isValid() && intervalFromTonic >= 0 && intervalFromTonic < 12;
        }
    };
    
    //==============================================================================
    // Advanced Harmony Engine Interface
    
    class AdvancedHarmonyEngine
    {
    public:
        virtual ~AdvancedHarmonyEngine() = default;
        
        // Borrowed chord functionality
        virtual std::vector<BorrowedChord> getBorrowedChords(const Key& originalKey, BorrowedChordSource source) = 0;
        virtual std::vector<BorrowedChord> getAllBorrowedChords(const Key& originalKey) = 0;
        virtual bool isBorrowedChord(const Chord& chord, const Key& originalKey) = 0;
        virtual BorrowedChord analyzeBorrowedChord(const Chord& chord, const Key& originalKey) = 0;
        
        // Secondary dominant functionality
        virtual std::vector<SecondaryDominant> getSecondaryDominants(const Key& key) = 0;
        virtual SecondaryDominant createSecondaryDominant(int targetDegree, const Key& key) = 0;
        virtual bool isValidSecondaryResolution(const Chord& secondary, const Chord& target, const Key& key) = 0;
        virtual std::vector<Chord> getSecondaryResolutionOptions(const SecondaryDominant& secondary, const Key& key) = 0;
        
        // Neapolitan chord functionality
        virtual NeapolitanChord createNeapolitanChord(const Key& key) = 0;
        virtual std::vector<Chord> getNeapolitanResolutions(const Key& key) = 0;
        virtual bool isValidNeapolitanVoiceLeading(const NeapolitanChord& neapolitan, const Chord& resolution) = 0;
        
        // Chromatic mediant functionality
        virtual std::vector<ChromaticMediant> getChromaticMediants(const Key& key) = 0;
        virtual ChromaticMediant createChromaticMediant(ChromaticMediantType type, const Key& key) = 0;
        virtual bool isValidChromaticMediantProgression(const Chord& from, const ChromaticMediant& mediant, const Key& key) = 0;
        virtual std::vector<Chord> getChromaticMediantResolutions(const ChromaticMediant& mediant, const Key& key) = 0;
        
        // Validation and analysis
        virtual bool validateAdvancedProgression(const std::vector<Chord>& progression, const Key& key) = 0;
        virtual std::vector<std::string> analyzeAdvancedTechniques(const std::vector<Chord>& progression, const Key& key) = 0;
    };
    
    //==============================================================================
    // Basic Implementation
    
    class BasicAdvancedHarmonyEngine : public AdvancedHarmonyEngine
    {
    public:
        BasicAdvancedHarmonyEngine();
        ~BasicAdvancedHarmonyEngine() override = default;
        
        // Borrowed chord implementation
        std::vector<BorrowedChord> getBorrowedChords(const Key& originalKey, BorrowedChordSource source) override;
        std::vector<BorrowedChord> getAllBorrowedChords(const Key& originalKey) override;
        bool isBorrowedChord(const Chord& chord, const Key& originalKey) override;
        BorrowedChord analyzeBorrowedChord(const Chord& chord, const Key& originalKey) override;
        
        // Secondary dominant implementation
        std::vector<SecondaryDominant> getSecondaryDominants(const Key& key) override;
        SecondaryDominant createSecondaryDominant(int targetDegree, const Key& key) override;
        bool isValidSecondaryResolution(const Chord& secondary, const Chord& target, const Key& key) override;
        std::vector<Chord> getSecondaryResolutionOptions(const SecondaryDominant& secondary, const Key& key) override;
        
        // Neapolitan chord implementation
        NeapolitanChord createNeapolitanChord(const Key& key) override;
        std::vector<Chord> getNeapolitanResolutions(const Key& key) override;
        bool isValidNeapolitanVoiceLeading(const NeapolitanChord& neapolitan, const Chord& resolution) override;
        
        // Chromatic mediant implementation
        std::vector<ChromaticMediant> getChromaticMediants(const Key& key) override;
        ChromaticMediant createChromaticMediant(ChromaticMediantType type, const Key& key) override;
        bool isValidChromaticMediantProgression(const Chord& from, const ChromaticMediant& mediant, const Key& key) override;
        std::vector<Chord> getChromaticMediantResolutions(const ChromaticMediant& mediant, const Key& key) override;
        
        // Validation and analysis implementation
        bool validateAdvancedProgression(const std::vector<Chord>& progression, const Key& key) override;
        std::vector<std::string> analyzeAdvancedTechniques(const std::vector<Chord>& progression, const Key& key) override;
        
    private:
        BasicMusicTheoryEngine basicEngine;
        
        // Helper methods for borrowed chords
        Key createSourceKey(const Key& originalKey, BorrowedChordSource source);
        std::string formatBorrowedSymbol(int scaleDegree, ChordType type, BorrowedChordSource source);
        bool isChordInKey(const Chord& chord, const Key& key);
        
        // Helper methods for secondary dominants
        Chord buildDominantOfDegree(int targetDegree, const Key& key);
        bool isDominantFunction(const Chord& chord, const Chord& target);
        std::string formatSecondarySymbol(int targetDegree, bool isSeventh);
        
        // Helper methods for Neapolitan chords
        Chord buildNeapolitanTriad(const Key& key);
        Chord invertChord(const Chord& chord, Inversion inversion);
        
        // Helper methods for chromatic mediants
        int getChromaticMediantRoot(ChromaticMediantType type, const Key& key);
        ChordType getChromaticMediantType(ChromaticMediantType type);
        bool hasCommonTones(const Chord& chord1, const Chord& chord2);
        std::string formatChromaticMediantSymbol(ChromaticMediantType type);
        
        // Analysis helpers
        bool containsAdvancedTechnique(const std::vector<Chord>& progression, const Key& key);
        std::vector<std::string> identifyTechniques(const Chord& chord, const Key& key);
    };
    
} // namespace HarmonyEngine

// ============================================================================
// Implementation (merged from .cpp - header-only library)
// ============================================================================

namespace HarmonyEngine
{
    //==============================================================================
    // BasicAdvancedHarmonyEngine Implementation
    
    inline BasicAdvancedHarmonyEngine::BasicAdvancedHarmonyEngine()
    {
    }
    
    //==============================================================================
    // Borrowed Chord Implementation
    
    inline std::vector<BorrowedChord> BasicAdvancedHarmonyEngine::getBorrowedChords(const Key& originalKey, BorrowedChordSource source)
    {
        std::vector<BorrowedChord> borrowedChords;
        
        if (!originalKey.isValid())
        {
            return borrowedChords;
        }
        
        Key sourceKey = createSourceKey(originalKey, source);
        if (!sourceKey.isValid())
        {
            return borrowedChords;
        }
        
        // Generate diatonic chords from source key
        auto sourceChordSet = basicEngine.generateDiatonicChords(sourceKey);

        // Filter out chords that already exist in original key
        borrowedChords.reserve(sourceChordSet.chords.size());
        for (size_t i = 0; i < sourceChordSet.chords.size(); ++i)
        {
            const auto& chord = sourceChordSet.chords[i];

            if (!isChordInKey(chord, originalKey))
            {
                BorrowedChord borrowed;
                borrowed.chord = chord;
                borrowed.source = source;
                borrowed.sourceKey = sourceKey;
                borrowed.scaleDegree = static_cast<int>(i + 1);
                borrowed.symbol = formatBorrowedSymbol(borrowed.scaleDegree, chord.type, source);

                borrowedChords.push_back(borrowed);
            }
        }
        
        return borrowedChords;
    }
    
    inline std::vector<BorrowedChord> BasicAdvancedHarmonyEngine::getAllBorrowedChords(const Key& originalKey)
    {
        std::vector<BorrowedChord> allBorrowed;
        
        std::vector<BorrowedChordSource> sources = {
            BorrowedChordSource::ParallelMajor,
            BorrowedChordSource::ParallelMinor,
            BorrowedChordSource::Dorian,
            BorrowedChordSource::Mixolydian,
            BorrowedChordSource::Lydian
        };

        for (auto source : sources)
        {
            auto borrowed = getBorrowedChords(originalKey, source);
            allBorrowed.insert(allBorrowed.end(), borrowed.begin(), borrowed.end());
        }
        
        return allBorrowed;
    }
    
    inline bool BasicAdvancedHarmonyEngine::isBorrowedChord(const Chord& chord, const Key& originalKey)
    {
        if (!chord.isValid() || !originalKey.isValid())
        {
            return false;
        }
        
        // Check if chord exists in original key
        if (isChordInKey(chord, originalKey))
        {
            return false;
        }
        
        // Check if chord exists in any common borrowed sources
        auto allBorrowed = getAllBorrowedChords(originalKey);

        int chordPitchClass = chord.rootNote % 12;
        return std::any_of(allBorrowed.begin(), allBorrowed.end(),
            [&](const BorrowedChord& borrowed) {
                return borrowed.chord.rootNote % 12 == chordPitchClass &&
                       borrowed.chord.type == chord.type;
            });
    }
    
    inline BorrowedChord BasicAdvancedHarmonyEngine::analyzeBorrowedChord(const Chord& chord, const Key& originalKey)
    {
        BorrowedChord result;
        
        if (!isBorrowedChord(chord, originalKey))
        {
            return result; // Invalid result
        }
        
        auto allBorrowed = getAllBorrowedChords(originalKey);

        int chordPitchClass = chord.rootNote % 12;
        auto it = std::find_if(allBorrowed.begin(), allBorrowed.end(),
            [&](const BorrowedChord& borrowed) {
                return borrowed.chord.rootNote % 12 == chordPitchClass &&
                       borrowed.chord.type == chord.type;
            });

        if (it != allBorrowed.end())
        {
            result = *it;
            result.chord = chord; // Use the actual chord with correct octave
        }

        return result;
    }
    
    //==============================================================================
    // Secondary Dominant Implementation
    
    inline std::vector<SecondaryDominant> BasicAdvancedHarmonyEngine::getSecondaryDominants(const Key& key)
    {
        std::vector<SecondaryDominant> secondaries;
        
        if (!key.isValid())
        {
            return secondaries;
        }
        
        // Create secondary dominants for degrees ii, iii, IV, V, vi
        // (Skip I as it would be the regular V, and vii° as it's diminished)
        std::vector<int> targetDegrees = {2, 3, 4, 5, 6};
        secondaries.reserve(targetDegrees.size());

        for (int degree : targetDegrees)
        {
            auto secondary = createSecondaryDominant(degree, key);
            if (secondary.isValid())
            {
                secondaries.push_back(secondary);
            }
        }
        
        return secondaries;
    }
    
    inline SecondaryDominant BasicAdvancedHarmonyEngine::createSecondaryDominant(int targetDegree, const Key& key)
    {
        SecondaryDominant secondary;
        
        if (!key.isValid() || targetDegree < 1 || targetDegree > 7)
        {
            return secondary;
        }
        
        // Get the target chord
        auto diatonicSet = basicEngine.generateDiatonicChords(key);
        secondary.target = diatonicSet.getChordByDegree(targetDegree);
        secondary.targetDegree = targetDegree;
        
        // Build dominant of the target
        secondary.dominant = buildDominantOfDegree(targetDegree, key);
        
        // Determine if accidentals are needed
        secondary.needsAccidentals = !isChordInKey(secondary.dominant, key);
        
        // Format symbol
        secondary.symbol = formatSecondarySymbol(targetDegree, secondary.dominant.type == ChordType::Dominant7);
        
        return secondary;
    }
    
    inline bool BasicAdvancedHarmonyEngine::isValidSecondaryResolution(const Chord& secondary, const Chord& target, const Key& key)
    {
        if (!secondary.isValid() || !target.isValid() || !key.isValid())
        {
            return false;
        }
        
        // Check if secondary is a dominant function chord
        if (!isDominantFunction(secondary, target))
        {
            return false;
        }
        
        // Check voice leading: leading tone should resolve up, seventh should resolve down
        // This is a simplified check - full voice leading analysis would be more complex
        int secondaryRoot = secondary.rootNote % 12;
        int targetRoot = target.rootNote % 12;
        
        // The secondary dominant should be a fifth above the target (or fourth below)
        int expectedSecondaryRoot = (targetRoot + 7) % 12;
        
        return secondaryRoot == expectedSecondaryRoot;
    }
    
    inline std::vector<Chord> BasicAdvancedHarmonyEngine::getSecondaryResolutionOptions(const SecondaryDominant& secondary, const Key& key)
    {
        std::vector<Chord> resolutions;
        
        if (!secondary.isValid() || !key.isValid())
        {
            return resolutions;
        }
        
        // Primary resolution is the target chord
        resolutions.reserve(2);
        resolutions.push_back(secondary.target);
        
        // Deceptive resolution (typically to vi if resolving to I, etc.)
        int deceptiveDegree = (secondary.targetDegree == 1) ? 6 : secondary.targetDegree + 1;
        if (deceptiveDegree <= 7)
        {
            auto diatonicSet = basicEngine.generateDiatonicChords(key);
            resolutions.push_back(diatonicSet.getChordByDegree(deceptiveDegree));
        }
        
        return resolutions;
    }
    
    //==============================================================================
    // Neapolitan Chord Implementation
    
    inline NeapolitanChord BasicAdvancedHarmonyEngine::createNeapolitanChord(const Key& key)
    {
        NeapolitanChord neapolitan;
        
        if (!key.isValid())
        {
            return neapolitan;
        }
        
        // Build Neapolitan chord (bII major triad, typically in first inversion)
        neapolitan.chord = buildNeapolitanTriad(key);
        neapolitan.inversion = Inversion::First;
        neapolitan.symbol = "N6"; // Traditional symbol for Neapolitan sixth
        
        // Invert to first inversion
        neapolitan.chord = invertChord(neapolitan.chord, neapolitan.inversion);
        
        // Get typical resolutions (usually to V or I6/4)
        neapolitan.resolutionOptions = getNeapolitanResolutions(key);
        
        return neapolitan;
    }
    
    inline std::vector<Chord> BasicAdvancedHarmonyEngine::getNeapolitanResolutions(const Key& key)
    {
        std::vector<Chord> resolutions;
        
        if (!key.isValid())
        {
            return resolutions;
        }
        
        auto diatonicSet = basicEngine.generateDiatonicChords(key);

        resolutions.reserve(2);
        // Primary resolution: V chord
        resolutions.push_back(diatonicSet.getChordByDegree(5));
        
        // Alternative resolution: I chord in second inversion (I6/4)
        Chord tonic = diatonicSet.getChordByDegree(1);
        resolutions.push_back(invertChord(tonic, Inversion::Second));
        
        return resolutions;
    }
    
    inline bool BasicAdvancedHarmonyEngine::isValidNeapolitanVoiceLeading(const NeapolitanChord& neapolitan, const Chord& resolution)
    {
        if (!neapolitan.isValid() || !resolution.isValid())
        {
            return false;
        }
        
        // Simplified voice leading check:
        // The bass note of N6 (which is the third of the chord) should move down by step
        // This is a basic check - full voice leading analysis would be more comprehensive
        
        if (neapolitan.chord.notes.empty() || resolution.notes.empty())
        {
            return false;
        }
        
        int neapolitanBass = neapolitan.chord.notes[0].midiNumber % 12;
        int resolutionBass = resolution.notes[0].midiNumber % 12;

        // Check for stepwise motion (1 or 2 semitones), accounting for octave wraparound
        int interval = std::abs(resolutionBass - neapolitanBass);
        interval = std::min(interval, 12 - interval);

        return interval <= 2;
    }
    
    //==============================================================================
    // Chromatic Mediant Implementation
    
    inline std::vector<ChromaticMediant> BasicAdvancedHarmonyEngine::getChromaticMediants(const Key& key)
    {
        std::vector<ChromaticMediant> mediants;
        
        if (!key.isValid())
        {
            return mediants;
        }
        
        std::vector<ChromaticMediantType> types = {
            ChromaticMediantType::FlatVI,
            ChromaticMediantType::FlatIII,
            ChromaticMediantType::FlatVII,
            ChromaticMediantType::FlatII,
            ChromaticMediantType::SharpIV
        };
        mediants.reserve(types.size());

        for (auto type : types)
        {
            auto mediant = createChromaticMediant(type, key);
            if (mediant.isValid())
            {
                mediants.push_back(mediant);
            }
        }
        
        return mediants;
    }
    
    inline ChromaticMediant BasicAdvancedHarmonyEngine::createChromaticMediant(ChromaticMediantType type, const Key& key)
    {
        ChromaticMediant mediant;
        
        if (!key.isValid())
        {
            return mediant;
        }
        
        mediant.type = type;
        
        // Get root note for this chromatic mediant type
        int rootNote = getChromaticMediantRoot(type, key);
        mediant.intervalFromTonic = (rootNote - key.tonicNote + 12) % 12;
        
        // Get chord type
        ChordType chordType = getChromaticMediantType(type);
        
        // Build the chord
        mediant.chord = Chord(chordType, rootNote + 60); // Place in 4th octave
        
        // Build chord notes based on type
        std::vector<int> intervals;
        switch (chordType)
        {
            case ChordType::Major:
                intervals = {0, 4, 7};
                break;
            case ChordType::Minor:
                intervals = {0, 3, 7};
                break;
            default:
                intervals = {0, 4, 7}; // Default to major
                break;
        }
        
        mediant.chord.notes.clear();
        mediant.chord.notes.reserve(intervals.size());
        for (int interval : intervals)
        {
            mediant.chord.notes.push_back(Note(rootNote + 60 + interval));
        }
        
        // Check for common tones with tonic
        Chord tonicChord(ChordType::Major, key.tonicNote + 60);
        tonicChord.notes = {Note(key.tonicNote + 60), Note(key.tonicNote + 64), Note(key.tonicNote + 67)};
        mediant.isCommonTone = hasCommonTones(mediant.chord, tonicChord);
        
        // Format symbol
        mediant.symbol = formatChromaticMediantSymbol(type);
        
        return mediant;
    }
    
    inline bool BasicAdvancedHarmonyEngine::isValidChromaticMediantProgression(const Chord& from, const ChromaticMediant& mediant, const Key& key)
    {
        if (!from.isValid() || !mediant.isValid() || !key.isValid())
        {
            return false;
        }
        
        // Chromatic mediants work best when there are common tones or smooth voice leading
        // This is a simplified check
        
        if (mediant.isCommonTone)
        {
            return true; // Common tone mediants are generally valid
        }
        
        // Check for smooth bass motion
        int fromRoot = from.rootNote % 12;
        int mediantRoot = mediant.chord.rootNote % 12;
        int interval = std::abs(mediantRoot - fromRoot);
        interval = std::min(interval, 12 - interval);

        // Chromatic mediants typically move by third (3 or 4 semitones)
        return interval == 3 || interval == 4;
    }
    
    inline std::vector<Chord> BasicAdvancedHarmonyEngine::getChromaticMediantResolutions(const ChromaticMediant& mediant, const Key& key)
    {
        std::vector<Chord> resolutions;
        
        if (!mediant.isValid() || !key.isValid())
        {
            return resolutions;
        }
        
        auto diatonicSet = basicEngine.generateDiatonicChords(key);
        
        // Common resolutions for chromatic mediants
        resolutions.reserve(2);
        switch (mediant.type)
        {
            case ChromaticMediantType::FlatVI:
                // bVI often resolves to IV or V
                resolutions.push_back(diatonicSet.getChordByDegree(4));
                resolutions.push_back(diatonicSet.getChordByDegree(5));
                break;
                
            case ChromaticMediantType::FlatIII:
                // bIII often resolves to vi or IV
                resolutions.push_back(diatonicSet.getChordByDegree(6));
                resolutions.push_back(diatonicSet.getChordByDegree(4));
                break;
                
            case ChromaticMediantType::FlatVII:
                // bVII often resolves to I or vi
                resolutions.push_back(diatonicSet.getChordByDegree(1));
                resolutions.push_back(diatonicSet.getChordByDegree(6));
                break;
                
            default:
                // Default: resolve to tonic
                resolutions.push_back(diatonicSet.getChordByDegree(1));
                break;
        }
        
        return resolutions;
    }
    
    //==============================================================================
    // Validation and Analysis Implementation
    
    inline bool BasicAdvancedHarmonyEngine::validateAdvancedProgression(const std::vector<Chord>& progression, const Key& key)
    {
        if (progression.empty() || !key.isValid())
        {
            return false;
        }
        
        // Check if progression contains any advanced techniques
        return containsAdvancedTechnique(progression, key);
    }
    
    inline std::vector<std::string> BasicAdvancedHarmonyEngine::analyzeAdvancedTechniques(const std::vector<Chord>& progression, const Key& key)
    {
        std::vector<std::string> techniques;
        
        if (progression.empty() || !key.isValid())
        {
            return techniques;
        }

        techniques.reserve(progression.size() * 2);
        for (size_t i = 0; i < progression.size(); ++i)
        {
            auto chordTechniques = identifyTechniques(progression[i], key);

            for (const auto& technique : chordTechniques)
            {
                techniques.push_back("Chord " + std::to_string(i + 1) + ": " + technique);
            }
        }
        
        return techniques;
    }
    
    //==============================================================================
    // Private Helper Methods
    
    inline Key BasicAdvancedHarmonyEngine::createSourceKey(const Key& originalKey, BorrowedChordSource source)
    {
        Key sourceKey = originalKey;
        
        switch (source)
        {
            case BorrowedChordSource::ParallelMajor:
                sourceKey.scaleType = ScaleType::Major;
                break;
                
            case BorrowedChordSource::ParallelMinor:
                sourceKey.scaleType = ScaleType::NaturalMinor;
                break;
                
            case BorrowedChordSource::Dorian:
                sourceKey.scaleType = ScaleType::Dorian;
                break;
                
            case BorrowedChordSource::Mixolydian:
                sourceKey.scaleType = ScaleType::Mixolydian;
                break;
                
            case BorrowedChordSource::Lydian:
                sourceKey.scaleType = ScaleType::Lydian;
                break;
                
            case BorrowedChordSource::Phrygian:
                sourceKey.scaleType = ScaleType::Phrygian;
                break;
                
            case BorrowedChordSource::RelativeMajor:
                // Relative major is a minor third up
                sourceKey.tonicNote = (originalKey.tonicNote + 3) % 12;
                sourceKey.scaleType = ScaleType::Major;
                break;
                
            case BorrowedChordSource::RelativeMinor:
                // Relative minor is a minor third down
                sourceKey.tonicNote = (originalKey.tonicNote - 3 + 12) % 12;
                sourceKey.scaleType = ScaleType::NaturalMinor;
                break;
        }
        
        // Regenerate scale pattern for new type
        sourceKey = Key(sourceKey.tonicNote, sourceKey.scaleType);
        
        return sourceKey;
    }
    
    inline std::string BasicAdvancedHarmonyEngine::formatBorrowedSymbol(int scaleDegree, ChordType type, BorrowedChordSource source)
    {
        static const std::string numerals[] = {"I", "II", "III", "IV", "V", "VI", "VII"};
        static const std::string minorNumerals[] = {"i", "ii", "iii", "iv", "v", "vi", "vii"};
        
        std::string symbol;
        
        // Determine if we need flat or sharp accidentals based on source
        bool needsFlat = (source == BorrowedChordSource::ParallelMinor && 
                         (scaleDegree == 3 || scaleDegree == 6 || scaleDegree == 7));
        
        if (needsFlat)
        {
            symbol += "b";
        }
        
        // Choose numeral based on chord type
        if (type == ChordType::Minor || type == ChordType::Diminished)
        {
            symbol += minorNumerals[scaleDegree - 1];
        }
        else
        {
            symbol += numerals[scaleDegree - 1];
        }
        
        // Add quality indicators
        if (type == ChordType::Diminished)
        {
            symbol += "°";
        }
        
        return symbol;
    }
    
    inline bool BasicAdvancedHarmonyEngine::isChordInKey(const Chord& chord, const Key& key)
    {
        if (!chord.isValid() || !key.isValid())
        {
            return false;
        }
        
        auto diatonicSet = basicEngine.generateDiatonicChords(key);

        int chordPitchClass = chord.rootNote % 12;
        return std::any_of(diatonicSet.chords.begin(), diatonicSet.chords.end(),
            [&](const Chord& diatonicChord) {
                return diatonicChord.rootNote % 12 == chordPitchClass &&
                       diatonicChord.type == chord.type;
            });
    }
    
    inline Chord BasicAdvancedHarmonyEngine::buildDominantOfDegree(int targetDegree, const Key& key)
    {
        if (!key.isValid() || targetDegree < 1 || targetDegree > 7)
        {
            return Chord();
        }
        
        // Get the target chord root
        auto diatonicSet = basicEngine.generateDiatonicChords(key);
        Chord target = diatonicSet.getChordByDegree(targetDegree);
        
        // Build dominant seventh chord a fifth above the target
        int dominantRoot = (target.rootNote + 7) % 12 + 60; // Place in 4th octave
        
        Chord dominant(ChordType::Dominant7, dominantRoot);
        
        // Build dominant seventh chord notes
        dominant.notes = {
            Note(dominantRoot),           // Root
            Note(dominantRoot + 4),       // Major third
            Note(dominantRoot + 7),       // Perfect fifth
            Note(dominantRoot + 10)       // Minor seventh
        };
        
        return dominant;
    }
    
    inline bool BasicAdvancedHarmonyEngine::isDominantFunction(const Chord& chord, const Chord& target)
    {
        if (!chord.isValid() || !target.isValid())
        {
            return false;
        }
        
        // Check if chord is a fifth above target (dominant relationship)
        int chordRoot = chord.rootNote % 12;
        int targetRoot = target.rootNote % 12;
        int interval = (chordRoot - targetRoot + 12) % 12;
        
        return interval == 7; // Perfect fifth
    }
    
    inline std::string BasicAdvancedHarmonyEngine::formatSecondarySymbol(int targetDegree, bool isSeventh)
    {
        static const std::string numerals[] = {"I", "II", "III", "IV", "V", "VI", "VII"};
        
        std::string symbol = isSeventh ? "V7/" : "V/";
        symbol += numerals[targetDegree - 1];
        
        return symbol;
    }
    
    inline Chord BasicAdvancedHarmonyEngine::buildNeapolitanTriad(const Key& key)
    {
        if (!key.isValid())
        {
            return Chord();
        }
        
        // Neapolitan is bII major triad
        int neapolitanRoot = (key.tonicNote + 1) % 12 + 60; // Flat second degree
        
        Chord neapolitan(ChordType::Major, neapolitanRoot);
        
        // Build major triad
        neapolitan.notes = {
            Note(neapolitanRoot),         // Root
            Note(neapolitanRoot + 4),     // Major third
            Note(neapolitanRoot + 7)      // Perfect fifth
        };
        
        return neapolitan;
    }
    
    inline Chord BasicAdvancedHarmonyEngine::invertChord(const Chord& chord, Inversion inversion)
    {
        Chord inverted = chord;
        inverted.inversion = inversion;
        
        if (chord.notes.size() < 3)
        {
            return inverted; // Can't invert with less than 3 notes
        }
        
        // Rearrange notes based on inversion
        switch (inversion)
        {
            case Inversion::Root:
                // No change needed
                break;
                
            case Inversion::First:
                // Move root to top
                if (!inverted.notes.empty())
                {
                    Note root = inverted.notes[0];
                    root.midiNumber += 12; // Move up an octave
                    inverted.notes.erase(inverted.notes.begin());
                    inverted.notes.push_back(root);
                }
                break;
                
            case Inversion::Second:
                // Move root and third to top
                if (inverted.notes.size() >= 2)
                {
                    Note root = inverted.notes[0];
                    Note third = inverted.notes[1];
                    root.midiNumber += 12;
                    third.midiNumber += 12;
                    inverted.notes.erase(inverted.notes.begin(), inverted.notes.begin() + 2);
                    inverted.notes.push_back(root);
                    inverted.notes.push_back(third);
                }
                break;
                
            default:
                break;
        }
        
        return inverted;
    }
    
    inline int BasicAdvancedHarmonyEngine::getChromaticMediantRoot(ChromaticMediantType type, const Key& key)
    {
        int tonic = key.tonicNote;
        
        switch (type)
        {
            case ChromaticMediantType::FlatVI:
                return (tonic + 8) % 12; // Flat sixth
                
            case ChromaticMediantType::FlatIII:
                return (tonic + 3) % 12; // Flat third
                
            case ChromaticMediantType::FlatVII:
                return (tonic + 10) % 12; // Flat seventh
                
            case ChromaticMediantType::FlatII:
                return (tonic + 1) % 12; // Flat second
                
            case ChromaticMediantType::SharpIV:
                return (tonic + 6) % 12; // Sharp fourth (tritone)
                
            case ChromaticMediantType::SharpVI:
                return (tonic + 9) % 12; // Sharp sixth
                
            default:
                return tonic;
        }
    }
    
    inline ChordType BasicAdvancedHarmonyEngine::getChromaticMediantType(ChromaticMediantType type)
    {
        // Most chromatic mediants are major triads, but this can vary
        switch (type)
        {
            case ChromaticMediantType::FlatVI:
            case ChromaticMediantType::FlatIII:
            case ChromaticMediantType::FlatVII:
                return ChordType::Major;
                
            case ChromaticMediantType::FlatII:
                return ChordType::Major; // Neapolitan relationship
                
            case ChromaticMediantType::SharpIV:
                return ChordType::Major; // Tritone substitution
                
            default:
                return ChordType::Major;
        }
    }
    
    inline bool BasicAdvancedHarmonyEngine::hasCommonTones(const Chord& chord1, const Chord& chord2)
    {
        if (!chord1.isValid() || !chord2.isValid())
        {
            return false;
        }
        
        // Check for common pitch classes
        for (const auto& note1 : chord1.notes)
        {
            int pitchClass1 = note1.midiNumber % 12;

            bool shared = std::any_of(chord2.notes.begin(), chord2.notes.end(),
                [pitchClass1](const Note& note2) {
                    return note2.midiNumber % 12 == pitchClass1;
                });

            if (shared)
                return true;
        }

        return false;
    }
    
    inline std::string BasicAdvancedHarmonyEngine::formatChromaticMediantSymbol(ChromaticMediantType type)
    {
        switch (type)
        {
            case ChromaticMediantType::FlatVI:
                return "bVI";
                
            case ChromaticMediantType::FlatIII:
                return "bIII";
                
            case ChromaticMediantType::FlatVII:
                return "bVII";
                
            case ChromaticMediantType::FlatII:
                return "bII";
                
            case ChromaticMediantType::SharpIV:
                return "#IV";
                
            case ChromaticMediantType::SharpVI:
                return "#VI";
                
            default:
                return "?";
        }
    }
    
    inline bool BasicAdvancedHarmonyEngine::containsAdvancedTechnique(const std::vector<Chord>& progression, const Key& key)
    {
        int expectedNeapolitan = (key.tonicNote + 1) % 12;

        return std::any_of(progression.begin(), progression.end(),
            [&](const Chord& chord) {
                if (isBorrowedChord(chord, key))
                    return true;

                // Check for secondary dominants (simplified check)
                if (chord.type == ChordType::Dominant7 && !isChordInKey(chord, key))
                    return true;

                // Check for Neapolitan (bII major)
                return chord.rootNote % 12 == expectedNeapolitan &&
                       chord.type == ChordType::Major;
            });
    }
    
    inline std::vector<std::string> BasicAdvancedHarmonyEngine::identifyTechniques(const Chord& chord, const Key& key)
    {
        std::vector<std::string> techniques;
        
        if (!chord.isValid() || !key.isValid())
        {
            return techniques;
        }
        
        // Check for borrowed chord
        if (isBorrowedChord(chord, key))
        {
            auto borrowed = analyzeBorrowedChord(chord, key);
            techniques.push_back("Borrowed chord (" + borrowed.symbol + ")");
        }
        
        // Check for secondary dominant
        if (chord.type == ChordType::Dominant7 && !isChordInKey(chord, key))
        {
            techniques.push_back("Secondary dominant");
        }
        
        // Check for Neapolitan
        int chordRoot = chord.rootNote % 12;
        int expectedNeapolitan = (key.tonicNote + 1) % 12;
        if (chordRoot == expectedNeapolitan && chord.type == ChordType::Major)
        {
            techniques.push_back("Neapolitan chord (bII)");
        }
        
        // Check for chromatic mediants
        auto mediants = getChromaticMediants(key);
        auto mediantIt = std::find_if(mediants.begin(), mediants.end(),
            [&](const ChromaticMediant& mediant) {
                return mediant.chord.rootNote % 12 == chordRoot &&
                       mediant.chord.type == chord.type;
            });

        if (mediantIt != mediants.end())
        {
            techniques.push_back("Chromatic mediant (" + mediantIt->symbol + ")");
        }
        
        return techniques;
    }
    
} // namespace HarmonyEngine
