/*
  ==============================================================================

    ReharmonizationEngine.h
    Created: Reharmonization preset system foundation
    
    This file contains the ReharmonizationEngine interface and preset data 
    structures for applying artist-specific and genre-specific reharmonization
    techniques to chord progressions.

  ==============================================================================
*/

#pragma once

// Implementation includes (header-only)
#include <algorithm>
#include <random>
#include <cmath>

#include "MusicTheory.h"
#include "MusicTheoryEngine.h"
#include <map>
#include <memory>
#include <functional>

namespace HarmonyEngine
{
    //==============================================================================
    // Chord Substitution Types
    
    enum class SubstitutionType
    {
        Direct,           // Direct chord replacement (Dm -> D7)
        Tritone,          // Tritone substitution (G7 -> Db7)
        Chromatic,        // Chromatic approach chords
        Modal,            // Modal interchange (borrowed chords)
        Secondary,        // Secondary dominants (V/V, V/vi, etc.)
        Diminished,       // Diminished passing chords
        Augmented,        // Augmented sixth chords
        Neapolitan,       // Neapolitan sixth chords
        Quartal,          // Quartal harmony voicings
        Extended          // Extended harmony (9ths, 11ths, 13ths)
    };
    
    //==============================================================================
    // Harmonic Context Analysis
    
    struct HarmonicContext
    {
        Key key;
        ChordFunction previousFunction;
        ChordFunction currentFunction;
        ChordFunction nextFunction;
        int measurePosition;        // Position within measure (1-4 for 4/4)
        int phrasePosition;         // Position within phrase (1-8 for 8-bar phrase)
        bool isPhraseBoundary;      // True if at phrase start/end
        bool isCadentialPoint;      // True if approaching cadence
        float tensionLevel;         // Current harmonic tension (0.0-1.0)
        
        HarmonicContext()
            : previousFunction(ChordFunction::Tonic)
            , currentFunction(ChordFunction::Tonic)
            , nextFunction(ChordFunction::Tonic)
            , measurePosition(1)
            , phrasePosition(1)
            , isPhraseBoundary(false)
            , isCadentialPoint(false)
            , tensionLevel(0.0f)
        {}
        
        bool isValid() const
        {
            return key.isValid() && 
                   measurePosition >= 1 && measurePosition <= 4 &&
                   phrasePosition >= 1 && phrasePosition <= 8 &&
                   tensionLevel >= 0.0f && tensionLevel <= 1.0f;
        }
    };
    
    //==============================================================================
    // Chord Substitution Definition
    
    struct ChordSubstitution
    {
        SubstitutionType type;
        Chord originalChord;
        Chord substitutionChord;
        float probability;          // Likelihood of applying (0.0-1.0)
        float complexityLevel;      // Harmonic complexity (0.0-1.0)
        std::string description;   // Human-readable description
        
        // Context requirements
        std::vector<ChordFunction> requiredPreviousFunctions;
        std::vector<ChordFunction> requiredNextFunctions;
        bool requiresCadentialContext;
        float minTensionLevel;
        float maxTensionLevel;
        
        ChordSubstitution()
            : type(SubstitutionType::Direct)
            , probability(1.0f)
            , complexityLevel(0.5f)
            , requiresCadentialContext(false)
            , minTensionLevel(0.0f)
            , maxTensionLevel(1.0f)
        {}
        
        bool isApplicableInContext(const HarmonicContext& context) const
        {
            // Check tension level requirements
            if (context.tensionLevel < minTensionLevel || context.tensionLevel > maxTensionLevel)
                return false;
                
            // Check cadential context requirement
            if (requiresCadentialContext && !context.isCadentialPoint)
                return false;
                
            // Check previous function requirements
            if (!requiredPreviousFunctions.empty() &&
                std::find(requiredPreviousFunctions.begin(), requiredPreviousFunctions.end(),
                          context.previousFunction) == requiredPreviousFunctions.end())
                return false;

            // Check next function requirements
            if (!requiredNextFunctions.empty() &&
                std::find(requiredNextFunctions.begin(), requiredNextFunctions.end(),
                          context.nextFunction) == requiredNextFunctions.end())
                return false;

            return true;
        }
    };
    
    //==============================================================================
    // Reharmonization Preset
    
    enum class PresetCategory
    {
        Jazz,
        Pop,
        Classical,
        User
    };
    
    struct ReharmonizationPreset
    {
        std::string name;
        std::string artist;        // Artist name (e.g., "McCoy Tyner", "Count Basie")
        PresetCategory category;
        float complexityLevel;      // Overall complexity (0.0-1.0)
        std::string description;
        
        // Substitution rules organized by chord function
        std::map<ChordFunction, std::vector<ChordSubstitution>> substitutions;
        
        // Global preset settings
        float substitutionProbability;  // Global probability modifier
        bool preserveOriginalBass;      // Keep original bass notes
        bool maintainHarmonicRhythm;    // Keep original chord timing
        int maxSimultaneousSubstitutions; // Limit complexity
        
        ReharmonizationPreset()
            : category(PresetCategory::User)
            , complexityLevel(0.5f)
            , substitutionProbability(0.7f)
            , preserveOriginalBass(false)
            , maintainHarmonicRhythm(true)
            , maxSimultaneousSubstitutions(2)
        {}
        
        bool isValid() const
        {
            return !name.empty() &&
                   complexityLevel >= 0.0f && complexityLevel <= 1.0f &&
                   substitutionProbability >= 0.0f && substitutionProbability <= 1.0f &&
                   maxSimultaneousSubstitutions >= 0;
        }
        
