/*
  ==============================================================================

    OrchestrationEngine.h
    Created: Orchestration engine foundation
    
    This file contains the OrchestrationEngine interface and basic orchestration
    algorithms for SATB arrangement generation with proper voice ranges and
    harmonic context adaptation for orchestration styles.

  ==============================================================================
*/

#pragma once

// Implementation includes (header-only)
#include <algorithm>
#include <random>
#include <cmath>
#include <iterator>

#include "MusicTheory.h"
#include "VoiceLeadingEngine.h"
#include <map>
#include <memory>

namespace HarmonyEngine
{
    //==============================================================================
    // Orchestration Style Definitions
    
    enum class OrchestrationStyleType
    {
        Jazz,               // Extended harmony voicings, spread voicings
        CinematicStrings,   // Lush pad voicings, divisi techniques
        Classical,          // Traditional SATB rules, proper voice ranges
        Pop,                // Contemporary voicing techniques, power chord structures
        Custom              // User-defined style
    };
    
    //==============================================================================
    // Voicing Strategy Configuration
    
    enum class VoicingStrategy
    {
        Close,              // Close position voicings (notes within an octave)
        Open,               // Open position voicings (spread across multiple octaves)
        Drop2,              // Drop-2 voicings (second highest note dropped an octave)
        Drop3,              // Drop-3 voicings (third highest note dropped an octave)
        Quartal,            // Quartal harmony (stacked fourths)
        Cluster,            // Cluster voicings (close intervals)
        Spread              // Wide spread voicings
    };
    
    //==============================================================================
    // Instrument Group Configuration
    
    struct InstrumentGroup
    {
        std::string name;
        std::vector<VoiceType> voices;
        Range combinedRange;
        VoicingStrategy preferredStrategy;
        float density;              // Voice density (0.0-1.0)
        bool allowDoubling;         // Allow note doubling
        
        InstrumentGroup(const std::string& groupName = "Default")
            : name(groupName), density(0.7f), allowDoubling(true)
        {
            // Default SATB configuration
            voices = {VoiceType::Soprano, VoiceType::Alto, VoiceType::Tenor, VoiceType::Bass};
            combinedRange = Range(40, 84); // E2 to C6
            preferredStrategy = VoicingStrategy::Close;
        }
    };
    
    //==============================================================================
    // Dynamic Curve for Expression
    
    struct DynamicCurve
    {
        std::vector<float> dynamicValues; // 0.0-1.0 for each beat/chord
        float defaultDynamic;
        bool hasSwells;
        bool hasCrescendos;
        
        DynamicCurve() : defaultDynamic(0.7f), hasSwells(false), hasCrescendos(false) {}
        
        float getDynamicAtPosition(int position) const
        {
            if (dynamicValues.empty() || position < 0)
                return defaultDynamic;
            
            if (position >= static_cast<int>(dynamicValues.size()))
                return dynamicValues.back();
                
            return dynamicValues[position];
        }
    };
    
    //==============================================================================
    // Orchestration Style Definition
    
    struct OrchestrationStyle
    {
        std::string name;
        OrchestrationStyleType type;
        std::map<VoiceType, VoicingStrategy> voicingStrategies;
        std::vector<InstrumentGroup> instrumentGroups;
        DynamicCurve defaultDynamics;
        
        // Style-specific parameters
        float harmonicComplexity;   // 0.0-1.0 (simple to complex)
        float voiceIndependence;    // 0.0-1.0 (homophonic to polyphonic)
        bool allowExtensions;       // Allow 9ths, 11ths, 13ths
        bool preferSpreadVoicings;  // Prefer wide voicings
        int maxVoiceCount;          // Maximum number of voices
        
        OrchestrationStyle(const std::string& styleName = "Default", 
                          OrchestrationStyleType styleType = OrchestrationStyleType::Classical)
            : name(styleName), type(styleType), harmonicComplexity(0.5f),
              voiceIndependence(0.5f), allowExtensions(false), 
              preferSpreadVoicings(false), maxVoiceCount(4)
        {
            initializeDefaultStrategies();
        }
        
        bool isValid() const
        {
            return !name.empty() &&
                   harmonicComplexity >= 0.0f && harmonicComplexity <= 1.0f &&
                   voiceIndependence >= 0.0f && voiceIndependence <= 1.0f &&
                   maxVoiceCount >= 2 && maxVoiceCount <= 16;
        }
        
    private:
        void initializeDefaultStrategies()
        {
            // Set default voicing strategies for each voice type
            voicingStrategies[VoiceType::Soprano] = VoicingStrategy::Close;
            voicingStrategies[VoiceType::Alto] = VoicingStrategy::Close;
            voicingStrategies[VoiceType::Tenor] = VoicingStrategy::Close;
            voicingStrategies[VoiceType::Bass] = VoicingStrategy::Close;
        }
    };
    
    //==============================================================================
    // SATB Arrangement Result
    
    struct SATBArrangement
    {
        Voice soprano;
        Voice alto;
        Voice tenor;
        Voice bass;
        std::vector<Chord> harmonizedProgression;
        OrchestrationStyle usedStyle;
        float arrangementQuality;   // Overall quality score (0.0-1.0)
        
        SATBArrangement() : arrangementQuality(0.0f) {}
        
        bool isValid() const
        {
            return soprano.isValid() && alto.isValid() && 
                   tenor.isValid() && bass.isValid() &&
                   !harmonizedProgression.empty() &&
                   arrangementQuality >= 0.0f && arrangementQuality <= 1.0f;
        }
        
        std::vector<Voice> getAllVoices() const
        {
            return {soprano, alto, tenor, bass};
        }
        
        size_t getChordCount() const
        {
            return harmonizedProgression.size();
        }
    };
    
    //==============================================================================
    // Orchestration Result
    
    struct OrchestrationResult
    {
        std::vector<Voice> voices;
        std::vector<Chord> originalProgression;
        std::vector<Chord> orchestratedProgression;
        OrchestrationStyle usedStyle;
        std::vector<InstrumentGroup> usedInstrumentGroups;
        float orchestrationQuality; // Overall quality score (0.0-1.0)
        std::vector<std::string> notes; // Additional notes about the orchestration
        
        OrchestrationResult() : orchestrationQuality(0.0f) {}
        
        bool isValid() const
        {
            return !voices.empty() &&
                   originalProgression.size() == orchestratedProgression.size() &&
                   !originalProgression.empty() &&
                   orchestrationQuality >= 0.0f && orchestrationQuality <= 1.0f;
        }
        
        size_t getVoiceCount() const { return voices.size(); }
        size_t getChordCount() const { return orchestratedProgression.size(); }
    };
    
    //==============================================================================
    // Harmonic Context for Orchestration
    
    struct OrchestrationContext
    {
        Key key;
        ChordFunction currentFunction;
        int chordIndex;
        int totalChords;
        float tensionLevel;         // Current harmonic tension (0.0-1.0)
        bool isPhraseBoundary;      // True if at phrase start/end
        bool isCadentialPoint;      // True if approaching cadence
        
        OrchestrationContext()
            : currentFunction(ChordFunction::Tonic), chordIndex(0), totalChords(1),
              tensionLevel(0.0f), isPhraseBoundary(false), isCadentialPoint(false) {}
        
        bool isValid() const
        {
            return key.isValid() &&
                   chordIndex >= 0 && chordIndex < totalChords &&
                   totalChords > 0 &&
                   tensionLevel >= 0.0f && tensionLevel <= 1.0f;
        }
        
        float getProgressionPosition() const
        {
            if (totalChords <= 1) return 0.0f;
            return static_cast<float>(chordIndex) / static_cast<float>(totalChords - 1);
        }
    };
    
    //==============================================================================
    // OrchestrationEngine Interface
    
    class OrchestrationEngine
    {
    public:
        virtual ~OrchestrationEngine() = default;
        
        // Core orchestration functionality
        virtual OrchestrationResult orchestrate(
            const std::vector<Chord>& progression,
            const OrchestrationStyle& style,
            const Key& key
        ) = 0;
        
        virtual SATBArrangement generateSATB(
            const std::vector<Chord>& progression,
            const Key& key
        ) = 0;
        
        // Style management and blending
        virtual OrchestrationStyle blendStyles(
            const std::vector<OrchestrationStyle>& styles,
            const std::vector<float>& weights
        ) = 0;
        
        // Enhanced style blending with characteristic extraction
        virtual std::map<std::string, float> extractStyleCharacteristics(
            const OrchestrationStyle& style
        ) = 0;
        
        virtual OrchestrationStyle createStyleFromCharacteristics(
            const std::map<std::string, float>& characteristics,
            const std::string& styleName = "Custom Blend"
        ) = 0;
        
        // Real-time style parameter adjustment
        virtual OrchestrationStyle adjustStyleParameters(
            const OrchestrationStyle& baseStyle,
            const std::map<std::string, float>& parameterAdjustments
        ) = 0;
        
        virtual std::vector<OrchestrationStyle> getAvailableStyles() = 0;
        virtual OrchestrationStyle getStyleByType(OrchestrationStyleType type) = 0;
        
        // Harmonic context adaptation
        virtual OrchestrationContext analyzeOrchestrationContext(
            const std::vector<Chord>& progression,
            int chordIndex,
            const Key& key
        ) = 0;
        
        virtual OrchestrationStyle adaptStyleToContext(
            const OrchestrationStyle& baseStyle,
            const OrchestrationContext& context
        ) = 0;
        
        // Voice arrangement utilities
        virtual std::vector<Voice> arrangeVoices(
            const Chord& chord,
            const OrchestrationStyle& style,
            const OrchestrationContext& context
        ) = 0;
        
        virtual bool validateVoiceArrangement(
            const std::vector<Voice>& voices,
            const OrchestrationStyle& style
        ) = 0;
        
        // Quality assessment
        virtual float calculateOrchestrationQuality(
            const OrchestrationResult& result
        ) = 0;
        
        virtual float calculateSATBQuality(
            const SATBArrangement& arrangement
        ) = 0;
    };
    
    //==============================================================================
    // Basic Implementation
    
    class BasicOrchestrationEngine : public OrchestrationEngine
    {
    public:
        BasicOrchestrationEngine();
        ~BasicOrchestrationEngine() override = default;
        
