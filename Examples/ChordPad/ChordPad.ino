// ChordPad.ino
// ---------------------------------------------------------------------------
// Four-voice chord pad — HarmonyEngine + Pico-DSP-Garden (rpdsp).
//
// HarmonyEngine defines a chord progression as Chord objects (root pitch
// class + ChordType). A small helper expands each ChordType into its interval
// pattern to derive four chord-tone MIDI notes. Core 1 steps the progression
// and triggers four rpdsp::TriggeredSynthVoice instances; Core 0 mixes all
// four voices into the stereo I2S buffer. Slow ADSR gives a sustained
// string-pad feel.
//
// The integration seam is:
//     HarmonyEngine::Chord (ChordType)  ->  interval pattern -> MIDI notes
//         -> rpdsp::TriggeredSynthVoice::noteOn(midi)
//             -> voice::process()  (subtracted saw + ADSR + filter)
//
// Dual-core contract (see Docs/realtime_rules.md):
//   Core 0 = real-time audio fill (never blocks, no allocation).
//   Core 1 = HarmonyEngine sequencing (blocking delay() is fine here).
//

// Library discovery triggers — arduino-cli detects libraries via root-level
// src/ headers; once loaded, src/ is on the include path.
#include <rpdsp.h>
#include <pico_audio_i2s.h>
#include <HarmonyEngine.h>

#include <pico_audio_i2s/audio.h>
#include <pico_audio_i2s/audio_i2s.h>
#include <rpdsp/voice.h>
#include <HarmonyEngine/MusicTheory.h>

// ---------------------------------------------------------------------------
// I2S pin assignment (see AGENTS.md wiring convention)
// ---------------------------------------------------------------------------
static const int PICO_AUDIO_I2S_DATA_PIN       = 15;
static const int PICO_AUDIO_I2S_CLOCK_PIN_BASE = 16; // LRCK = 16, BCLK = 17

// ---------------------------------------------------------------------------
// Audio engine constants
// ---------------------------------------------------------------------------
static const float    SAMPLE_RATE        = 48000.0f;
static const int      NUM_AUDIO_BUFFERS  = 3;
static const int      SAMPLES_PER_BUFFER = 256;

static audio_buffer_pool_t *producer_pool = nullptr;

// ---------------------------------------------------------------------------
// DSP objects — four subtractive synth voices (SATB-ish ensemble)
// ---------------------------------------------------------------------------
using Voice = rpdsp::TriggeredSynthVoice<3>;  // 3 saw oscillators each
static const int NUM_VOICES = 4;
static Voice g_voices[NUM_VOICES];

// ---------------------------------------------------------------------------
// HarmonyEngine — chord progression
// ---------------------------------------------------------------------------
// Each entry is a HarmonyEngine Chord (rootNote + ChordType). rootNote here is
// a pitch class (0..11) used as the chord root; voices are octave-shifted
// below to taste. The progression is I - vi - IV - V in C major.
static const HarmonyEngine::Chord PROGRESSION[] = {
    HarmonyEngine::Chord(HarmonyEngine::ChordType::Major,     0),  // C  (I)
    HarmonyEngine::Chord(HarmonyEngine::ChordType::Minor,     9),  // Am (vi)
    HarmonyEngine::Chord(HarmonyEngine::ChordType::Major,     5),  // F  (IV)
    HarmonyEngine::Chord(HarmonyEngine::ChordType::Dominant7, 7)   // G7 (V)
};
static const int PROGRESSION_LENGTH = sizeof(PROGRESSION) / sizeof(PROGRESSION[0]);

// Maximum chord tones we ever derive from a ChordType.
static const int MAX_CHORD_TONES = 4;