        std::vector<ChordSubstitution> getSubstitutionsForFunction(ChordFunction function) const
        {
            auto it = substitutions.find(function);
            if (it != substitutions.end())
                return it->second;
            return {};
        }
        
        void addSubstitution(ChordFunction function, const ChordSubstitution& substitution)
        {
            substitutions[function].push_back(substitution);
        }
    };
    
    //==============================================================================
    // Reharmonization Result
    
    struct ReharmonizationOption
    {
        Chord originalChord;
        Chord reharmonizedChord;
        SubstitutionType substitutionType;
        float confidenceScore;      // How well it fits (0.0-1.0)
        std::string explanation;   // Why this substitution works
        
        ReharmonizationOption()
            : substitutionType(SubstitutionType::Direct)
            , confidenceScore(0.0f)
        {}
    };
    
    struct ReharmonizationResult
    {
        std::vector<Chord> originalProgression;
        std::vector<Chord> reharmonizedProgression;
        std::vector<ReharmonizationOption> appliedSubstitutions;
        ReharmonizationPreset usedPreset;
        float overallQualityScore;  // Overall reharmonization quality (0.0-1.0)
        std::vector<std::string> notes; // Additional notes about the reharmonization
        
        ReharmonizationResult()
            : overallQualityScore(0.0f)
        {}
        
        bool isValid() const
        {
            return originalProgression.size() == reharmonizedProgression.size() &&
                   !originalProgression.empty() &&
                   overallQualityScore >= 0.0f && overallQualityScore <= 1.0f;
        }
    };
    
    //==============================================================================
    // ReharmonizationEngine Interface
    
    class ReharmonizationEngine
    {
    public:
        virtual ~ReharmonizationEngine() = default;
        
        // Core reharmonization functionality
        virtual ReharmonizationResult applyPreset(
            const std::vector<Chord>& originalProgression,
            const ReharmonizationPreset& preset,
            const Key& key
        ) = 0;
        
        virtual std::vector<ReharmonizationOption> suggestAlternatives(
            const Chord& chord,
            const HarmonicContext& context
        ) = 0;
        
        virtual HarmonicContext analyzeHarmonicContext(
            const std::vector<Chord>& progression,
            int chordIndex,
            const Key& key
        ) = 0;
        
        // Preset management
        virtual std::vector<ReharmonizationPreset> getAvailablePresets() = 0;
        virtual ReharmonizationPreset getPresetByName(const std::string& name) = 0;
        virtual bool loadPreset(const ReharmonizationPreset& preset) = 0;
        virtual bool savePreset(const ReharmonizationPreset& preset) = 0;
        
        // Learning functionality
        virtual ReharmonizationPreset learnFromProgression(
            const std::vector<Chord>& originalProgression,
            const std::vector<Chord>& reharmonizedProgression,
            const std::string& styleName,
            const Key& key
        ) = 0;
        
        // Utility functions
        virtual float calculateSubstitutionQuality(
            const Chord& original,
            const Chord& substitution,
            const HarmonicContext& context
        ) = 0;
        
        virtual bool isValidSubstitution(
            const Chord& original,
            const Chord& substitution,
            const HarmonicContext& context
        ) = 0;
    };
    
    //==============================================================================
    // Factory Function
    
    inline std::unique_ptr<ReharmonizationEngine> createReharmonizationEngine();
    
} // namespace HarmonyEngine

// ============================================================================
// Implementation (merged from .cpp - header-only library)
// ============================================================================

namespace HarmonyEngine
{
    //==============================================================================
    // Basic Implementation
    
    class BasicReharmonizationEngine : public ReharmonizationEngine
    {
    public:
        BasicReharmonizationEngine();
        ~BasicReharmonizationEngine() override = default;
        
        // ReharmonizationEngine interface implementation
        ReharmonizationResult applyPreset(
            const std::vector<Chord>& originalProgression,
            const ReharmonizationPreset& preset,
            const Key& key
        ) override;
        
        std::vector<ReharmonizationOption> suggestAlternatives(
            const Chord& chord,
            const HarmonicContext& context
        ) override;
        
        HarmonicContext analyzeHarmonicContext(
            const std::vector<Chord>& progression,
            int chordIndex,
            const Key& key
        ) override;
        
        std::vector<ReharmonizationPreset> getAvailablePresets() override;
        ReharmonizationPreset getPresetByName(const std::string& name) override;
        bool loadPreset(const ReharmonizationPreset& preset) override;
        bool savePreset(const ReharmonizationPreset& preset) override;
        
        ReharmonizationPreset learnFromProgression(
            const std::vector<Chord>& originalProgression,
            const std::vector<Chord>& reharmonizedProgression,
            const std::string& styleName,
            const Key& key
        ) override;
        
        float calculateSubstitutionQuality(
            const Chord& original,
            const Chord& substitution,
            const HarmonicContext& context
        ) override;
        
        bool isValidSubstitution(
            const Chord& original,
            const Chord& substitution,
            const HarmonicContext& context
        ) override;
        
    private:
        std::vector<ReharmonizationPreset> presets;
        std::unique_ptr<MusicTheoryEngine> theoryEngine;
        std::mt19937 randomGenerator;
        
        // Chord substitution algorithms
        std::vector<Chord> generateTritoneSubstitutions(const Chord& chord, const Key& key);
        std::vector<Chord> generateModalInterchangeSubstitutions(const Chord& chord, const Key& key);
        std::vector<Chord> generateSecondaryDominants(const Chord& chord, const Key& key);
        std::vector<Chord> generateDiminishedPassingChords(const Chord& chord, const Key& key);
        std::vector<Chord> generateQuartalHarmonySubstitutions(const Chord& chord, const Key& key);
        std::vector<Chord> generateExtendedHarmonySubstitutions(const Chord& chord, const Key& key);
        