        // OrchestrationEngine interface implementation
        OrchestrationResult orchestrate(
            const std::vector<Chord>& progression,
            const OrchestrationStyle& style,
            const Key& key
        ) override;
        
        SATBArrangement generateSATB(
            const std::vector<Chord>& progression,
            const Key& key
        ) override;
        
        OrchestrationStyle blendStyles(
            const std::vector<OrchestrationStyle>& styles,
            const std::vector<float>& weights
        ) override;
        
        // Enhanced style blending implementation
        std::map<std::string, float> extractStyleCharacteristics(
            const OrchestrationStyle& style
        ) override;
        
        OrchestrationStyle createStyleFromCharacteristics(
            const std::map<std::string, float>& characteristics,
            const std::string& styleName = "Custom Blend"
        ) override;
        
        OrchestrationStyle adjustStyleParameters(
            const OrchestrationStyle& baseStyle,
            const std::map<std::string, float>& parameterAdjustments
        ) override;
        
        std::vector<OrchestrationStyle> getAvailableStyles() override;
        OrchestrationStyle getStyleByType(OrchestrationStyleType type) override;
        
        OrchestrationContext analyzeOrchestrationContext(
            const std::vector<Chord>& progression,
            int chordIndex,
            const Key& key
        ) override;
        
        OrchestrationStyle adaptStyleToContext(
            const OrchestrationStyle& baseStyle,
            const OrchestrationContext& context
        ) override;
        
        std::vector<Voice> arrangeVoices(
            const Chord& chord,
            const OrchestrationStyle& style,
            const OrchestrationContext& context
        ) override;
        
        bool validateVoiceArrangement(
            const std::vector<Voice>& voices,
            const OrchestrationStyle& style
        ) override;
        
        float calculateOrchestrationQuality(
            const OrchestrationResult& result
        ) override;
        
        float calculateSATBQuality(
            const SATBArrangement& arrangement
        ) override;
        
    private:
        std::unique_ptr<VoiceLeadingEngine> voiceLeadingEngine;
        std::vector<OrchestrationStyle> availableStyles;
        
        // Style initialization
        void initializeAvailableStyles();
        OrchestrationStyle createJazzStyle();
        OrchestrationStyle createCinematicStringsStyle();
        OrchestrationStyle createClassicalStyle();
        OrchestrationStyle createPopStyle();
        
        // SATB arrangement helpers
        SATBArrangement arrangeSATBProgression(
            const std::vector<Chord>& progression,
            const Key& key
        );
        
        void arrangeSATBChord(
            const Chord& chord,
            Voice& soprano,
            Voice& alto,
            Voice& tenor,
            Voice& bass,
            int chordIndex,
            const Key& key
        );
        
        // Voice arrangement algorithms
        std::vector<Voice> arrangeClosePosition(
            const Chord& chord,
            const OrchestrationStyle& style,
            const OrchestrationContext& context
        );
        
        std::vector<Voice> arrangeOpenPosition(
            const Chord& chord,
            const OrchestrationStyle& style,
            const OrchestrationContext& context
        );
        
        std::vector<Voice> arrangeDrop2Voicing(
            const Chord& chord,
            const OrchestrationStyle& style,
            const OrchestrationContext& context
        );
        
        // Harmonic context analysis
        ChordFunction analyzeChordFunction(
            const Chord& chord,
            const Key& key
        );
        
        float calculateHarmonicTension(
            const Chord& chord,
            const Key& key,
            const OrchestrationContext& context
        );
        
        // Style adaptation helpers
        void adaptComplexityToContext(
            OrchestrationStyle& style,
            const OrchestrationContext& context
        );
        
        void adaptVoicingToTension(
            OrchestrationStyle& style,
            float tensionLevel
        );
        
        // Quality assessment helpers
        float assessVoiceLeadingQuality(const std::vector<Voice>& voices);
        float assessHarmonicBalance(const std::vector<Voice>& voices);
        float assessStyleConsistency(
            const OrchestrationResult& result,
            const OrchestrationStyle& targetStyle
        );
        
        // Validation helpers
        bool validateVoiceRanges(const std::vector<Voice>& voices);
        bool validateVoiceSpacing(const std::vector<Voice>& voices);
        bool validateStyleCompliance(
            const std::vector<Voice>& voices,
            const OrchestrationStyle& style
        );
        
        // Style-specific arrangement methods
        std::vector<Voice> arrangeClassicalSATB(
            const Chord& chord,
            const OrchestrationStyle& style,
            const OrchestrationContext& context
        );
        
        std::vector<Voice> arrangeJazzExtendedHarmony(
            const Chord& chord,
            const OrchestrationStyle& style,
            const OrchestrationContext& context
        );
        
        std::vector<Voice> arrangeCinematicPadVoicing(
            const Chord& chord,
            const OrchestrationStyle& style,
            const OrchestrationContext& context
        );
        
        // Utility functions
        std::vector<int> extractChordTones(const Chord& chord);
        std::vector<int> distributeNotesToVoices(
            const std::vector<int>& chordTones,
            int voiceCount,
            const OrchestrationStyle& style
        );
        
        Range getVoiceRangeForStyle(
            VoiceType voiceType,
            const OrchestrationStyle& style
        );
        
        // Enhanced style blending helpers
        float calculateStyleSimilarity(
            const OrchestrationStyle& style1,
            const OrchestrationStyle& style2
        );
        
        VoicingStrategy blendVoicingStrategies(
            const std::vector<VoicingStrategy>& strategies,
            const std::vector<float>& weights
        );
        
        DynamicCurve blendDynamicCurves(
            const std::vector<DynamicCurve>& curves,
            const std::vector<float>& weights
        );
        
        std::vector<InstrumentGroup> blendInstrumentGroups(
            const std::vector<std::vector<InstrumentGroup>>& groupSets,
            const std::vector<float>& weights
        );
    };
    
    //==============================================================================
    // Factory Function
    
    std::unique_ptr<OrchestrationEngine> createOrchestrationEngine();
    
} // namespace HarmonyEngine

// ============================================================================
// Implementation (merged from .cpp - header-only library)
// ============================================================================

namespace HarmonyEngine
{
    //==============================================================================
    // BasicOrchestrationEngine Implementation
    
  inline   BasicOrchestrationEngine::BasicOrchestrationEngine()
    {
        // Initialize voice leading engine for smooth voice leading
        voiceLeadingEngine = std::make_unique<BasicVoiceLeadingEngine>();
        
        // Initialize available orchestration styles
        initializeAvailableStyles();
    }
    
    //==============================================================================
    // Core Orchestration Functionality
    
    inline OrchestrationResult BasicOrchestrationEngine::orchestrate(
        const std::vector<Chord>& progression,
        const OrchestrationStyle& style,
        const Key& key)
    {
        OrchestrationResult result;
        
        if (progression.empty() || !key.isValid() || !style.isValid())
        {
            return result; // Return invalid result
        }
        
        result.originalProgression = progression;
        result.usedStyle = style;
        
        // Process each chord in the progression
        for (size_t i = 0; i < progression.size(); ++i)
        {
            // Analyze harmonic context for this chord
            OrchestrationContext context = analyzeOrchestrationContext(progression, static_cast<int>(i), key);
            
            // Adapt style to current context
            OrchestrationStyle adaptedStyle = adaptStyleToContext(style, context);
            
            // Arrange voices for this chord
            std::vector<Voice> chordVoices = arrangeVoices(progression[i], adaptedStyle, context);
            
            // Add voices to result (merge with existing voices)
            if (result.voices.empty())
            {
                result.voices = chordVoices;
            }
            else
            {
                // Add notes to existing voices
                for (size_t voiceIdx = 0; voiceIdx < chordVoices.size() && voiceIdx < result.voices.size(); ++voiceIdx)
                {
                    if (!chordVoices[voiceIdx].notes.empty())
                    {
                        result.voices[voiceIdx].notes.insert(
                            result.voices[voiceIdx].notes.end(),
                            chordVoices[voiceIdx].notes.begin(),
                            chordVoices[voiceIdx].notes.end()
                        );
                    }
                }
            }
            
            // Create orchestrated chord from arranged voices
            Chord orchestratedChord = progression[i]; // Start with original
            // Note: In a full implementation, we would reconstruct the chord from the voices
            result.orchestratedProgression.push_back(orchestratedChord);
        }
        
        // Calculate overall orchestration quality
        result.orchestrationQuality = calculateOrchestrationQuality(result);
        
        return result;
    }
    
    inline SATBArrangement BasicOrchestrationEngine::generateSATB(
        const std::vector<Chord>& progression,
        const Key& key)
    {
        SATBArrangement arrangement;
        
        if (progression.empty() || !key.isValid())
        {
            return arrangement; // Return invalid arrangement
        }
        
        // Use classical style for SATB arrangement
        OrchestrationStyle classicalStyle = getStyleByType(OrchestrationStyleType::Classical);
        arrangement.usedStyle = classicalStyle;
        
        // Initialize voices with proper types and ranges
        arrangement.soprano = Voice(VoiceType::Soprano);
        arrangement.alto = Voice(VoiceType::Alto);
        arrangement.tenor = Voice(VoiceType::Tenor);
        arrangement.bass = Voice(VoiceType::Bass);
        
        // Arrange each chord in SATB format
        for (size_t i = 0; i < progression.size(); ++i)
        {
            arrangeSATBChord(
                progression[i],
                arrangement.soprano,
                arrangement.alto,
                arrangement.tenor,
                arrangement.bass,
                static_cast<int>(i),
                key
            );
            
            arrangement.harmonizedProgression.push_back(progression[i]);
        }
        
        // Calculate arrangement quality
        arrangement.arrangementQuality = calculateSATBQuality(arrangement);
        
        return arrangement;
    }
    
    //==============================================================================
    // Style Management and Blending
    
