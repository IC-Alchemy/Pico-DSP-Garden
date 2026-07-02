/*
  ==============================================================================

    MusicTheory.h
    Created: Core music theory data structures and interfaces
    
    This file contains the fundamental music theory data structures including
    Note, Chord, Key, Voice, and related enums and validation functions.

  ==============================================================================
*/

#pragma once

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <algorithm>

namespace HarmonyEngine
{
    //==============================================================================
    // Enums and Constants

    // Pitch-class name lookup, indexed by midiNumber % 12.
    static const char* const kNoteNames[12] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };

    enum class NoteClass
    {
        C = 0, CSharp = 1, D = 2, DSharp = 3, E = 4, F = 5,
        FSharp = 6, G = 7, GSharp = 8, A = 9, ASharp = 10, B = 11
    };
    
    enum class ScaleType
    {
        // Major scale modes
        Major,          // Ionian (1st mode)
        Dorian,         // 2nd mode of major
        Phrygian,       // 3rd mode of major
        Lydian,         // 4th mode of major
        Mixolydian,     // 5th mode of major
        Aeolian,        // 6th mode of major (natural minor)
        Locrian,        // 7th mode of major
        
        // Minor scales
        NaturalMinor,   // Same as Aeolian
        HarmonicMinor,  // Harmonic minor scale
        MelodicMinor,   // Melodic minor scale
        
        // Harmonic minor modes
        HarmonicMinorMode1,  // Harmonic minor (1st mode)
        LocrianNat6,         // 2nd mode of harmonic minor
        IonianAug5,          // 3rd mode of harmonic minor
        DorianSharp4,        // 4th mode of harmonic minor
        PhrygianDominant,    // 5th mode of harmonic minor
        LydianSharp2,        // 6th mode of harmonic minor
        SuperLocrianDim7     // 7th mode of harmonic minor
    };
    
    enum class ChordType
    {
        Major,
        Minor,
        Diminished,
        Augmented,
        Major7,
        Minor7,
        Dominant7,
        Diminished7,
        HalfDiminished7,
        MinorMajor7,
        Augmented7,
        Sus2,
        Sus4,
        Add9,
        Major9,
        Minor9,
        Dominant9
    };
    
    enum class Inversion
    {
        Root = 0,
        First = 1,
        Second = 2,
        Third = 3
    };
    
    enum class VoiceType
    {
        Soprano,
        Alto,
        Tenor,
        Bass,
        Lead,
        Harmony1,
        Harmony2,
        Harmony3
    };
    
    enum class ChordFunction
    {
        Tonic,
        Subdominant,
        Dominant,
        Predominant,
        Mediant,
        Submediant,
        LeadingTone,
        Secondary
    };
    
    //==============================================================================
    // Core Data Structures
    
    struct Range
    {
        int lowestNote;
        int highestNote;
        
        Range(int low = 0, int high = 127) : lowestNote(low), highestNote(high) {}
        
        bool contains(int midiNote) const
        {
            return midiNote >= lowestNote && midiNote <= highestNote;
        }
        
        bool isValid() const
        {
            return lowestNote >= 0 && highestNote <= 127 && lowestNote <= highestNote;
        }
    };
    
    struct Note
    {
        int midiNumber;
        float duration;
        float velocity;
        float startTime;
        
        Note(int midi = 60, float dur = 1.0f, float vel = 0.8f, float start = 0.0f)
            : midiNumber(midi), duration(dur), velocity(vel), startTime(start) {}
        
        bool isValid() const
        {
            return midiNumber >= 0 && midiNumber <= 127 &&
                   duration > 0.0f &&
                   velocity >= 0.0f && velocity <= 1.0f &&
                   startTime >= 0.0f;
        }
        
        NoteClass getNoteClass() const
        {
            return static_cast<NoteClass>(midiNumber % 12);
        }
        
        int getOctave() const
        {
            return (midiNumber / 12) - 1;
        }
        
        std::string getNoteName() const
        {
            return kNoteNames[midiNumber % 12] + std::to_string(getOctave());
        }
        
        bool operator==(const Note& other) const
        {
            return midiNumber == other.midiNumber &&
                   std::abs(duration - other.duration) < 0.001f &&
                   std::abs(velocity - other.velocity) < 0.001f &&
                   std::abs(startTime - other.startTime) < 0.001f;
        }
    };
    
    struct Chord
    {
        std::vector<Note> notes;
        ChordType type;
        int rootNote;
        std::vector<int> extensions;
        Inversion inversion;
        ChordFunction function;
        
        Chord(ChordType chordType = ChordType::Major, int root = 60)
            : type(chordType), rootNote(root), inversion(Inversion::Root), function(ChordFunction::Tonic) {}
        
        bool isValid() const
        {
            if (notes.size() < 2 || notes.size() > 8)
                return false;

            if (rootNote < 0 || rootNote > 127)
                return false;

            return std::all_of(notes.begin(), notes.end(),
                [](const Note& n) { return n.isValid(); });
        }
        
        bool containsNote(int midiNumber) const
        {
            int pc = midiNumber % 12;
            return std::any_of(notes.begin(), notes.end(),
                [pc](const Note& n) { return n.midiNumber % 12 == pc; });
        }
        
        std::vector<int> getIntervals() const
        {
            std::vector<int> intervals;
            if (notes.empty()) return intervals;
            
            int bassNote = notes[0].midiNumber;
            for (size_t i = 1; i < notes.size(); ++i)
            {
                intervals.push_back(notes[i].midiNumber - bassNote);
            }
            return intervals;
        }
        
        std::string getChordSymbol() const
        {
            std::string symbol = kNoteNames[rootNote % 12];
            
            switch (type)
            {
                case ChordType::Major: break; // No suffix for major
                case ChordType::Minor: symbol += "m"; break;
                case ChordType::Diminished: symbol += "dim"; break;
                case ChordType::Augmented: symbol += "aug"; break;
                case ChordType::Major7: symbol += "maj7"; break;
                case ChordType::Minor7: symbol += "m7"; break;
                case ChordType::Dominant7: symbol += "7"; break;
                case ChordType::Diminished7: symbol += "dim7"; break;
                case ChordType::HalfDiminished7: symbol += "m7b5"; break;
                case ChordType::Sus2: symbol += "sus2"; break;
                case ChordType::Sus4: symbol += "sus4"; break;
                default: break;
            }
            
            return symbol;
        }
    };
    
    struct Key
    {
        int tonicNote;
        ScaleType scaleType;
        std::vector<int> scalePattern;
        
        Key(int tonic = 0, ScaleType scale = ScaleType::Major)
            : tonicNote(tonic), scaleType(scale)
        {
            generateScalePattern();
        }
        
        bool isValid() const
        {
            return tonicNote >= 0 && tonicNote < 12 && 
                   scalePattern.size() >= 5 && scalePattern.size() <= 12;
        }
        
        std::vector<int> getScaleDegrees() const
        {
            std::vector<int> degrees;
            for (int degree : scalePattern)
            {
                degrees.push_back((tonicNote + degree) % 12);
            }
            return degrees;
        }
        
        // A pitch class is in the key iff it equals (tonic + scale degree) mod 12
        // for some scale degree. Equivalent to getScaleDegrees() membership but
        // avoids allocating a vector per call.
        bool containsNote(int noteClass) const
        {
            int target = noteClass % 12;
            return std::any_of(scalePattern.begin(), scalePattern.end(),
                [target, this](int degree) {
                    return (tonicNote + degree) % 12 == target;
                });
        }
        
        std::string getKeySignature() const
        {
            std::string keyName = kNoteNames[tonicNote];
            
            switch (scaleType)
            {
                // Major scale modes
                case ScaleType::Major: break;
                case ScaleType::Dorian: keyName += " dorian"; break;
                case ScaleType::Phrygian: keyName += " phrygian"; break;
                case ScaleType::Lydian: keyName += " lydian"; break;
                case ScaleType::Mixolydian: keyName += " mixolydian"; break;
                case ScaleType::Aeolian: keyName += " aeolian"; break;
                case ScaleType::Locrian: keyName += " locrian"; break;
                
                // Minor scales
                case ScaleType::NaturalMinor: keyName += " minor"; break;
                case ScaleType::HarmonicMinor: 
                case ScaleType::HarmonicMinorMode1: keyName += " harmonic minor"; break;
                case ScaleType::MelodicMinor: keyName += " melodic minor"; break;
                
                // Harmonic minor modes
                case ScaleType::LocrianNat6: keyName += " locrian ♮6"; break;
                case ScaleType::IonianAug5: keyName += " ionian #5"; break;
                case ScaleType::DorianSharp4: keyName += " dorian #4"; break;
                case ScaleType::PhrygianDominant: keyName += " phrygian dominant"; break;
                case ScaleType::LydianSharp2: keyName += " lydian #2"; break;
                case ScaleType::SuperLocrianDim7: keyName += " super locrian dim7"; break;
                
                default: keyName += " (unknown mode)"; break;
            }
            
            return keyName;
        }
        
    private:
        void generateScalePattern()
        {
            scalePattern.clear();
            
            switch (scaleType)
            {
                // Major scale modes
                case ScaleType::Major:
                    scalePattern = {0, 2, 4, 5, 7, 9, 11};
                    break;
                case ScaleType::Dorian:
                    scalePattern = {0, 2, 3, 5, 7, 9, 10};
                    break;
                case ScaleType::Phrygian:
                    scalePattern = {0, 1, 3, 5, 7, 8, 10};
                    break;
                case ScaleType::Lydian:
                    scalePattern = {0, 2, 4, 6, 7, 9, 11};
                    break;
                case ScaleType::Mixolydian:
                    scalePattern = {0, 2, 4, 5, 7, 9, 10};
                    break;
                case ScaleType::Aeolian:
                case ScaleType::NaturalMinor:
                    scalePattern = {0, 2, 3, 5, 7, 8, 10};
                    break;
                case ScaleType::Locrian:
                    scalePattern = {0, 1, 3, 5, 6, 8, 10};
                    break;
                    
                // Minor scales
                case ScaleType::HarmonicMinor:
                case ScaleType::HarmonicMinorMode1:
                    scalePattern = {0, 2, 3, 5, 7, 8, 11};
                    break;
                case ScaleType::MelodicMinor:
                    scalePattern = {0, 2, 3, 5, 7, 9, 11};
                    break;
                    
                // Harmonic minor modes
                case ScaleType::LocrianNat6:
                    scalePattern = {0, 1, 3, 5, 6, 9, 10};
                    break;
                case ScaleType::IonianAug5:
                    scalePattern = {0, 2, 4, 5, 8, 9, 11};
                    break;
                case ScaleType::DorianSharp4:
                    scalePattern = {0, 2, 3, 6, 7, 9, 10};
                    break;
                case ScaleType::PhrygianDominant:
                    scalePattern = {0, 1, 4, 5, 7, 8, 10};
                    break;
                case ScaleType::LydianSharp2:
                    scalePattern = {0, 3, 4, 6, 7, 9, 11};
                    break;
                case ScaleType::SuperLocrianDim7:
                    scalePattern = {0, 1, 3, 4, 6, 8, 9};
                    break;
            }
        }
    };
    
    struct Voice
    {
        std::vector<Note> notes;
        VoiceType type;
        Range range;
        
        Voice(VoiceType voiceType = VoiceType::Soprano)
            : type(voiceType)
        {
            setDefaultRange();
        }
        
        bool isValid() const
        {
            if (!range.isValid())
                return false;

            return std::all_of(notes.begin(), notes.end(),
                [this](const Note& n) {
                    return n.isValid() && range.contains(n.midiNumber);
                });
        }
        
        void addNote(const Note& note)
        {
            if (range.contains(note.midiNumber))
            {
                notes.push_back(note);
            }
        }
        
        float getDuration() const
        {
            if (notes.empty()) return 0.0f;
            
            float maxEndTime = 0.0f;
            for (const auto& note : notes)
            {
                maxEndTime = std::max(maxEndTime, note.startTime + note.duration);
            }
            return maxEndTime;
        }
        
        std::vector<int> getMelodicIntervals() const
        {
            std::vector<int> intervals;
            for (size_t i = 1; i < notes.size(); ++i)
            {
                intervals.push_back(notes[i].midiNumber - notes[i-1].midiNumber);
            }
            return intervals;
        }
        
    private:
        void setDefaultRange()
        {
            switch (type)
            {
                case VoiceType::Soprano:
                    range = Range(60, 84); // C4 to C6
                    break;
                case VoiceType::Alto:
                    range = Range(53, 77); // F3 to F5
                    break;
                case VoiceType::Tenor:
                    range = Range(48, 72); // C3 to C5
                    break;
                case VoiceType::Bass:
                    range = Range(40, 64); // E2 to E4
                    break;
                default:
                    range = Range(36, 96); // C2 to C7 (wide range for other voice types)
                    break;
            }
        }
    };
    
} // namespace HarmonyEngine