        // Context analysis helpers
        ChordFunction analyzeChordFunction(const Chord& chord, const Key& key);
        float calculateTensionLevel(const Chord& chord, const HarmonicContext& context);
        bool isCadentialContext(const std::vector<Chord>& progression, int chordIndex, const Key& key);
        
        // Preset creation helpers
        void initializeBuiltInPresets();
        ReharmonizationPreset createJazzPreset(const std::string& artistName);
        ReharmonizationPreset createPopPreset();
        ReharmonizationPreset createClassicalPreset();
        
        // Substitution quality evaluation
        float evaluateVoiceLeading(const Chord& from, const Chord& to);
        float evaluateHarmonicFunction(const Chord& chord, const HarmonicContext& context);
        float evaluateComplexity(const Chord& chord, float targetComplexity);
        
        // Utility functions
        Chord transposeChord(const Chord& chord, int semitones);
        std::vector<int> getCommonTones(const Chord& chord1, const Chord& chord2);
        bool sharesCommonTones(const Chord& chord1, const Chord& chord2);
        int calculateInterval(int note1, int note2);
    };
    
    //==============================================================================
    // BasicReharmonizationEngine Implementation
    
  inline   BasicReharmonizationEngine::BasicReharmonizationEngine()
        : randomGenerator(std::random_device{}())
    {
        theoryEngine = std::make_unique<BasicMusicTheoryEngine>();
        initializeBuiltInPresets();
    }
    
    inline ReharmonizationResult BasicReharmonizationEngine::applyPreset(
        const std::vector<Chord>& originalProgression,
        const ReharmonizationPreset& preset,
        const Key& key)
    {
        ReharmonizationResult result;
        result.originalProgression = originalProgression;
        result.usedPreset = preset;
        result.reharmonizedProgression = originalProgression; // Start with original
        
        if (originalProgression.empty() || !preset.isValid() || !key.isValid())
        {
            result.overallQualityScore = 0.0f;
            result.notes.push_back("Invalid input parameters");
            return result;
        }
        
        int substitutionsApplied = 0;
        float totalQualityScore = 0.0f;
        
        // Process each chord in the progression
        for (size_t i = 0; i < originalProgression.size(); ++i)
        {
            if (substitutionsApplied >= preset.maxSimultaneousSubstitutions)
                break;
                
            const Chord& originalChord = originalProgression[i];
            HarmonicContext context = analyzeHarmonicContext(originalProgression, static_cast<int>(i), key);
            
            // Get chord function and find applicable substitutions
            ChordFunction function = analyzeChordFunction(originalChord, key);
            auto substitutions = preset.getSubstitutionsForFunction(function);
            
            // Find best applicable substitution
            ChordSubstitution bestSubstitution;
            float bestScore = 0.0f;
            bool foundSubstitution = false;
            
            for (const auto& substitution : substitutions)
            {
                if (!substitution.isApplicableInContext(context))
                    continue;
                    
                // Calculate probability of applying this substitution
                float probability = substitution.probability * preset.substitutionProbability;
                std::uniform_real_distribution<float> dist(0.0f, 1.0f);
                
                if (dist(randomGenerator) > probability)
                    continue;
                    
                // Calculate quality score for this substitution
                float quality = calculateSubstitutionQuality(originalChord, substitution.substitutionChord, context);
                
                if (quality > bestScore)
                {
                    bestScore = quality;
                    bestSubstitution = substitution;
                    foundSubstitution = true;
                }
            }
            
            // Apply the best substitution if found
            if (foundSubstitution && bestScore > 0.5f)
            {
                result.reharmonizedProgression[i] = bestSubstitution.substitutionChord;
                
                ReharmonizationOption option;
                option.originalChord = originalChord;
                option.reharmonizedChord = bestSubstitution.substitutionChord;
                option.substitutionType = bestSubstitution.type;
                option.confidenceScore = bestScore;
                option.explanation = bestSubstitution.description;
                
                result.appliedSubstitutions.push_back(option);
                substitutionsApplied++;
                totalQualityScore += bestScore;
            }
            else
            {
                totalQualityScore += 0.7f; // Score for keeping original chord
            }
        }
        
        // Calculate overall quality score
        result.overallQualityScore = totalQualityScore / static_cast<float>(originalProgression.size());
        
        // Add notes about the reharmonization
        if (substitutionsApplied > 0)
        {
            result.notes.push_back("Applied " + std::to_string(substitutionsApplied) + 
                                 " substitutions using " + preset.name + " style");
        }
        else
        {
            result.notes.push_back("No suitable substitutions found for this progression");
        }
        
        return result;
    }
    