    inline OrchestrationStyle BasicOrchestrationEngine::blendStyles(
        const std::vector<OrchestrationStyle>& styles,
        const std::vector<float>& weights)
    {
        OrchestrationStyle blendedStyle("Blended Style", OrchestrationStyleType::Custom);
        
        if (styles.empty() || weights.empty() || styles.size() != weights.size())
        {
            return getStyleByType(OrchestrationStyleType::Classical); // Return default
        }
        
        // Normalize weights
        float totalWeight = 0.0f;
        for (float weight : weights)
        {
            totalWeight += std::max(0.0f, weight); // Ensure non-negative weights
        }
        
        if (totalWeight <= 0.0f)
        {
            return getStyleByType(OrchestrationStyleType::Classical); // Return default
        }
        
        // Enhanced blending: Extract characteristics from all styles
        std::vector<std::map<std::string, float>> styleCharacteristics;
        for (const auto& style : styles)
        {
            styleCharacteristics.push_back(extractStyleCharacteristics(style));
        }
        
        // Blend characteristics with weighted combination
        std::map<std::string, float> blendedCharacteristics;
        
        // Initialize with first style's characteristics
        if (!styleCharacteristics.empty())
        {
            for (const auto& characteristic : styleCharacteristics[0])
            {
                blendedCharacteristics[characteristic.first] = 0.0f;
            }
        }
        
        // Weighted blend of all characteristics
        for (size_t i = 0; i < styles.size(); ++i)
        {
            float normalizedWeight = weights[i] / totalWeight;
            
            for (const auto& characteristic : styleCharacteristics[i])
            {
                blendedCharacteristics[characteristic.first] += characteristic.second * normalizedWeight;
            }
        }
        
        // Create blended style from characteristics
        blendedStyle = createStyleFromCharacteristics(blendedCharacteristics, "Advanced Blended Style");
        
        // Enhanced voicing strategy blending
        std::vector<VoicingStrategy> sopranoStrategies, altoStrategies, tenorStrategies, bassStrategies;
        std::vector<float> normalizedWeights;
        
        for (size_t i = 0; i < styles.size(); ++i)
        {
            float normalizedWeight = weights[i] / totalWeight;
            normalizedWeights.push_back(normalizedWeight);
            
            // Collect voicing strategies for each voice type
            auto it = styles[i].voicingStrategies.find(VoiceType::Soprano);
            sopranoStrategies.push_back(it != styles[i].voicingStrategies.end() ? it->second : VoicingStrategy::Close);
            
            it = styles[i].voicingStrategies.find(VoiceType::Alto);
            altoStrategies.push_back(it != styles[i].voicingStrategies.end() ? it->second : VoicingStrategy::Close);
            
            it = styles[i].voicingStrategies.find(VoiceType::Tenor);
            tenorStrategies.push_back(it != styles[i].voicingStrategies.end() ? it->second : VoicingStrategy::Close);
            
            it = styles[i].voicingStrategies.find(VoiceType::Bass);
            bassStrategies.push_back(it != styles[i].voicingStrategies.end() ? it->second : VoicingStrategy::Close);
        }
        
        // Blend voicing strategies for each voice type
        blendedStyle.voicingStrategies[VoiceType::Soprano] = blendVoicingStrategies(sopranoStrategies, normalizedWeights);
        blendedStyle.voicingStrategies[VoiceType::Alto] = blendVoicingStrategies(altoStrategies, normalizedWeights);
        blendedStyle.voicingStrategies[VoiceType::Tenor] = blendVoicingStrategies(tenorStrategies, normalizedWeights);
        blendedStyle.voicingStrategies[VoiceType::Bass] = blendVoicingStrategies(bassStrategies, normalizedWeights);
        
        // Enhanced dynamic curve blending
        std::vector<DynamicCurve> dynamicCurves;
        for (const auto& style : styles)
        {
            dynamicCurves.push_back(style.defaultDynamics);
        }
        blendedStyle.defaultDynamics = blendDynamicCurves(dynamicCurves, normalizedWeights);
        
        // Enhanced instrument group blending
        std::vector<std::vector<InstrumentGroup>> instrumentGroupSets;
        for (const auto& style : styles)
        {
            instrumentGroupSets.push_back(style.instrumentGroups);
        }
        blendedStyle.instrumentGroups = blendInstrumentGroups(instrumentGroupSets, normalizedWeights);
        
        return blendedStyle;
    }
    
    inline std::vector<OrchestrationStyle> BasicOrchestrationEngine::getAvailableStyles()
    {
        return availableStyles;
    }
    
    inline OrchestrationStyle BasicOrchestrationEngine::getStyleByType(OrchestrationStyleType type)
    {
        auto it = std::find_if(availableStyles.begin(), availableStyles.end(),
                               [type](const OrchestrationStyle& s) { return s.type == type; });
        return (it != availableStyles.end()) ? *it : createClassicalStyle();
    }
    
    //==============================================================================
    // Enhanced Style Blending Implementation
    
    inline std::map<std::string, float> BasicOrchestrationEngine::extractStyleCharacteristics(
        const OrchestrationStyle& style)
    {
        std::map<std::string, float> characteristics;
        
        if (!style.isValid())
        {
            return characteristics;
        }
        
        // Extract core numeric characteristics
        characteristics["harmonicComplexity"] = style.harmonicComplexity;
        characteristics["voiceIndependence"] = style.voiceIndependence;
        characteristics["maxVoiceCount"] = static_cast<float>(style.maxVoiceCount) / 16.0f; // Normalize to 0-1
        
        // Extract boolean characteristics as 0.0 or 1.0
        characteristics["allowExtensions"] = style.allowExtensions ? 1.0f : 0.0f;
        characteristics["preferSpreadVoicings"] = style.preferSpreadVoicings ? 1.0f : 0.0f;
        
        // Extract dynamic characteristics
        characteristics["defaultDynamic"] = style.defaultDynamics.defaultDynamic;
        characteristics["hasSwells"] = style.defaultDynamics.hasSwells ? 1.0f : 0.0f;
        characteristics["hasCrescendos"] = style.defaultDynamics.hasCrescendos ? 1.0f : 0.0f;
        
        // Extract voicing strategy characteristics (convert to numeric values)
        float closeStrategyWeight = 0.0f;
        float openStrategyWeight = 0.0f;
        float drop2StrategyWeight = 0.0f;
        float quartalStrategyWeight = 0.0f;
        float spreadStrategyWeight = 0.0f;
        
        for (const auto& voicingPair : style.voicingStrategies)
        {
            switch (voicingPair.second)
            {
                case VoicingStrategy::Close:
                    closeStrategyWeight += 0.25f;
                    break;
                case VoicingStrategy::Open:
                    openStrategyWeight += 0.25f;
                    break;
                case VoicingStrategy::Drop2:
                    drop2StrategyWeight += 0.25f;
                    break;
                case VoicingStrategy::Quartal:
                    quartalStrategyWeight += 0.25f;
                    break;
                case VoicingStrategy::Spread:
                    spreadStrategyWeight += 0.25f;
                    break;
                default:
                    closeStrategyWeight += 0.25f; // Default to close
                    break;
            }
        }
        
        characteristics["closeStrategyWeight"] = closeStrategyWeight;
        characteristics["openStrategyWeight"] = openStrategyWeight;
        characteristics["drop2StrategyWeight"] = drop2StrategyWeight;
        characteristics["quartalStrategyWeight"] = quartalStrategyWeight;
        characteristics["spreadStrategyWeight"] = spreadStrategyWeight;
        
        // Extract instrument group characteristics
        if (!style.instrumentGroups.empty())
        {
            float avgDensity = 0.0f;
            float allowDoublingRatio = 0.0f;
            
            for (const auto& group : style.instrumentGroups)
            {
                avgDensity += group.density;
                if (group.allowDoubling) allowDoublingRatio += 1.0f;
            }
            
            avgDensity /= static_cast<float>(style.instrumentGroups.size());
            allowDoublingRatio /= static_cast<float>(style.instrumentGroups.size());
            
            characteristics["avgInstrumentDensity"] = avgDensity;
            characteristics["allowDoublingRatio"] = allowDoublingRatio;
        }
        
        return characteristics;
    }
    
    inline OrchestrationStyle BasicOrchestrationEngine::createStyleFromCharacteristics(
        const std::map<std::string, float>& characteristics,
        const std::string& styleName)
    {
        OrchestrationStyle style(styleName, OrchestrationStyleType::Custom);
        
        if (characteristics.empty())
        {
            return createClassicalStyle(); // Return default if no characteristics
        }
        
        // Set core numeric parameters
        auto it = characteristics.find("harmonicComplexity");
        if (it != characteristics.end())
        {
            style.harmonicComplexity = std::max(0.0f, std::min(1.0f, it->second));
        }
        
        it = characteristics.find("voiceIndependence");
        if (it != characteristics.end())
        {
            style.voiceIndependence = std::max(0.0f, std::min(1.0f, it->second));
        }
        
        it = characteristics.find("maxVoiceCount");
        if (it != characteristics.end())
        {
            style.maxVoiceCount = static_cast<int>(std::max(2.0f, std::min(16.0f, it->second * 16.0f)));
        }
        
        // Set boolean parameters
        it = characteristics.find("allowExtensions");
        if (it != characteristics.end())
        {
            style.allowExtensions = (it->second >= 0.5f);
        }
        
        it = characteristics.find("preferSpreadVoicings");
        if (it != characteristics.end())
        {
            style.preferSpreadVoicings = (it->second >= 0.5f);
        }
        
        // Set dynamic characteristics
        it = characteristics.find("defaultDynamic");
        if (it != characteristics.end())
        {
            style.defaultDynamics.defaultDynamic = std::max(0.0f, std::min(1.0f, it->second));
        }
        
        it = characteristics.find("hasSwells");
        if (it != characteristics.end())
        {
            style.defaultDynamics.hasSwells = (it->second >= 0.5f);
        }
        
        it = characteristics.find("hasCrescendos");
        if (it != characteristics.end())
        {
            style.defaultDynamics.hasCrescendos = (it->second >= 0.5f);
        }
        
        // Reconstruct voicing strategies from weights
        std::vector<VoicingStrategy> strategies = {
            VoicingStrategy::Close, VoicingStrategy::Open, VoicingStrategy::Drop2,
            VoicingStrategy::Quartal, VoicingStrategy::Spread
        };
        
        std::vector<std::string> strategyNames = {
            "closeStrategyWeight", "openStrategyWeight", "drop2StrategyWeight",
            "quartalStrategyWeight", "spreadStrategyWeight"
        };
        
        // Find dominant strategy for each voice type
        VoicingStrategy dominantStrategy = VoicingStrategy::Close;
        float maxWeight = 0.0f;
        
        for (size_t i = 0; i < strategyNames.size(); ++i)
        {
            it = characteristics.find(strategyNames[i]);
            if (it != characteristics.end() && it->second > maxWeight)
            {
                maxWeight = it->second;
                dominantStrategy = strategies[i];
            }
        }
        
        // Apply dominant strategy to all voice types
        style.voicingStrategies[VoiceType::Soprano] = dominantStrategy;
        style.voicingStrategies[VoiceType::Alto] = dominantStrategy;
        style.voicingStrategies[VoiceType::Tenor] = dominantStrategy;
        style.voicingStrategies[VoiceType::Bass] = dominantStrategy;
        
        return style;
    }
    