// ---------------------------------------------------------------------------
// Expand a HarmonyEngine ChordType into its semitone interval pattern.
// This is the musical knowledge HarmonyEngine's Chord/ChordType encodes;
// ChordAnalyzer only *analyzes* chords, so we keep this tiny generator local.
// Returns the number of tones written (1..MAX_CHORD_TONES).
// ---------------------------------------------------------------------------
static int chordIntervals(HarmonyEngine::ChordType type, int out[MAX_CHORD_TONES])
{
    switch (type)
    {
        case HarmonyEngine::ChordType::Major:           out[0]=0; out[1]=4; out[2]=7;          return 3;
        case HarmonyEngine::ChordType::Minor:           out[0]=0; out[1]=3; out[2]=7;          return 3;
        case HarmonyEngine::ChordType::Diminished:      out[0]=0; out[1]=3; out[2]=6;          return 3;
        case HarmonyEngine::ChordType::Augmented:       out[0]=0; out[1]=4; out[2]=8;          return 3;
        case HarmonyEngine::ChordType::Sus2:            out[0]=0; out[1]=2; out[2]=7;          return 3;
        case HarmonyEngine::ChordType::Sus4:            out[0]=0; out[1]=5; out[2]=7;          return 3;
        case HarmonyEngine::ChordType::Major7:          out[0]=0; out[1]=4; out[2]=7; out[3]=11; return 4;
        case HarmonyEngine::ChordType::Minor7:          out[0]=0; out[1]=3; out[2]=7; out[3]=10; return 4;
        case HarmonyEngine::ChordType::Dominant7:       out[0]=0; out[1]=4; out[2]=7; out[3]=10; return 4;
        case HarmonyEngine::ChordType::Diminished7:     out[0]=0; out[1]=3; out[2]=6; out[3]=9;  return 4;
        case HarmonyEngine::ChordType::HalfDiminished7: out[0]=0; out[1]=3; out[2]=6; out[3]=10; return 4;
        case HarmonyEngine::ChordType::MinorMajor7:     out[0]=0; out[1]=3; out[2]=7; out[3]=11; return 4;
        default:                                        out[0]=0; out[1]=4; out[2]=7;          return 3;
    }
}

// ---------------------------------------------------------------------------
// Trigger one chord: derive its tones and re-voice all four synths.
// Octave offsets give a SATB-ish spread (bass low, soprano high).
// ---------------------------------------------------------------------------
static void triggerChord(const HarmonyEngine::Chord& chord)
{
    int intervals[MAX_CHORD_TONES];
    int toneCount = chordIntervals(chord.type, intervals);
    if (toneCount > NUM_VOICES) toneCount = NUM_VOICES;

    // Per-voice octave offsets so a triad spreads across SATB registers.
    // Voices beyond the chord-tone count double the root (bass-like).
    static const int octaveShift[NUM_VOICES] = { -12, 0, 0, +12 }; // bass, tenor, alto, soprano

    for (int v = 0; v < NUM_VOICES; ++v)
    {
        int toneIdx = (v < toneCount) ? v : 0;  // extras double the root
        int midi = chord.rootNote + 48 + intervals[toneIdx] + octaveShift[v];
        // Clamp into a sane MIDI range.
        if (midi < 24)  midi = 24;
        if (midi > 96)  midi = 96;

        g_voices[v].noteOn(midi, 0.8f, 0);
    }
}

// ---------------------------------------------------------------------------
// Initialise DSP voices (called once from Core 0 setup)
// ---------------------------------------------------------------------------
static void initVoices()
{
    // Use the library's built-in three-saw subtractive preset — a warm pad
    // source. Each voice gets its own instance so chords are truly polyphonic.
    rpdsp::TriggeredSynthVoicePreset<3> preset = rpdsp::classicThreeSawSubtractivePreset();

    // Slow attack/release for a sustained string-pad feel.
    preset.ampEnvelope.attackSeconds  = 0.45f;
    preset.ampEnvelope.releaseSeconds = 0.60f;
    preset.ampEnvelope.sustain        = 0.7f;
    preset.gain = 0.18f;  // leave headroom when 4 voices sum

    for (int v = 0; v < NUM_VOICES; ++v)
    {
        g_voices[v].prepare(SAMPLE_RATE);
        g_voices[v].applyPreset(preset);
    }
}