    inline std::vector<ReharmonizationOption> BasicReharmonizationEngine::suggestAlternatives(
        const Chord& chord,
        const HarmonicContext& context)
    {
        std::vector<ReharmonizationOption> options;
        
        if (!chord.isValid() || !context.isValid())
            return options;
        
        // Generate different types of substitutions
        auto tritoneSubstitutions = generateTritoneSubstitutions(chord, context.key);
        auto modalSubstitutions = generateModalInterchangeSubstitutions(chord, context.key);
        auto secondaryDominants = generateSecondaryDominants(chord, context.key);
        auto extendedHarmony = generateExtendedHarmonySubstitutions(chord, context.key);
        
        // Convert to ReharmonizationOptions with quality scores
        auto addOptions = [&](const std::vector<Chord>& substitutions, SubstitutionType type, const std::string& description)
        {
            for (const auto& substitution : substitutions)
            {
                if (isValidSubstitution(chord, substitution, context))
                {
                    ReharmonizationOption option;
                    option.originalChord = chord;
                    option.reharmonizedChord = substitution;
                    option.substitutionType = type;
                    option.confidenceScore = calculateSubstitutionQuality(chord, substitution, context);
                    option.explanation = description + " - " + substitution.getChordSymbol();
                    
                    if (option.confidenceScore > 0.3f) // Only include reasonable options
                        options.push_back(option);
                }
            }
        };
        
        addOptions(tritoneSubstitutions, SubstitutionType::Tritone, "Tritone substitution");
        addOptions(modalSubstitutions, SubstitutionType::Modal, "Modal interchange");
        addOptions(secondaryDominants, SubstitutionType::Secondary, "Secondary dominant");
        addOptions(extendedHarmony, SubstitutionType::Extended, "Extended harmony");
        
        // Sort by confidence score (highest first)
        std::sort(options.begin(), options.end(), 
                 [](const ReharmonizationOption& a, const ReharmonizationOption& b)
                 {
                     return a.confidenceScore > b.confidenceScore;
                 });
        
        // Limit to top 6 options
        if (options.size() > 6)
            options.resize(6);
        
        return options;
    }
    
    inline HarmonicContext BasicReharmonizationEngine::analyzeHarmonicContext(
        const std::vector<Chord>& progression,
        int chordIndex,
        const Key& key)
    {
        HarmonicContext context;
        context.key = key;
        
        if (chordIndex < 0 || chordIndex >= static_cast<int>(progression.size()) || !key.isValid())
            return context;
        
        // Analyze current chord function
        context.currentFunction = analyzeChordFunction(progression[chordIndex], key);
        
        // Analyze previous chord function
        if (chordIndex > 0)
        {
            context.previousFunction = analyzeChordFunction(progression[chordIndex - 1], key);
        }
        
        // Analyze next chord function
        if (chordIndex < static_cast<int>(progression.size()) - 1)
        {
            context.nextFunction = analyzeChordFunction(progression[chordIndex + 1], key);
        }
        
        // Calculate position information (assuming 4/4 time, 4 chords per measure)
        context.measurePosition = (chordIndex % 4) + 1;
        context.phrasePosition = (chordIndex % 8) + 1;
        context.isPhraseBoundary = (chordIndex % 8 == 0) || (chordIndex % 8 == 7);
        context.isCadentialPoint = isCadentialContext(progression, chordIndex, key);
        
        // Calculate tension level
        context.tensionLevel = calculateTensionLevel(progression[chordIndex], context);
        
        return context;
    }
    
    inline std::vector<ReharmonizationPreset> BasicReharmonizationEngine::getAvailablePresets()
    {
        return presets;
    }
    
    inline ReharmonizationPreset BasicReharmonizationEngine::getPresetByName(const std::string& name)
    {
        auto it = std::find_if(presets.begin(), presets.end(),
                               [&name](const ReharmonizationPreset& p) { return p.name == name; });
        return (it != presets.end()) ? *it : ReharmonizationPreset();
    }
    
    inline bool BasicReharmonizationEngine::loadPreset(const ReharmonizationPreset& preset)
    {
        if (!preset.isValid())
            return false;

        auto it = std::find_if(presets.begin(), presets.end(),
                               [&preset](const ReharmonizationPreset& p) { return p.name == preset.name; });
        if (it != presets.end())
        {
            *it = preset;
            return true;
        }

        presets.push_back(preset);
        return true;
    }
    
    inline bool BasicReharmonizationEngine::savePreset(const ReharmonizationPreset& preset)
    {
        // For now, just store in memory (could be extended to file I/O)
        return loadPreset(preset);
    }
    
    inline ReharmonizationPreset BasicReharmonizationEngine::learnFromProgression(
        const std::vector<Chord>& originalProgression,
        const std::vector<Chord>& reharmonizedProgression,
        const std::string& styleName,
        const Key& key)
    {
        ReharmonizationPreset learnedPreset;
        learnedPreset.name = styleName;
        learnedPreset.category = PresetCategory::User;
        learnedPreset.description = "Learned from user input";
        
        if (originalProgression.size() != reharmonizedProgression.size() || 
            originalProgression.empty() || !key.isValid())
        {
            return learnedPreset;
        }
        
        // Analyze differences between original and reharmonized progressions
        for (size_t i = 0; i < originalProgression.size(); ++i)
        {
            const Chord& original = originalProgression[i];
            const Chord& reharmonized = reharmonizedProgression[i];
            
            // Skip if chords are the same
            if (original.rootNote == reharmonized.rootNote && original.type == reharmonized.type)
                continue;
            
            // Analyze the substitution
            ChordFunction function = analyzeChordFunction(original, key);
            HarmonicContext context = analyzeHarmonicContext(originalProgression, static_cast<int>(i), key);
            
            ChordSubstitution substitution;
            substitution.originalChord = original;
            substitution.substitutionChord = reharmonized;
            substitution.probability = 0.8f; // High probability for learned patterns
            substitution.complexityLevel = 0.6f;
            substitution.description = "Learned substitution: " + original.getChordSymbol() + 
                                     " -> " + reharmonized.getChordSymbol();
            
            // Determine substitution type based on analysis
            if (calculateInterval(original.rootNote, reharmonized.rootNote) == 6) // Tritone
            {
                substitution.type = SubstitutionType::Tritone;
            }
            else if (sharesCommonTones(original, reharmonized))
            {
                substitution.type = SubstitutionType::Modal;
            }
            else
            {
                substitution.type = SubstitutionType::Direct;
            }
            
            learnedPreset.addSubstitution(function, substitution);
        }
        
        // Calculate overall complexity
        float totalComplexity = 0.0f;
        int substitutionCount = 0;
        for (const auto& pair : learnedPreset.substitutions)
        {
            for (const auto& sub : pair.second)
            {
                totalComplexity += sub.complexityLevel;
                substitutionCount++;
            }
        }
        
        if (substitutionCount > 0)
        {
            learnedPreset.complexityLevel = totalComplexity / static_cast<float>(substitutionCount);
        }
        
        return learnedPreset;
    }
    