    inline OrchestrationStyle BasicOrchestrationEngine::adjustStyleParameters(
        const OrchestrationStyle& baseStyle,
        const std::map<std::string, float>& parameterAdjustments)
    {
        OrchestrationStyle adjustedStyle = baseStyle;
        
        if (!baseStyle.isValid() || parameterAdjustments.empty())
        {
            return adjustedStyle;
        }
        
        // Apply real-time parameter adjustments
        for (const auto& adjustment : parameterAdjustments)
        {
            const std::string& paramName = adjustment.first;
            float adjustmentValue = adjustment.second;
            
            if (paramName == "harmonicComplexity")
            {
                adjustedStyle.harmonicComplexity = std::max(0.0f, std::min(1.0f, 
                    adjustedStyle.harmonicComplexity + adjustmentValue));
            }
            else if (paramName == "voiceIndependence")
            {
                adjustedStyle.voiceIndependence = std::max(0.0f, std::min(1.0f, 
                    adjustedStyle.voiceIndependence + adjustmentValue));
            }
            else if (paramName == "maxVoiceCount")
            {
                int newVoiceCount = adjustedStyle.maxVoiceCount + static_cast<int>(adjustmentValue);
                adjustedStyle.maxVoiceCount = std::max(2, std::min(16, newVoiceCount));
            }
            else if (paramName == "allowExtensions")
            {
                // Toggle boolean if adjustment is significant
                if (std::abs(adjustmentValue) >= 0.5f)
                {
                    adjustedStyle.allowExtensions = !adjustedStyle.allowExtensions;
                }
            }
            else if (paramName == "preferSpreadVoicings")
            {
                // Toggle boolean if adjustment is significant
                if (std::abs(adjustmentValue) >= 0.5f)
                {
                    adjustedStyle.preferSpreadVoicings = !adjustedStyle.preferSpreadVoicings;
                }
            }
            else if (paramName == "defaultDynamic")
            {
                adjustedStyle.defaultDynamics.defaultDynamic = std::max(0.0f, std::min(1.0f, 
                    adjustedStyle.defaultDynamics.defaultDynamic + adjustmentValue));
            }
            else if (paramName == "instrumentDensity")
            {
                // Adjust density for all instrument groups
                for (auto& group : adjustedStyle.instrumentGroups)
                {
                    group.density = std::max(0.0f, std::min(1.0f, group.density + adjustmentValue));
                }
            }
        }
        
        return adjustedStyle;
    }
    
    //==============================================================================
    // Harmonic Context Analysis and Adaptation
    
    inline OrchestrationContext BasicOrchestrationEngine::analyzeOrchestrationContext(
        const std::vector<Chord>& progression,
        int chordIndex,
        const Key& key)
    {
        OrchestrationContext context;
        
        if (chordIndex < 0 || chordIndex >= static_cast<int>(progression.size()) || !key.isValid())
        {
            return context; // Return invalid context
        }
        
        context.key = key;
        context.chordIndex = chordIndex;
        context.totalChords = static_cast<int>(progression.size());
        
        // Analyze chord function
        context.currentFunction = analyzeChordFunction(progression[chordIndex], key);
        
        // Calculate harmonic tension
        context.tensionLevel = calculateHarmonicTension(progression[chordIndex], key, context);
        
        // Determine phrase boundaries (simple heuristic: every 4 or 8 chords)
        context.isPhraseBoundary = (chordIndex % 4 == 0) || (chordIndex % 8 == 0);
        
        // Determine cadential points (approaching end of phrase or progression)
        int remainingChords = context.totalChords - chordIndex - 1;
        context.isCadentialPoint = (remainingChords <= 1) || 
                                  ((chordIndex + 1) % 4 == 0) || 
                                  ((chordIndex + 1) % 8 == 0);
        
        return context;
    }
    
    inline OrchestrationStyle BasicOrchestrationEngine::adaptStyleToContext(
        const OrchestrationStyle& baseStyle,
        const OrchestrationContext& context)
    {
        OrchestrationStyle adaptedStyle = baseStyle;
        
        if (!context.isValid())
        {
            return adaptedStyle;
        }
        
        // Adapt complexity based on context
        adaptComplexityToContext(adaptedStyle, context);
        
        // Adapt voicing based on tension level
        adaptVoicingToTension(adaptedStyle, context.tensionLevel);
        
        return adaptedStyle;
    }
    
    //==============================================================================
    // Voice Arrangement
    
    inline std::vector<Voice> BasicOrchestrationEngine::arrangeVoices(
        const Chord& chord,
        const OrchestrationStyle& style,
        const OrchestrationContext& context)
    {
        std::vector<Voice> voices;
        
        if (!chord.isValid() || !style.isValid() || !context.isValid())
        {
            return voices;
        }
        
        // Use style-specific arrangement methods for optimal results
        switch (style.type)
        {
            case OrchestrationStyleType::Jazz:
                voices = arrangeJazzExtendedHarmony(chord, style, context);
                break;
                
            case OrchestrationStyleType::CinematicStrings:
                voices = arrangeCinematicPadVoicing(chord, style, context);
                break;
                
            case OrchestrationStyleType::Classical:
                voices = arrangeClassicalSATB(chord, style, context);
                break;
                
            case OrchestrationStyleType::Pop:
            case OrchestrationStyleType::Custom:
            default:
                // Fall back to strategy-based arrangement
                VoicingStrategy strategy = VoicingStrategy::Close;
                if (!style.voicingStrategies.empty())
                {
                    strategy = style.voicingStrategies.begin()->second;
                }
                
                switch (strategy)
                {
                    case VoicingStrategy::Close:
                        voices = arrangeClosePosition(chord, style, context);
                        break;
                    case VoicingStrategy::Open:
                        voices = arrangeOpenPosition(chord, style, context);
                        break;
                    case VoicingStrategy::Drop2:
                        voices = arrangeDrop2Voicing(chord, style, context);
                        break;
                    default:
                        voices = arrangeClosePosition(chord, style, context);
                        break;
                }
                break;
        }
        
        return voices;
    }
    
    inline bool BasicOrchestrationEngine::validateVoiceArrangement(
        const std::vector<Voice>& voices,
        const OrchestrationStyle& style)
    {
        if (voices.empty() || !style.isValid())
        {
            return false;
        }

        // Validate voice ranges, spacing, and style compliance
        return validateVoiceRanges(voices) &&
               validateVoiceSpacing(voices) &&
               validateStyleCompliance(voices, style);
    }
    
    //==============================================================================
    // Quality Assessment
    
    inline float BasicOrchestrationEngine::calculateOrchestrationQuality(
        const OrchestrationResult& result)
    {
        if (!result.isValid())
        {
            return 0.0f;
        }
        
        float voiceLeadingQuality = assessVoiceLeadingQuality(result.voices);
        float harmonicBalance = assessHarmonicBalance(result.voices);
        float styleConsistency = assessStyleConsistency(result, result.usedStyle);
        
        // Weighted average of quality factors
        return (voiceLeadingQuality * 0.4f + harmonicBalance * 0.3f + styleConsistency * 0.3f);
    }
    
    inline float BasicOrchestrationEngine::calculateSATBQuality(
        const SATBArrangement& arrangement)
    {
        if (!arrangement.isValid())
        {
            return 0.0f;
        }
        
        std::vector<Voice> voices = arrangement.getAllVoices();
        
        float voiceLeadingQuality = assessVoiceLeadingQuality(voices);
        float harmonicBalance = assessHarmonicBalance(voices);
        
        // For SATB, we also check traditional voice leading rules
        float traditionalCompliance = 1.0f;
        
        // Check for parallel fifths and octaves using voice leading engine
        for (size_t i = 1; i < voices[0].notes.size(); ++i)
        {
            if (voiceLeadingEngine->hasParallelFifths(voices, static_cast<int>(i-1), static_cast<int>(i)) ||
                voiceLeadingEngine->hasParallelOctaves(voices, static_cast<int>(i-1), static_cast<int>(i)))
            {
                traditionalCompliance *= 0.8f; // Penalty for parallel motion
            }
        }
        
        return (voiceLeadingQuality * 0.4f + harmonicBalance * 0.3f + traditionalCompliance * 0.3f);
    }
    
    //==============================================================================
    // Private Helper Methods - Style Initialization
    
    inline void BasicOrchestrationEngine::initializeAvailableStyles()
    {
        availableStyles.clear();
        availableStyles.push_back(createJazzStyle());
        availableStyles.push_back(createCinematicStringsStyle());
        availableStyles.push_back(createClassicalStyle());
        availableStyles.push_back(createPopStyle());
    }
    