// ---------------------------------------------------------------------------
// Audio fill callback — Core 0 hot path; must not block
// ---------------------------------------------------------------------------
static void fill_audio_buffer(audio_buffer_t *buffer)
{
    const int    N   = static_cast<int>(buffer->max_sample_count);
    int32_t     *out = reinterpret_cast<int32_t *>(buffer->buffer->bytes);

    for (int i = 0; i < N; ++i)
    {
        // Sum the four voices. process() returns 0 when a voice is idle, so
        // releasing voices naturally drop out of the mix.
        float mixed = 0.0f;
        for (int v = 0; v < NUM_VOICES; ++v)
        {
            mixed += g_voices[v].process();
        }

        int32_t s = rpdsp::toInt24x32(mixed);
        out[2 * i + 0] = s; // Left
        out[2 * i + 1] = s; // Right
    }

    buffer->sample_count = N;
}

// ---------------------------------------------------------------------------
// I2S audio setup helper
// ---------------------------------------------------------------------------
static void setupI2SAudio(audio_format_t *audioFmt, audio_i2s_config_t *i2sCfg)
{
    if (!audio_i2s_setup(audioFmt, i2sCfg))
    {
        Serial.println("[CORE0] audio_i2s_setup failed!");
        return;
    }
    if (!audio_i2s_connect(producer_pool))
    {
        Serial.println("[CORE0] audio_i2s_connect failed!");
        return;
    }
    audio_i2s_set_enabled(true);
    Serial.println("[CORE0] I2S audio started.");
}

// ---------------------------------------------------------------------------
// Core 0 — setup & loop (real-time audio)
// ---------------------------------------------------------------------------
void setup()
{
    delay(150); // let power rails settle

    initVoices();

    static audio_format_t audioFmt = {
        .sample_freq   = static_cast<uint32_t>(SAMPLE_RATE),
        .format        = AUDIO_BUFFER_FORMAT_PCM_S32,
        .channel_count = 2
    };
    static audio_buffer_format_t bufFmt = {
        .format        = &audioFmt,
        .sample_stride = 8           // 2 channels x 4 bytes/sample (24-in-32)
    };

    producer_pool = audio_new_producer_pool(&bufFmt, NUM_AUDIO_BUFFERS, SAMPLES_PER_BUFFER);

    audio_i2s_config_t i2sCfg = {
        .data_pin       = PICO_AUDIO_I2S_DATA_PIN,
        .clock_pin_base = PICO_AUDIO_I2S_CLOCK_PIN_BASE,
        .dma_channel    = 0,
        .pio_sm         = 0
    };

    setupI2SAudio(&audioFmt, &i2sCfg);
}

void loop()
{
    // Real-time audio fill — Core 0 must never block here
    audio_buffer_t *buf = take_audio_buffer(producer_pool, true);
    if (buf)
    {
        fill_audio_buffer(buf);
        give_audio_buffer(producer_pool, buf);
    }
}

// ---------------------------------------------------------------------------
// Core 1 — setup1 & loop1 (non-real-time: HarmonyEngine sequencing)
// ---------------------------------------------------------------------------
void setup1()
{
    delay(200);
    Serial.begin(115200);
    Serial.println("[CORE1] ChordPad — HarmonyEngine + rpdsp");
    Serial.print  ("[CORE1] Progression length: ");
    Serial.println(PROGRESSION_LENGTH);
}

void loop1()
{
    // Step through the HarmonyEngine chord progression. Each chord rings for
    // ~2.2 s (attack + sustain), then releases as the next triggers.
    for (int i = 0; i < PROGRESSION_LENGTH; ++i)
    {
        // Release any sustaining voices first so the chord change breathes.
        for (int v = 0; v < NUM_VOICES; ++v)
        {
            g_voices[v].noteOff();
        }

        // Small gap lets the release tail fade before the next attack.
        delay(120);

        // Trigger the next chord — HarmonyEngine Chord -> rpdsp voices.
        triggerChord(PROGRESSION[i]);

        // Let the chord sustain. Blocking delay is fine on Core 1.
        delay(2000);
    }
}