    inline float BasicReharmonizationEngine::calculateSubstitutionQuality(
        const Chord& original,
        const Chord& substitution,
        const HarmonicContext& context)
    {
        if (!original.isValid() || !substitution.isValid() || !context.isValid())
            return 0.0f;
        
        float score = 0.0f;
        
        // Voice leading quality (30% of score)
        score += evaluateVoiceLeading(original, substitution) * 0.3f;
        
        // Harmonic function appropriateness (40% of score)
        score += evaluateHarmonicFunction(substitution, context) * 0.4f;
        
        // Complexity appropriateness (20% of score)
        score += evaluateComplexity(substitution, context.tensionLevel) * 0.2f;
        
        // Common tone retention (10% of score)
        if (sharesCommonTones(original, substitution))
        {
            score += 0.1f;
        }
        
        return std::min(1.0f, score);
    }
    
    inline bool BasicReharmonizationEngine::isValidSubstitution(
        const Chord& original,
        const Chord& substitution,
        const HarmonicContext& context)
    {
        if (!original.isValid() || !substitution.isValid() || !context.isValid())
            return false;
        
        // Basic validity check
        float quality = calculateSubstitutionQuality(original, substitution, context);
        return quality > 0.3f; // Minimum quality threshold
    }
    
    //==============================================================================
    // Private Helper Methods
    
    inline std::vector<Chord> BasicReharmonizationEngine::generateTritoneSubstitutions(const Chord& chord, const Key& key)
    {
        std::vector<Chord> substitutions;
        
        // Only apply tritone substitution to dominant chords
        if (chord.type != ChordType::Dominant7)
            return substitutions;
        
        // Create tritone substitution (root + 6 semitones)
        int newRoot = (chord.rootNote + 6) % 12;
        Chord tritoneSubstitution(ChordType::Dominant7, newRoot + 60); // Place in 4th octave
        
        // Build the chord notes
        tritoneSubstitution.notes = {
            Note(newRoot + 60, 1.0f, 0.8f, 0.0f),      // Root
            Note(newRoot + 64, 1.0f, 0.8f, 0.0f),      // Major third
            Note(newRoot + 70, 1.0f, 0.8f, 0.0f),      // Minor seventh
            Note(newRoot + 66, 1.0f, 0.8f, 0.0f)       // Tritone (b5)
        };
        
        substitutions.push_back(tritoneSubstitution);
        return substitutions;
    }
    
    inline std::vector<Chord> BasicReharmonizationEngine::generateModalInterchangeSubstitutions(const Chord& chord, const Key& key)
    {
        std::vector<Chord> substitutions;
        
        // Borrow chords from the parallel mode (major<->minor).
        // NaturalMinor and Aeolian are equivalent.
        ScaleType parallelType;
        if (key.scaleType == ScaleType::Major)
            parallelType = ScaleType::NaturalMinor;
        else if (key.scaleType == ScaleType::NaturalMinor || key.scaleType == ScaleType::Aeolian)
            parallelType = ScaleType::Major;
        else
            return substitutions;

        Key parallelKey(key.tonicNote, parallelType);
        auto parallelChords = theoryEngine->generateDiatonicChords(parallelKey);

        // Find chords with same root as original
        for (const auto& parallelChord : parallelChords.chords)
        {
            if (parallelChord.rootNote % 12 == chord.rootNote % 12)
            {
                substitutions.push_back(parallelChord);
            }
        }

        return substitutions;
    }
    
    inline std::vector<Chord> BasicReharmonizationEngine::generateSecondaryDominants(const Chord& chord, const Key& key)
    {
        std::vector<Chord> substitutions;
        
        // Generate V/V, V/vi, V/IV, etc.
        auto diatonicChords = theoryEngine->generateDiatonicChords(key);
        
        for (const auto& targetChord : diatonicChords.chords)
        {
            // Create dominant of this chord
            int dominantRoot = (targetChord.rootNote + 7) % 12; // Perfect fifth above
            Chord secondaryDominant(ChordType::Dominant7, dominantRoot + 60);
            
            // Build the dominant seventh chord
            secondaryDominant.notes = {
                Note(dominantRoot + 60, 1.0f, 0.8f, 0.0f),      // Root
                Note(dominantRoot + 64, 1.0f, 0.8f, 0.0f),      // Major third
                Note(dominantRoot + 67, 1.0f, 0.8f, 0.0f),      // Perfect fifth
                Note(dominantRoot + 70, 1.0f, 0.8f, 0.0f)       // Minor seventh
            };
            
            substitutions.push_back(secondaryDominant);
        }
        
        return substitutions;
    }
    