    inline OrchestrationStyle BasicOrchestrationEngine::createJazzStyle()
    {
        OrchestrationStyle jazzStyle("Jazz", OrchestrationStyleType::Jazz);
        
        // Jazz characteristics: Extended harmony voicings, spread voicings, rhythmic comping
        jazzStyle.harmonicComplexity = 0.8f;        // High complexity for extended chords
        jazzStyle.voiceIndependence = 0.7f;         // Moderate independence for comping
        jazzStyle.allowExtensions = true;           // Essential for jazz harmony (9ths, 11ths, 13ths)
        jazzStyle.preferSpreadVoicings = true;      // Wide voicings characteristic of jazz
        jazzStyle.maxVoiceCount = 6;                // Allow for extended harmony voicings
        
        // Jazz-specific voicing strategies for extended harmony
        jazzStyle.voicingStrategies[VoiceType::Soprano] = VoicingStrategy::Drop2;    // Drop-2 for melody
        jazzStyle.voicingStrategies[VoiceType::Alto] = VoicingStrategy::Drop2;       // Drop-2 for inner voices
        jazzStyle.voicingStrategies[VoiceType::Tenor] = VoicingStrategy::Quartal;    // Quartal harmony
        jazzStyle.voicingStrategies[VoiceType::Bass] = VoicingStrategy::Open;        // Open bass lines
        
        // Create jazz-specific instrument groups
        InstrumentGroup jazzCombo("Jazz Combo");
        jazzCombo.voices = {VoiceType::Soprano, VoiceType::Alto, VoiceType::Tenor, VoiceType::Bass};
        jazzCombo.combinedRange = Range(36, 96);    // Extended range for jazz instruments
        jazzCombo.preferredStrategy = VoicingStrategy::Drop2;
        jazzCombo.density = 0.8f;                   // Rich harmonic density
        jazzCombo.allowDoubling = false;            // Avoid doubling for clarity
        
        InstrumentGroup jazzExtended("Jazz Extended");
        jazzExtended.voices = {VoiceType::Soprano, VoiceType::Alto, VoiceType::Tenor, 
                              VoiceType::Bass, VoiceType::Soprano, VoiceType::Alto}; // 6 voices
        jazzExtended.combinedRange = Range(36, 96);
        jazzExtended.preferredStrategy = VoicingStrategy::Spread;
        jazzExtended.density = 0.9f;                // Very rich for extended chords
        jazzExtended.allowDoubling = true;          // Allow doubling for fullness
        
        jazzStyle.instrumentGroups = {jazzCombo, jazzExtended};
        
        // Jazz dynamics: Moderate with swells for expression
        jazzStyle.defaultDynamics.defaultDynamic = 0.7f;
        jazzStyle.defaultDynamics.hasSwells = true;
        jazzStyle.defaultDynamics.hasCrescendos = false;
        
        return jazzStyle;
    }
    
    inline OrchestrationStyle BasicOrchestrationEngine::createCinematicStringsStyle()
    {
        OrchestrationStyle cinematicStyle("Cinematic Strings", OrchestrationStyleType::CinematicStrings);
        
        // Cinematic characteristics: Lush pad voicings, divisi techniques, dynamic swells
        cinematicStyle.harmonicComplexity = 0.6f;   // Moderate complexity for lush harmony
        cinematicStyle.voiceIndependence = 0.4f;    // Lower independence for pad-like texture
        cinematicStyle.allowExtensions = true;      // Extensions for lush sound
        cinematicStyle.preferSpreadVoicings = true; // Wide voicings for cinematic breadth
        cinematicStyle.maxVoiceCount = 8;           // Multiple divisi sections
        
        // Cinematic-specific voicing strategies for lush pad voicings
        cinematicStyle.voicingStrategies[VoiceType::Soprano] = VoicingStrategy::Spread;  // Wide spread for strings
        cinematicStyle.voicingStrategies[VoiceType::Alto] = VoicingStrategy::Spread;     // Lush inner voices
        cinematicStyle.voicingStrategies[VoiceType::Tenor] = VoicingStrategy::Open;      // Open voicing for warmth
        cinematicStyle.voicingStrategies[VoiceType::Bass] = VoicingStrategy::Open;       // Open bass for foundation
        
        // Create cinematic-specific instrument groups with divisi techniques
        InstrumentGroup violins1("Violins I");
        violins1.voices = {VoiceType::Soprano, VoiceType::Soprano}; // Divisi violins
        violins1.combinedRange = Range(55, 96);     // G3 to C7
        violins1.preferredStrategy = VoicingStrategy::Spread;
        violins1.density = 0.9f;                    // Very dense for lush sound
        violins1.allowDoubling = true;              // Divisi allows doubling
        
        InstrumentGroup violins2("Violins II");
        violins2.voices = {VoiceType::Alto, VoiceType::Alto}; // Divisi second violins
        violins2.combinedRange = Range(48, 84);     // C3 to C6
        violins2.preferredStrategy = VoicingStrategy::Spread;
        violins2.density = 0.9f;
        violins2.allowDoubling = true;
        
        InstrumentGroup violas("Violas");
        violas.voices = {VoiceType::Tenor, VoiceType::Tenor}; // Divisi violas
        violas.combinedRange = Range(48, 79);       // C3 to G5
        violas.preferredStrategy = VoicingStrategy::Open;
        violas.density = 0.8f;
        violas.allowDoubling = true;
        
        InstrumentGroup cellos("Cellos");
        cellos.voices = {VoiceType::Bass, VoiceType::Bass}; // Divisi cellos
        cellos.combinedRange = Range(36, 72);       // C2 to C5
        cellos.preferredStrategy = VoicingStrategy::Open;
        cellos.density = 0.7f;
        cellos.allowDoubling = true;
        
        cinematicStyle.instrumentGroups = {violins1, violins2, violas, cellos};
        
        // Cinematic dynamics: Dynamic swells and crescendos for emotional impact
        cinematicStyle.defaultDynamics.defaultDynamic = 0.6f;
        cinematicStyle.defaultDynamics.hasSwells = true;        // Essential for cinematic expression
        cinematicStyle.defaultDynamics.hasCrescendos = true;    // Build-ups for dramatic effect
        
        // Create dynamic curve with swells
        cinematicStyle.defaultDynamics.dynamicValues = {0.4f, 0.6f, 0.8f, 0.9f, 0.7f, 0.5f, 0.3f, 0.6f};
        
        return cinematicStyle;
    }
    
    inline OrchestrationStyle BasicOrchestrationEngine::createClassicalStyle()
    {
        OrchestrationStyle classicalStyle("Classical", OrchestrationStyleType::Classical);
        
        // Classical characteristics: Traditional SATB rules, proper voice leading, functional harmony
        classicalStyle.harmonicComplexity = 0.4f;      // Moderate complexity following traditional rules
        classicalStyle.voiceIndependence = 0.6f;       // Good independence for contrapuntal writing
        classicalStyle.allowExtensions = false;        // Traditional triads and seventh chords only
        classicalStyle.preferSpreadVoicings = false;   // Close position preferred in classical SATB
        classicalStyle.maxVoiceCount = 4;              // Traditional SATB (4 voices)
        
        // Classical SATB voicing strategies following traditional rules
        classicalStyle.voicingStrategies[VoiceType::Soprano] = VoicingStrategy::Close;  // Close position
        classicalStyle.voicingStrategies[VoiceType::Alto] = VoicingStrategy::Close;     // Close position
        classicalStyle.voicingStrategies[VoiceType::Tenor] = VoicingStrategy::Close;    // Close position
        classicalStyle.voicingStrategies[VoiceType::Bass] = VoicingStrategy::Close;     // Close position
        
        // Create classical SATB instrument group with proper ranges
        InstrumentGroup satbChoir("SATB Choir");
        satbChoir.voices = {VoiceType::Soprano, VoiceType::Alto, VoiceType::Tenor, VoiceType::Bass};
        satbChoir.combinedRange = Range(40, 81);    // E2 to A5 (traditional SATB range)
        satbChoir.preferredStrategy = VoicingStrategy::Close;
        satbChoir.density = 0.7f;                   // Moderate density for clarity
        satbChoir.allowDoubling = true;             // Allow doubling for reinforcement
        
        // Traditional orchestral SATB with proper voice ranges
        InstrumentGroup orchestralSATB("Orchestral SATB");
        orchestralSATB.voices = {VoiceType::Soprano, VoiceType::Alto, VoiceType::Tenor, VoiceType::Bass};
        orchestralSATB.combinedRange = Range(36, 84); // C2 to C6 (orchestral range)
        orchestralSATB.preferredStrategy = VoicingStrategy::Close;
        orchestralSATB.density = 0.6f;             // Conservative density for traditional sound
        orchestralSATB.allowDoubling = false;      // Avoid doubling for voice independence
        
        classicalStyle.instrumentGroups = {satbChoir, orchestralSATB};
        
        // Classical dynamics: Steady, controlled dynamics without excessive swells
        classicalStyle.defaultDynamics.defaultDynamic = 0.7f;
        classicalStyle.defaultDynamics.hasSwells = false;       // Minimal swells in classical style
        classicalStyle.defaultDynamics.hasCrescendos = false;   // Controlled dynamics
        
        return classicalStyle;
    }
    
    inline OrchestrationStyle BasicOrchestrationEngine::createPopStyle()
    {
        OrchestrationStyle popStyle("Pop", OrchestrationStyleType::Pop);
        popStyle.harmonicComplexity = 0.5f;
        popStyle.voiceIndependence = 0.3f;
        popStyle.allowExtensions = true;
        popStyle.preferSpreadVoicings = false;
        popStyle.maxVoiceCount = 5;
        
        // Set pop-specific voicing strategies
        popStyle.voicingStrategies[VoiceType::Soprano] = VoicingStrategy::Close;
        popStyle.voicingStrategies[VoiceType::Alto] = VoicingStrategy::Close;
        popStyle.voicingStrategies[VoiceType::Tenor] = VoicingStrategy::Open;
        popStyle.voicingStrategies[VoiceType::Bass] = VoicingStrategy::Open;
        
        return popStyle;
    }
    
    //==============================================================================
    // Private Helper Methods - SATB Arrangement
    