    inline std::vector<Chord> BasicReharmonizationEngine::generateDiminishedPassingChords(const Chord& chord, const Key& key)
    {
        std::vector<Chord> substitutions;
        
        // Generate diminished chords between chord tones
        for (int semitone = 1; semitone <= 11; ++semitone)
        {
            int dimRoot = (chord.rootNote + semitone) % 12;
            Chord diminishedChord(ChordType::Diminished7, dimRoot + 60);
            
            // Build diminished seventh chord (minor thirds stacked)
            diminishedChord.notes = {
                Note(dimRoot + 60, 1.0f, 0.8f, 0.0f),      // Root
                Note(dimRoot + 63, 1.0f, 0.8f, 0.0f),      // Minor third
                Note(dimRoot + 66, 1.0f, 0.8f, 0.0f),      // Diminished fifth
                Note(dimRoot + 69, 1.0f, 0.8f, 0.0f)       // Diminished seventh
            };
            
            substitutions.push_back(diminishedChord);
        }
        
        return substitutions;
    }
    
    inline std::vector<Chord> BasicReharmonizationEngine::generateQuartalHarmonySubstitutions(const Chord& chord, const Key& key)
    {
        std::vector<Chord> substitutions;
        
        // Generate quartal harmony (stacked fourths) based on chord root
        Chord quartalChord(ChordType::Sus4, chord.rootNote);
        
        // Build quartal harmony (perfect fourths stacked)
        quartalChord.notes = {
            Note(chord.rootNote, 1.0f, 0.8f, 0.0f),           // Root
            Note(chord.rootNote + 5, 1.0f, 0.8f, 0.0f),       // Perfect fourth
            Note(chord.rootNote + 10, 1.0f, 0.8f, 0.0f),      // Minor seventh (another fourth)
            Note(chord.rootNote + 15, 1.0f, 0.8f, 0.0f)       // Major third of next octave
        };
        
        substitutions.push_back(quartalChord);
        return substitutions;
    }
    
    inline std::vector<Chord> BasicReharmonizationEngine::generateExtendedHarmonySubstitutions(const Chord& chord, const Key& key)
    {
        std::vector<Chord> substitutions;
        
        // Generate extended versions of the chord (add9, add11, add13)
        if (chord.type == ChordType::Major)
        {
            Chord add9Chord(ChordType::Add9, chord.rootNote);
            add9Chord.notes = chord.notes;
            add9Chord.notes.push_back(Note(chord.rootNote + 14, 1.0f, 0.8f, 0.0f)); // Add 9th
            substitutions.push_back(add9Chord);
            
            Chord maj9Chord(ChordType::Major9, chord.rootNote);
            maj9Chord.notes = chord.notes;
            maj9Chord.notes.push_back(Note(chord.rootNote + 11, 1.0f, 0.8f, 0.0f)); // Add 7th
            maj9Chord.notes.push_back(Note(chord.rootNote + 14, 1.0f, 0.8f, 0.0f)); // Add 9th
            substitutions.push_back(maj9Chord);
        }
        else if (chord.type == ChordType::Minor)
        {
            Chord min9Chord(ChordType::Minor9, chord.rootNote);
            min9Chord.notes = chord.notes;
            min9Chord.notes.push_back(Note(chord.rootNote + 10, 1.0f, 0.8f, 0.0f)); // Add minor 7th
            min9Chord.notes.push_back(Note(chord.rootNote + 14, 1.0f, 0.8f, 0.0f)); // Add 9th
            substitutions.push_back(min9Chord);
        }
        
        return substitutions;
    }
    
    inline ChordFunction BasicReharmonizationEngine::analyzeChordFunction(const Chord& chord, const Key& key)
    {
        return theoryEngine->identifyFunction(chord, key);
    }
    
    inline float BasicReharmonizationEngine::calculateTensionLevel(const Chord& chord, const HarmonicContext& context)
    {
        float tension = 0.0f;
        
        // Base tension on chord type
        switch (chord.type)
        {
            case ChordType::Major:
            case ChordType::Minor:
                tension += 0.2f;
                break;
            case ChordType::Dominant7:
                tension += 0.7f;
                break;
            case ChordType::Diminished:
            case ChordType::Diminished7:
                tension += 0.9f;
                break;
            case ChordType::HalfDiminished7:
                tension += 0.6f;
                break;
            default:
                tension += 0.4f;
                break;
        }
        
        // Add tension based on harmonic function
        switch (context.currentFunction)
        {
            case ChordFunction::Dominant:
                tension += 0.3f;
                break;
            case ChordFunction::LeadingTone:
                tension += 0.4f;
                break;
            case ChordFunction::Secondary:
                tension += 0.2f;
                break;
            default:
                break;
        }
        
        // Add tension based on position in phrase
        if (context.measurePosition == 3 || context.measurePosition == 4)
        {
            tension += 0.1f; // Slightly more tension on beats 3 and 4
        }
        
        return std::min(1.0f, tension);
    }
    
    inline bool BasicReharmonizationEngine::isCadentialContext(const std::vector<Chord>& progression, int chordIndex, const Key& key)
    {
        // Check if we're approaching a cadence (V-I or similar)
        if (chordIndex >= static_cast<int>(progression.size()) - 1)
            return false;
        
        ChordFunction currentFunction = analyzeChordFunction(progression[chordIndex], key);
        ChordFunction nextFunction = analyzeChordFunction(progression[chordIndex + 1], key);
        
        // Common cadential progressions
        return (currentFunction == ChordFunction::Dominant && nextFunction == ChordFunction::Tonic) ||
               (currentFunction == ChordFunction::Subdominant && nextFunction == ChordFunction::Tonic) ||
               (currentFunction == ChordFunction::LeadingTone && nextFunction == ChordFunction::Tonic);
    }
    
    inline void BasicReharmonizationEngine::initializeBuiltInPresets()
    {
        // Create basic jazz preset
        auto jazzPreset = createJazzPreset("Basic Jazz");
        presets.push_back(jazzPreset);
        
        // Create basic pop preset
        auto popPreset = createPopPreset();
        presets.push_back(popPreset);
        
        // Create basic classical preset
        auto classicalPreset = createClassicalPreset();
        presets.push_back(classicalPreset);
    }
    
    inline ReharmonizationPreset BasicReharmonizationEngine::createJazzPreset(const std::string& artistName)
    {
        ReharmonizationPreset preset;
        preset.name = artistName;
        preset.category = PresetCategory::Jazz;
        preset.complexityLevel = 0.7f;
        preset.description = "Jazz reharmonization with tritone substitutions and extended harmony";
        preset.substitutionProbability = 0.6f;
        
        // Add tritone substitutions for dominant chords
        ChordSubstitution tritoneSubstitution;
        tritoneSubstitution.type = SubstitutionType::Tritone;
        tritoneSubstitution.probability = 0.8f;
        tritoneSubstitution.complexityLevel = 0.7f;
        tritoneSubstitution.description = "Jazz tritone substitution";
        tritoneSubstitution.requiresCadentialContext = true;
        preset.addSubstitution(ChordFunction::Dominant, tritoneSubstitution);
        
        // Add extended harmony substitutions
        ChordSubstitution extendedHarmony;
        extendedHarmony.type = SubstitutionType::Extended;
        extendedHarmony.probability = 0.6f;
        extendedHarmony.complexityLevel = 0.6f;
        extendedHarmony.description = "Extended jazz harmony (9ths, 11ths, 13ths)";
        preset.addSubstitution(ChordFunction::Tonic, extendedHarmony);
        preset.addSubstitution(ChordFunction::Subdominant, extendedHarmony);
        
        // Add secondary dominants
        ChordSubstitution secondaryDominant;
        secondaryDominant.type = SubstitutionType::Secondary;
        secondaryDominant.probability = 0.5f;
        secondaryDominant.complexityLevel = 0.5f;
        secondaryDominant.description = "Secondary dominant approach";
        preset.addSubstitution(ChordFunction::Subdominant, secondaryDominant);
        preset.addSubstitution(ChordFunction::Mediant, secondaryDominant);
        
        return preset;
    }
    
    inline ReharmonizationPreset BasicReharmonizationEngine::createPopPreset()
    {
        ReharmonizationPreset preset;
        preset.name = "Contemporary Pop";
        preset.category = PresetCategory::Pop;
        preset.complexityLevel = 0.4f;
        preset.description = "Modern pop harmony with modal interchange";
        preset.substitutionProbability = 0.4f;
        preset.preserveOriginalBass = true;
        
        // Add modal interchange substitutions
        ChordSubstitution modalInterchange;
        modalInterchange.type = SubstitutionType::Modal;
        modalInterchange.probability = 0.7f;
        modalInterchange.complexityLevel = 0.4f;
        modalInterchange.description = "Modal interchange from parallel minor/major";
        preset.addSubstitution(ChordFunction::Subdominant, modalInterchange);
        preset.addSubstitution(ChordFunction::Submediant, modalInterchange);
        
        // Add simple extended chords
        ChordSubstitution simpleExtensions;
        simpleExtensions.type = SubstitutionType::Extended;
        simpleExtensions.probability = 0.5f;
        simpleExtensions.complexityLevel = 0.3f;
        simpleExtensions.description = "Simple extensions (add9, sus4)";
        preset.addSubstitution(ChordFunction::Tonic, simpleExtensions);
        
        return preset;
    }
    
    inline ReharmonizationPreset BasicReharmonizationEngine::createClassicalPreset()
    {
        ReharmonizationPreset preset;
        preset.name = "Classical";
        preset.category = PresetCategory::Classical;
        preset.complexityLevel = 0.3f;
        preset.description = "Traditional classical harmony with proper voice leading";
        preset.substitutionProbability = 0.3f;
        preset.preserveOriginalBass = true;
        preset.maintainHarmonicRhythm = true;
        
        // Add Neapolitan sixth chords
        ChordSubstitution neapolitan;
        neapolitan.type = SubstitutionType::Neapolitan;
        neapolitan.probability = 0.4f;
        neapolitan.complexityLevel = 0.5f;
        neapolitan.description = "Neapolitan sixth chord";
        neapolitan.requiresCadentialContext = true;
        neapolitan.requiredNextFunctions = {ChordFunction::Dominant};
        preset.addSubstitution(ChordFunction::Subdominant, neapolitan);
        
        // Add secondary dominants (classical style)
        ChordSubstitution classicalSecondary;
        classicalSecondary.type = SubstitutionType::Secondary;
        classicalSecondary.probability = 0.6f;
        classicalSecondary.complexityLevel = 0.4f;
        classicalSecondary.description = "Classical secondary dominant";
        preset.addSubstitution(ChordFunction::Mediant, classicalSecondary);
        preset.addSubstitution(ChordFunction::Submediant, classicalSecondary);
        
        return preset;
    }
    
    //==============================================================================
    // Quality Evaluation Methods
    