    inline void BasicOrchestrationEngine::arrangeSATBChord(
        const Chord& chord,
        Voice& soprano,
        Voice& alto,
        Voice& tenor,
        Voice& bass,
        int chordIndex,
        const Key& key)
    {
        if (!chord.isValid() || chord.notes.empty())
        {
            return;
        }
        
        // Extract chord tones
        std::vector<int> chordTones = extractChordTones(chord);
        
        if (chordTones.size() < 3)
        {
            return; // Need at least 3 notes for proper SATB
        }
        
        // Sort chord tones
        std::sort(chordTones.begin(), chordTones.end());
        
        // Calculate note timing
        float startTime = static_cast<float>(chordIndex);
        float duration = 1.0f; // Assume quarter note duration
        
        // Assign bass note (lowest chord tone, in bass range)
        int bassNote = chordTones[0];
        while (bassNote < bass.range.lowestNote)
        {
            bassNote += 12;
        }
        while (bassNote > bass.range.highestNote)
        {
            bassNote -= 12;
        }
        bass.addNote(Note(bassNote, duration, 0.8f, startTime));
        
        // Assign tenor note (next chord tone up, in tenor range)
        int tenorNote = (chordTones.size() > 1) ? chordTones[1] : chordTones[0];
        while (tenorNote < tenor.range.lowestNote)
        {
            tenorNote += 12;
        }
        while (tenorNote > tenor.range.highestNote)
        {
            tenorNote -= 12;
        }
        tenor.addNote(Note(tenorNote, duration, 0.8f, startTime));
        
        // Assign alto note (next chord tone up, in alto range)
        int altoNote = (chordTones.size() > 2) ? chordTones[2] : chordTones[1 % chordTones.size()];
        while (altoNote < alto.range.lowestNote)
        {
            altoNote += 12;
        }
        while (altoNote > alto.range.highestNote)
        {
            altoNote -= 12;
        }
        alto.addNote(Note(altoNote, duration, 0.8f, startTime));
        
        // Assign soprano note (highest chord tone, in soprano range)
        int sopranoNote = chordTones.back();
        while (sopranoNote < soprano.range.lowestNote)
        {
            sopranoNote += 12;
        }
        while (sopranoNote > soprano.range.highestNote)
        {
            sopranoNote -= 12;
        }
        soprano.addNote(Note(sopranoNote, duration, 0.8f, startTime));
    }
    
    //==============================================================================
    // Private Helper Methods - Voice Arrangement Algorithms
    
    inline std::vector<Voice> BasicOrchestrationEngine::arrangeClosePosition(
        const Chord& chord,
        const OrchestrationStyle& style,
        const OrchestrationContext& context)
    {
        std::vector<Voice> voices;
        std::vector<int> chordTones = extractChordTones(chord);
        
        if (chordTones.empty())
        {
            return voices;
        }
        
        // For classical style, use traditional SATB close position rules
        if (style.type == OrchestrationStyleType::Classical)
        {
            return arrangeClassicalSATB(chord, style, context);
        }
        
        int voiceCount = std::min(static_cast<int>(chordTones.size()), style.maxVoiceCount);
        voiceCount = std::max(voiceCount, 3); // Minimum 3 voices for close position
        
        // Sort chord tones for close position arrangement
        std::sort(chordTones.begin(), chordTones.end());
        
        // Create voices with close position spacing (within an octave)
        for (int i = 0; i < voiceCount; ++i)
        {
            VoiceType voiceType = static_cast<VoiceType>(i % 4); // Cycle through SATB
            Voice voice(voiceType);
            
            // Assign note from chord tones
            if (i < static_cast<int>(chordTones.size()))
            {
                int noteToAdd = chordTones[i];
                
                // For close position, keep notes within reasonable range of each other
                if (i > 0 && !voices.empty())
                {
                    int previousNote = voices.back().notes.empty() ? noteToAdd : voices.back().notes.back().midiNumber;
                    
                    // Keep close position spacing (within 12 semitones)
                    while (noteToAdd - previousNote > 12)
                    {
                        noteToAdd -= 12;
                    }
                    while (previousNote - noteToAdd > 12)
                    {
                        noteToAdd += 12;
                    }
                }
                
                // Adjust to voice range
                while (noteToAdd < voice.range.lowestNote)
                {
                    noteToAdd += 12;
                }
                while (noteToAdd > voice.range.highestNote)
                {
                    noteToAdd -= 12;
                }
                
                voice.addNote(Note(noteToAdd, 1.0f, 0.8f, static_cast<float>(context.chordIndex)));
            }
            
            voices.push_back(voice);
        }
        
        return voices;
    }
    
    inline std::vector<Voice> BasicOrchestrationEngine::arrangeOpenPosition(
        const Chord& chord,
        const OrchestrationStyle& style,
        const OrchestrationContext& context)
    {
        std::vector<Voice> voices;
        std::vector<int> chordTones = extractChordTones(chord);
        
        if (chordTones.empty())
        {
            return voices;
        }
        
        int voiceCount = std::min(static_cast<int>(chordTones.size()), style.maxVoiceCount);
        
        // Create voices with wider spacing
        for (int i = 0; i < voiceCount; ++i)
        {
            VoiceType voiceType = static_cast<VoiceType>(i % 4);
            Voice voice(voiceType);
            
            if (i < static_cast<int>(chordTones.size()))
            {
                int noteToAdd = chordTones[i];
                
                // Add octave spacing for open position
                noteToAdd += (i * 7); // Add a fifth for each voice up
                
                // Adjust to voice range
                while (noteToAdd < voice.range.lowestNote)
                {
                    noteToAdd += 12;
                }
                while (noteToAdd > voice.range.highestNote)
                {
                    noteToAdd -= 12;
                }
                
                voice.addNote(Note(noteToAdd, 1.0f, 0.8f, static_cast<float>(context.chordIndex)));
            }
            
            voices.push_back(voice);
        }
        
        return voices;
    }
    
    inline std::vector<Voice> BasicOrchestrationEngine::arrangeDrop2Voicing(
        const Chord& chord,
        const OrchestrationStyle& style,
        const OrchestrationContext& context)
    {
        // Start with close position
        std::vector<Voice> voices = arrangeClosePosition(chord, style, context);
        
        // Drop the second highest voice down an octave
        if (voices.size() >= 2)
        {
            Voice& secondHighest = voices[voices.size() - 2];
            if (!secondHighest.notes.empty())
            {
                secondHighest.notes[0].midiNumber -= 12;
                
                // Ensure it's still in range
                if (secondHighest.notes[0].midiNumber < secondHighest.range.lowestNote)
                {
                    secondHighest.notes[0].midiNumber += 12;
                }
            }
        }
        
        return voices;
    }
    
    //==============================================================================
    // Private Helper Methods - Context Analysis
    
    inline ChordFunction BasicOrchestrationEngine::analyzeChordFunction(
        const Chord& chord,
        const Key& key)
    {
        if (!chord.isValid() || !key.isValid() || chord.notes.empty())
        {
            return ChordFunction::Tonic;
        }
        
        // Simple chord function analysis based on root note
        int chordRoot = chord.rootNote % 12;
        int keyTonic = key.tonicNote % 12;
        
        int interval = (chordRoot - keyTonic + 12) % 12;
        
        switch (interval)
        {
            case 0: return ChordFunction::Tonic;      // I
            case 2: return ChordFunction::Mediant;    // ii
            case 4: return ChordFunction::Mediant;    // iii
            case 5: return ChordFunction::Subdominant; // IV
            case 7: return ChordFunction::Dominant;   // V
            case 9: return ChordFunction::Submediant; // vi
            case 11: return ChordFunction::LeadingTone; // vii
            default: return ChordFunction::Tonic;
        }
    }
    
    inline float BasicOrchestrationEngine::calculateHarmonicTension(
        const Chord& chord,
        const Key& key,
        const OrchestrationContext& context)
    {
        float tension = 0.0f;
        
        // Base tension on chord function
        switch (context.currentFunction)
        {
            case ChordFunction::Tonic: tension = 0.2f; break;
            case ChordFunction::Subdominant: tension = 0.4f; break;
            case ChordFunction::Dominant: tension = 0.8f; break;
            case ChordFunction::Secondary: tension = 0.9f; break;
            default: tension = 0.5f; break;
        }
        
        // Increase tension for dissonant intervals
        std::vector<int> intervals = chord.getIntervals();
        for (int interval : intervals)
        {
            int normalizedInterval = interval % 12;
            if (normalizedInterval == 1 || normalizedInterval == 6 || normalizedInterval == 10)
            {
                tension += 0.1f; // Add tension for minor 2nd, tritone, minor 7th
            }
        }
        
        // Increase tension approaching cadences
        if (context.isCadentialPoint)
        {
            tension += 0.2f;
        }
        
        return std::min(tension, 1.0f);
    }
    
    //==============================================================================
    // Private Helper Methods - Style Adaptation
    
    inline void BasicOrchestrationEngine::adaptComplexityToContext(
        OrchestrationStyle& style,
        const OrchestrationContext& context)
    {
        // Increase complexity at cadential points
        if (context.isCadentialPoint)
        {
            style.harmonicComplexity = std::min(style.harmonicComplexity + 0.2f, 1.0f);
        }
        
        // Adjust complexity based on progression position
        float progressionPosition = context.getProgressionPosition();
        if (progressionPosition > 0.75f) // Near the end
        {
            style.harmonicComplexity = std::min(style.harmonicComplexity + 0.1f, 1.0f);
        }
    }
    
    inline void BasicOrchestrationEngine::adaptVoicingToTension(
        OrchestrationStyle& style,
        float tensionLevel)
    {
        // Use more spread voicings for higher tension
        if (tensionLevel > 0.7f)
        {
            style.preferSpreadVoicings = true;
            
            // Update voicing strategies for high tension
            for (auto& strategy : style.voicingStrategies)
            {
                if (strategy.second == VoicingStrategy::Close)
                {
                    strategy.second = VoicingStrategy::Open;
                }
            }
        }
    }
    
    //==============================================================================
    // Private Helper Methods - Quality Assessment
    
    inline float BasicOrchestrationEngine::assessVoiceLeadingQuality(const std::vector<Voice>& voices)
    {
        if (voices.empty())
        {
            return 0.0f;
        }
        
        float totalSmoothness = 0.0f;
        int validVoices = 0;
        
        for (const auto& voice : voices)
        {
            if (voice.notes.size() > 1)
            {
                totalSmoothness += voiceLeadingEngine->calculateMelodicComplexity(voice);
                validVoices++;
            }
        }
        
        return (validVoices > 0) ? (1.0f - totalSmoothness / validVoices) : 0.0f;
    }
    
    inline float BasicOrchestrationEngine::assessHarmonicBalance(const std::vector<Voice>& voices)
    {
        // Simple assessment based on voice count and range distribution
        if (voices.empty())
        {
            return 0.0f;
        }
        
        // Check if we have good range distribution
        int lowVoices = 0, midVoices = 0, highVoices = 0;
        
        for (const auto& voice : voices)
        {
            if (!voice.notes.empty())
            {
                int avgPitch = voice.notes[0].midiNumber; // Simplified
                if (avgPitch < 55) lowVoices++;
                else if (avgPitch < 70) midVoices++;
                else highVoices++;
            }
        }
        
        // Balanced distribution gets higher score
        float balance = 1.0f;
        if (lowVoices == 0 || highVoices == 0) balance *= 0.7f;
        if (midVoices == 0) balance *= 0.8f;
        
        return balance;
    }
    
    inline float BasicOrchestrationEngine::assessStyleConsistency(
        const OrchestrationResult& result,
        const OrchestrationStyle& targetStyle)
    {
        // Simple consistency check based on voice count
        int actualVoiceCount = static_cast<int>(result.voices.size());
        int targetVoiceCount = targetStyle.maxVoiceCount;

        if (actualVoiceCount <= targetVoiceCount)
            return 1.0f;

        return static_cast<float>(targetVoiceCount) / static_cast<float>(actualVoiceCount);
    }
    
    //==============================================================================
    // Private Helper Methods - Validation
    
    inline bool BasicOrchestrationEngine::validateVoiceRanges(const std::vector<Voice>& voices)
    {
        return std::all_of(voices.begin(), voices.end(),
                           [](const Voice& v) { return v.isValid(); });
    }
    
    inline bool BasicOrchestrationEngine::validateVoiceSpacing(const std::vector<Voice>& voices)
    {
        // Check for reasonable spacing between adjacent voices
        for (size_t i = 1; i < voices.size(); ++i)
        {
            if (!voices[i-1].notes.empty() && !voices[i].notes.empty())
            {
                int interval = std::abs(voices[i].notes[0].midiNumber - voices[i-1].notes[0].midiNumber);
                if (interval > 24) // More than 2 octaves apart
                {
                    return false;
                }
            }
        }
        return true;
    }
    
    inline bool BasicOrchestrationEngine::validateStyleCompliance(
        const std::vector<Voice>& voices,
        const OrchestrationStyle& style)
    {
        // Check voice count compliance
        if (static_cast<int>(voices.size()) > style.maxVoiceCount)
        {
            return false;
        }
        
        return true;
    }
    
    //==============================================================================
    // Private Helper Methods - Utilities
    
    inline std::vector<int> BasicOrchestrationEngine::extractChordTones(const Chord& chord)
    {
        std::vector<int> chordTones;
        chordTones.reserve(chord.notes.size());

        std::transform(chord.notes.begin(), chord.notes.end(), std::back_inserter(chordTones),
                       [](const Note& n) { return n.midiNumber; });

        // Remove duplicates and sort
        std::sort(chordTones.begin(), chordTones.end());
        chordTones.erase(std::unique(chordTones.begin(), chordTones.end()), chordTones.end());

        return chordTones;
    }
    
    inline Range BasicOrchestrationEngine::getVoiceRangeForStyle(
        VoiceType voiceType,
        const OrchestrationStyle& style)
    {
        // Return default ranges (could be customized per style)
        Voice tempVoice(voiceType);
        return tempVoice.range;
    }
    
    //==============================================================================
    // Enhanced Style Blending Helper Methods
    
    inline float BasicOrchestrationEngine::calculateStyleSimilarity(
        const OrchestrationStyle& style1,
        const OrchestrationStyle& style2)
    {
        if (!style1.isValid() || !style2.isValid())
        {
            return 0.0f;
        }
        
        float similarity = 0.0f;
        int comparisonCount = 0;
        
        // Compare numeric parameters
        float complexityDiff = std::abs(style1.harmonicComplexity - style2.harmonicComplexity);
        similarity += (1.0f - complexityDiff);
        comparisonCount++;
        
        float independenceDiff = std::abs(style1.voiceIndependence - style2.voiceIndependence);
        similarity += (1.0f - independenceDiff);
        comparisonCount++;
        
        // Compare boolean parameters
        if (style1.allowExtensions == style2.allowExtensions)
        {
            similarity += 1.0f;
        }
        comparisonCount++;
        
        if (style1.preferSpreadVoicings == style2.preferSpreadVoicings)
        {
            similarity += 1.0f;
        }
        comparisonCount++;
        
        // Compare voice count similarity
        float voiceCountSimilarity = 1.0f - (std::abs(style1.maxVoiceCount - style2.maxVoiceCount) / 16.0f);
        similarity += std::max(0.0f, voiceCountSimilarity);
        comparisonCount++;
        
        return comparisonCount > 0 ? (similarity / static_cast<float>(comparisonCount)) : 0.0f;
    }
    
    inline VoicingStrategy BasicOrchestrationEngine::blendVoicingStrategies(
        const std::vector<VoicingStrategy>& strategies,
        const std::vector<float>& weights)
    {
        if (strategies.empty() || weights.empty() || strategies.size() != weights.size())
        {
            return VoicingStrategy::Close; // Default
        }
        
        // Count weighted occurrences of each strategy
        std::map<VoicingStrategy, float> strategyWeights;
        
        for (size_t i = 0; i < strategies.size(); ++i)
        {
            strategyWeights[strategies[i]] += weights[i];
        }
        
        // Find strategy with highest weighted count
        VoicingStrategy dominantStrategy = VoicingStrategy::Close;
        float maxWeight = 0.0f;
        
        for (const auto& pair : strategyWeights)
        {
            if (pair.second > maxWeight)
            {
                maxWeight = pair.second;
                dominantStrategy = pair.first;
            }
        }
        
        return dominantStrategy;
    }
    
    inline DynamicCurve BasicOrchestrationEngine::blendDynamicCurves(
        const std::vector<DynamicCurve>& curves,
        const std::vector<float>& weights)
    {
        DynamicCurve blendedCurve;
        
        if (curves.empty() || weights.empty() || curves.size() != weights.size())
        {
            return blendedCurve; // Return default
        }
        
        // Normalize weights
        float totalWeight = 0.0f;
        for (float weight : weights)
        {
            totalWeight += std::max(0.0f, weight);
        }
        
        if (totalWeight <= 0.0f)
        {
            return blendedCurve;
        }
        
        // Blend default dynamic
        blendedCurve.defaultDynamic = 0.0f;
        for (size_t i = 0; i < curves.size(); ++i)
        {
            float normalizedWeight = weights[i] / totalWeight;
            blendedCurve.defaultDynamic += curves[i].defaultDynamic * normalizedWeight;
        }
        
        // Blend boolean characteristics (use majority vote weighted by weights)
        float swellsWeight = 0.0f;
        float crescendosWeight = 0.0f;
        
        for (size_t i = 0; i < curves.size(); ++i)
        {
            float normalizedWeight = weights[i] / totalWeight;
            if (curves[i].hasSwells) swellsWeight += normalizedWeight;
            if (curves[i].hasCrescendos) crescendosWeight += normalizedWeight;
        }
        
        blendedCurve.hasSwells = (swellsWeight >= 0.5f);
        blendedCurve.hasCrescendos = (crescendosWeight >= 0.5f);
        
        // Blend dynamic values if available
        size_t maxDynamicValues = 0;
        for (const auto& curve : curves)
        {
            maxDynamicValues = std::max(maxDynamicValues, curve.dynamicValues.size());
        }
        
        if (maxDynamicValues > 0)
        {
            blendedCurve.dynamicValues.resize(maxDynamicValues, blendedCurve.defaultDynamic);
            
            for (size_t valueIndex = 0; valueIndex < maxDynamicValues; ++valueIndex)
            {
                float blendedValue = 0.0f;
                float valueWeight = 0.0f;
                
                for (size_t curveIndex = 0; curveIndex < curves.size(); ++curveIndex)
                {
                    if (valueIndex < curves[curveIndex].dynamicValues.size())
                    {
                        float normalizedWeight = weights[curveIndex] / totalWeight;
                        blendedValue += curves[curveIndex].dynamicValues[valueIndex] * normalizedWeight;
                        valueWeight += normalizedWeight;
                    }
                }
                
                if (valueWeight > 0.0f)
                {
                    blendedCurve.dynamicValues[valueIndex] = blendedValue;
                }
            }
        }
        
        return blendedCurve;
    }
    
    inline std::vector<InstrumentGroup> BasicOrchestrationEngine::blendInstrumentGroups(
        const std::vector<std::vector<InstrumentGroup>>& groupSets,
        const std::vector<float>& weights)
    {
        std::vector<InstrumentGroup> blendedGroups;
        
        if (groupSets.empty() || weights.empty() || groupSets.size() != weights.size())
        {
            // Return default SATB group
            blendedGroups.push_back(InstrumentGroup("Default SATB"));
            return blendedGroups;
        }
        
        // Find the maximum number of groups across all sets
        size_t maxGroups = 0;
        for (const auto& groupSet : groupSets)
        {
            maxGroups = std::max(maxGroups, groupSet.size());
        }
        
        if (maxGroups == 0)
        {
            blendedGroups.push_back(InstrumentGroup("Default SATB"));
            return blendedGroups;
        }
        
        // Normalize weights
        float totalWeight = 0.0f;
        for (float weight : weights)
        {
            totalWeight += std::max(0.0f, weight);
        }
        
        if (totalWeight <= 0.0f)
        {
            blendedGroups.push_back(InstrumentGroup("Default SATB"));
            return blendedGroups;
        }
        
        // Blend each group position
        for (size_t groupIndex = 0; groupIndex < maxGroups; ++groupIndex)
        {
            InstrumentGroup blendedGroup("Blended Group " + std::to_string(static_cast<int>(groupIndex + 1)));
            
            float avgDensity = 0.0f;
            float allowDoublingWeight = 0.0f;
            float groupWeight = 0.0f;
            
            // Collect characteristics from all available groups at this position
            for (size_t setIndex = 0; setIndex < groupSets.size(); ++setIndex)
            {
                if (groupIndex < groupSets[setIndex].size())
                {
                    float normalizedWeight = weights[setIndex] / totalWeight;
                    const InstrumentGroup& group = groupSets[setIndex][groupIndex];
                    
                    avgDensity += group.density * normalizedWeight;
                    if (group.allowDoubling) allowDoublingWeight += normalizedWeight;
                    groupWeight += normalizedWeight;
                    
                    // Use voices from the group with highest weight for this position
                    if (normalizedWeight > 0.0f && blendedGroup.voices.empty())
                    {
                        blendedGroup.voices = group.voices;
                        blendedGroup.combinedRange = group.combinedRange;
                        blendedGroup.preferredStrategy = group.preferredStrategy;
                    }
                }
            }
            
            if (groupWeight > 0.0f)
            {
                blendedGroup.density = avgDensity;
                blendedGroup.allowDoubling = (allowDoublingWeight >= 0.5f);
                blendedGroups.push_back(blendedGroup);
            }
        }
        
        // Ensure we have at least one group
        if (blendedGroups.empty())
        {
            blendedGroups.push_back(InstrumentGroup("Default SATB"));
        }
        
        return blendedGroups;
    }
    