    inline float BasicReharmonizationEngine::evaluateVoiceLeading(const Chord& from, const Chord& to)
    {
        if (from.notes.empty() || to.notes.empty())
            return 0.0f;
        
        float score = 1.0f;
        float totalMotion = 0.0f;
        int voiceCount = 0;
        
        // Calculate voice leading motion for each voice
        size_t minVoices = std::min(from.notes.size(), to.notes.size());
        
        for (size_t i = 0; i < minVoices; ++i)
        {
            int interval = std::abs(to.notes[i].midiNumber - from.notes[i].midiNumber);
            totalMotion += static_cast<float>(interval);
            voiceCount++;
            
            // Penalize large leaps
            if (interval > 7) // More than a perfect fifth
            {
                score -= 0.1f;
            }
            
            // Reward stepwise motion
            if (interval <= 2) // Stepwise or common tone
            {
                score += 0.05f;
            }
        }
        
        // Calculate average motion
        if (voiceCount > 0)
        {
            float averageMotion = totalMotion / static_cast<float>(voiceCount);
            // Prefer smaller average motion
            score -= (averageMotion / 12.0f) * 0.3f; // Normalize to semitones
        }
        
        return std::max(0.0f, std::min(1.0f, score));
    }
    
    inline float BasicReharmonizationEngine::evaluateHarmonicFunction(const Chord& chord, const HarmonicContext& context)
    {
        ChordFunction chordFunction = analyzeChordFunction(chord, context.key);
        float score = 0.5f; // Base score
        
        // Evaluate function appropriateness based on context
        switch (context.currentFunction)
        {
            case ChordFunction::Tonic:
                if (chordFunction == ChordFunction::Tonic || 
                    chordFunction == ChordFunction::Submediant ||
                    chordFunction == ChordFunction::Mediant)
                {
                    score += 0.3f;
                }
                break;
                
            case ChordFunction::Subdominant:
                if (chordFunction == ChordFunction::Subdominant ||
                    chordFunction == ChordFunction::Predominant ||  // pre-existing bug: was ChordFunction::Supertonic (no such enum; II is predominant)
                    chordFunction == ChordFunction::Submediant)
                {
                    score += 0.3f;
                }
                break;
                
            case ChordFunction::Dominant:
                if (chordFunction == ChordFunction::Dominant ||
                    chordFunction == ChordFunction::LeadingTone ||
                    chordFunction == ChordFunction::Secondary)
                {
                    score += 0.3f;
                }
                break;
                
            default:
                score += 0.1f;
                break;
        }
        
        // Bonus for cadential contexts
        if (context.isCadentialPoint && chordFunction == ChordFunction::Dominant)
        {
            score += 0.2f;
        }
        
        return std::min(1.0f, score);
    }
    
    inline float BasicReharmonizationEngine::evaluateComplexity(const Chord& chord, float targetComplexity)
    {
        float chordComplexity = 0.0f;
        
        // Base complexity on chord type
        switch (chord.type)
        {
            case ChordType::Major:
            case ChordType::Minor:
                chordComplexity = 0.1f;
                break;
            case ChordType::Dominant7:
                chordComplexity = 0.4f;
                break;
            case ChordType::Major7:
            case ChordType::Minor7:
                chordComplexity = 0.3f;
                break;
            case ChordType::Diminished7:
            case ChordType::HalfDiminished7:
                chordComplexity = 0.6f;
                break;
            case ChordType::Add9:
            case ChordType::Sus4:
                chordComplexity = 0.5f;
                break;
            case ChordType::Major9:
            case ChordType::Minor9:
                chordComplexity = 0.7f;
                break;
            default:
                chordComplexity = 0.5f;
                break;
        }
        
        // Add complexity based on extensions and alterations
        chordComplexity += static_cast<float>(chord.extensions.size()) * 0.1f;
        
        // Score based on how well complexity matches target
        float complexityDifference = std::abs(chordComplexity - targetComplexity);
        return std::max(0.0f, 1.0f - (complexityDifference * 2.0f));
    }
    
    //==============================================================================
    // Utility Methods
    
    inline Chord BasicReharmonizationEngine::transposeChord(const Chord& chord, int semitones)
    {
        Chord transposed = chord;
        transposed.rootNote = (chord.rootNote + semitones) % 12;
        
        for (auto& note : transposed.notes)
        {
            note.midiNumber += semitones;
            // Keep within reasonable MIDI range
            while (note.midiNumber < 21) note.midiNumber += 12;
            while (note.midiNumber > 108) note.midiNumber -= 12;
        }
        
        return transposed;
    }
    
    inline std::vector<int> BasicReharmonizationEngine::getCommonTones(const Chord& chord1, const Chord& chord2)
    {
        std::vector<int> commonTones;
        
        for (const auto& note1 : chord1.notes)
        {
            int pitchClass1 = note1.midiNumber % 12;
            for (const auto& note2 : chord2.notes)
            {
                int pitchClass2 = note2.midiNumber % 12;
                if (pitchClass1 == pitchClass2)
                {
                    commonTones.push_back(pitchClass1);
                    break; // Avoid duplicates
                }
            }
        }
        
        return commonTones;
    }
    
    inline bool BasicReharmonizationEngine::sharesCommonTones(const Chord& chord1, const Chord& chord2)
    {
        return !getCommonTones(chord1, chord2).empty();
    }
    
    inline int BasicReharmonizationEngine::calculateInterval(int note1, int note2)
    {
        int interval = std::abs(note2 - note1) % 12;
        return std::min(interval, 12 - interval); // Return smallest interval
    }
    
    //==============================================================================
    // Factory Function
    
    inline std::unique_ptr<ReharmonizationEngine> createReharmonizationEngine()
    {
        return std::make_unique<BasicReharmonizationEngine>();
    }

} // namespace HarmonyEngine