    //==============================================================================
    // Factory Function
    
    std::unique_ptr<OrchestrationEngine> createOrchestrationEngine()
    {
        return std::make_unique<BasicOrchestrationEngine>();
    }

    //==============================================================================
    // Style-Specific Arrangement Methods
    
    inline std::vector<Voice> BasicOrchestrationEngine::arrangeClassicalSATB(
        const Chord& chord,
        const OrchestrationStyle& style,
        const OrchestrationContext& context)
    {
        std::vector<Voice> voices;
        std::vector<int> chordTones = extractChordTones(chord);
        
        if (chordTones.size() < 3)
        {
            return voices; // Need at least 3 notes for proper SATB
        }
        
        // Sort chord tones
        std::sort(chordTones.begin(), chordTones.end());
        
        // Create SATB voices with traditional ranges
        Voice soprano(VoiceType::Soprano);
        Voice alto(VoiceType::Alto);
        Voice tenor(VoiceType::Tenor);
        Voice bass(VoiceType::Bass);
        
        // Traditional SATB ranges (more conservative)
        soprano.range = Range(60, 81);  // C4 to A5
        alto.range = Range(55, 74);     // G3 to D5
        tenor.range = Range(48, 69);    // C3 to A4
        bass.range = Range(40, 62);     // E2 to D4
        
        float startTime = static_cast<float>(context.chordIndex);
        float duration = 1.0f;
        
        // Bass gets the root (lowest chord tone)
        int bassNote = chordTones[0];
        while (bassNote < bass.range.lowestNote)
        {
            bassNote += 12;
        }
        while (bassNote > bass.range.highestNote)
        {
            bassNote -= 12;
        }
        bass.addNote(Note(bassNote, duration, 0.8f, startTime));
        
        // Tenor gets the third or fifth
        int tenorNote = (chordTones.size() > 1) ? chordTones[1] : chordTones[0];
        while (tenorNote < tenor.range.lowestNote)
        {
            tenorNote += 12;
        }
        while (tenorNote > tenor.range.highestNote)
        {
            tenorNote -= 12;
        }
        // Avoid voice crossing with bass
        if (tenorNote <= bassNote)
        {
            tenorNote = bassNote + 3; // Add a minor third minimum
        }
        tenor.addNote(Note(tenorNote, duration, 0.8f, startTime));
        
        // Alto gets the fifth or third
        int altoNote = (chordTones.size() > 2) ? chordTones[2] : chordTones[1 % chordTones.size()];
        while (altoNote < alto.range.lowestNote)
        {
            altoNote += 12;
        }
        while (altoNote > alto.range.highestNote)
        {
            altoNote -= 12;
        }
        // Avoid voice crossing with tenor
        if (altoNote <= tenorNote)
        {
            altoNote = tenorNote + 2; // Add a major second minimum
        }
        alto.addNote(Note(altoNote, duration, 0.8f, startTime));
        
        // Soprano gets the melody note (highest chord tone or root)
        int sopranoNote = chordTones.back();
        while (sopranoNote < soprano.range.lowestNote)
        {
            sopranoNote += 12;
        }
        while (sopranoNote > soprano.range.highestNote)
        {
            sopranoNote -= 12;
        }
        // Avoid voice crossing with alto
        if (sopranoNote <= altoNote)
        {
            sopranoNote = altoNote + 2; // Add a major second minimum
        }
        soprano.addNote(Note(sopranoNote, duration, 0.8f, startTime));
        
        voices = {soprano, alto, tenor, bass};
        return voices;
    }
    
    inline std::vector<Voice> BasicOrchestrationEngine::arrangeJazzExtendedHarmony(
        const Chord& chord,
        const OrchestrationStyle& style,
        const OrchestrationContext& context)
    {
        std::vector<Voice> voices;
        std::vector<int> chordTones = extractChordTones(chord);
        
        if (chordTones.empty())
        {
            return voices;
        }
        
        // Add jazz extensions (9th, 11th, 13th) if allowed
        std::vector<int> extendedTones = chordTones;
        if (style.allowExtensions && chordTones.size() >= 3)
        {
            int root = chordTones[0];
            
            // Add 9th (2 semitones above octave)
            extendedTones.push_back(root + 14);
            
            // Add 11th (5 semitones above octave) - avoid if it conflicts with 3rd
            bool hasThird = std::find(chordTones.begin(), chordTones.end(), root + 4) != chordTones.end() ||
                           std::find(chordTones.begin(), chordTones.end(), root + 3) != chordTones.end();
            if (!hasThird)
            {
                extendedTones.push_back(root + 17);
            }
            
            // Add 13th (9 semitones above octave)
            extendedTones.push_back(root + 21);
        }
        
        // Sort extended tones
        std::sort(extendedTones.begin(), extendedTones.end());
        
        // Create up to 6 voices for jazz extended harmony
        int voiceCount = std::min(static_cast<int>(extendedTones.size()), 6);
        
        for (int i = 0; i < voiceCount; ++i)
        {
            VoiceType voiceType = static_cast<VoiceType>(i % 4);
            Voice voice(voiceType);
            
            // Use wider ranges for jazz
            switch (voiceType)
            {
                case VoiceType::Soprano: voice.range = Range(60, 96); break;  // C4 to C7
                case VoiceType::Alto: voice.range = Range(55, 84); break;     // G3 to C6
                case VoiceType::Tenor: voice.range = Range(48, 79); break;    // C3 to G5
                case VoiceType::Bass: voice.range = Range(36, 72); break;     // C2 to C5
            }
            
            if (i < static_cast<int>(extendedTones.size()))
            {
                int noteToAdd = extendedTones[i];
                
                // Apply drop-2 voicing for jazz characteristic sound
                if (i == voiceCount - 2 && voiceCount >= 4)
                {
                    noteToAdd -= 12; // Drop second highest voice an octave
                }
                
                // Adjust to voice range
                while (noteToAdd < voice.range.lowestNote)
                {
                    noteToAdd += 12;
                }
                while (noteToAdd > voice.range.highestNote)
                {
                    noteToAdd -= 12;
                }
                
                voice.addNote(Note(noteToAdd, 1.0f, 0.8f, static_cast<float>(context.chordIndex)));
            }
            
            voices.push_back(voice);
        }
        
        return voices;
    }
    
    inline std::vector<Voice> BasicOrchestrationEngine::arrangeCinematicPadVoicing(
        const Chord& chord,
        const OrchestrationStyle& style,
        const OrchestrationContext& context)
    {
        std::vector<Voice> voices;
        std::vector<int> chordTones = extractChordTones(chord);
        
        if (chordTones.empty())
        {
            return voices;
        }
        
        // Add lush extensions for cinematic sound
        std::vector<int> lushTones = chordTones;
        if (style.allowExtensions && chordTones.size() >= 3)
        {
            int root = chordTones[0];
            
            // Add 9th for lush sound
            lushTones.push_back(root + 14);
            
            // Add 6th for sweetness
            lushTones.push_back(root + 9);
            
            // Add octave doubling for fullness
            for (int tone : chordTones)
            {
                lushTones.push_back(tone + 12);
            }
        }
        
        // Sort tones
        std::sort(lushTones.begin(), lushTones.end());
        
        // Create up to 8 voices for cinematic divisi
        int voiceCount = std::min(static_cast<int>(lushTones.size()), 8);
        
        // Create string section ranges
        std::vector<Range> stringRanges = {
            Range(55, 96),  // Violins I (G3 to C7)
            Range(55, 91),  // Violins I divisi (G3 to G6)
            Range(48, 84),  // Violins II (C3 to C6)
            Range(48, 79),  // Violins II divisi (C3 to G5)
            Range(48, 79),  // Violas (C3 to G5)
            Range(43, 74),  // Violas divisi (G2 to D5)
            Range(36, 72),  // Cellos (C2 to C5)
            Range(36, 67)   // Cellos divisi (C2 to G4)
        };
        
        for (int i = 0; i < voiceCount; ++i)
        {
            VoiceType voiceType = static_cast<VoiceType>(i % 4);
            Voice voice(voiceType);
            
            // Use string section ranges
            if (i < static_cast<int>(stringRanges.size()))
            {
                voice.range = stringRanges[i];
            }
            
            if (i < static_cast<int>(lushTones.size()))
            {
                int noteToAdd = lushTones[i];
                
                // Spread voicing for cinematic breadth
                if (i > 0 && style.preferSpreadVoicings)
                {
                    // Add octave spacing for spread voicing
                    noteToAdd += (i / 2) * 12;
                }
                
                // Adjust to voice range
                while (noteToAdd < voice.range.lowestNote)
                {
                    noteToAdd += 12;
                }
                while (noteToAdd > voice.range.highestNote)
                {
                    noteToAdd -= 12;
                }
                
                // Dynamic swells for cinematic expression
                float dynamic = style.defaultDynamics.getDynamicAtPosition(context.chordIndex);
                
                voice.addNote(Note(noteToAdd, 1.0f, dynamic, static_cast<float>(context.chordIndex)));
            }
            
            voices.push_back(voice);
        }
        
        return voices;
    }

} // namespace HarmonyEngine
