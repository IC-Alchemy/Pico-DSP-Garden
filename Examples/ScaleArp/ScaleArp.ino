// ScaleArp.ino
// ---------------------------------------------------------------------------
// Scale/mode-aware arpeggiator — HarmonyEngine + Pico-DSP-Garden (rpdsp).
//
// HarmonyEngine builds a Key (tonic + ScaleType); its scalePattern (semitone
// offsets within an octave) is expanded into MIDI notes. Core 1 walks an
// up-and-down arpeggio across those notes and rotates through several
// scale/mode families. Core 0 renders each note with a rpdsp Hypersaw.
//
// The integration seam is:
//     HarmonyEngine::Key  ->  MIDI note numbers  ->  rpdsp::midiNoteToHz()
//                                                     -> rpdsp::Hypersaw
//
// Dual-core contract (see Docs/realtime_rules.md):
//   Core 0 = real-time audio fill (never blocks).
//   Core 1 = HarmonyEngine sequencing (blocking delay() is fine here).
//

// Library discovery triggers — arduino-cli detects libraries via root-level
// src/ headers; once loaded, src/ is on the include path.
#include <rpdsp.h>
#include <pico_audio_i2s.h>
#include <HarmonyEngine.h>

#include <pico_audio_i2s/audio.h>
#include <pico_audio_i2s/audio_i2s.h>
#include <rpdsp/hypersaw.h>
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
// DSP objects
// ---------------------------------------------------------------------------
static rpdsp::Hypersaw hypersaw;

// ---------------------------------------------------------------------------
// HarmonyEngine — scale/mode catalogue
// ---------------------------------------------------------------------------
// Each entry pairs a tonic pitch class (0..11) with a scale type. Core 1
// cycles through these so the demo exercises several mode families.
struct ScaleEntry {
    HarmonyEngine::NoteClass  tonic;
    HarmonyEngine::ScaleType  scale;
};

static const ScaleEntry SCALES[] = {
    { HarmonyEngine::NoteClass::C,      HarmonyEngine::ScaleType::Major            }, // C  Ionian
    { HarmonyEngine::NoteClass::A,      HarmonyEngine::ScaleType::Aeolian          }, // A  natural minor
    { HarmonyEngine::NoteClass::D,      HarmonyEngine::ScaleType::Dorian           }, // D  dorian
    { HarmonyEngine::NoteClass::E,      HarmonyEngine::ScaleType::PhrygianDominant }  // E  phrygian dominant
};
static const int SCALE_COUNT = sizeof(SCALES) / sizeof(SCALES[0]);

// Maximum notes in one octave-plus-one pattern: 8 (7 scale degrees + octave).
static const int MAX_NOTES = 8;

// Resolved MIDI notes for the current scale, set up on Core 1.
static int  g_scaleNotes[MAX_NOTES];
static int  g_scaleLength = 0;

// ---------------------------------------------------------------------------
// Core-safe handoff: Core 1 writes, Core 0 reads.
// ---------------------------------------------------------------------------
volatile int g_currentMidiNote = 60;  // MIDI note currently being rendered

// ---------------------------------------------------------------------------
// Build the up-and-down arpeggio for the given key into g_scaleNotes.
// Pattern: degree 0,1,2,...,6,7(octave),6,5,...,1 -> 15 steps, 8 unique notes.
// We only store the unique notes here; the walk index is computed in loop1().
// ---------------------------------------------------------------------------
static void buildScaleNotes(const ScaleEntry& entry)
{
    // Key generates the semitone offsets for its scale type internally.
    HarmonyEngine::Key key(static_cast<int>(entry.tonic), entry.scale);

    g_scaleLength = 0;
    for (int degree : key.scalePattern)
    {
        if (g_scaleLength >= MAX_NOTES) break;
        // Place the scale around MIDI 60 (C4): one octave up from the tonic.
        // entry.tonic is a pitch class (0..11); +12 puts us at C4-ish register.
        g_scaleNotes[g_scaleLength++] = static_cast<int>(entry.tonic) + 48 + degree;
    }
    // Append the octave to give the arp a satisfying peak.
    if (g_scaleLength > 0 && g_scaleLength < MAX_NOTES)
    {
        g_scaleNotes[g_scaleLength++] = g_scaleNotes[0] + 12;
    }
}

// ---------------------------------------------------------------------------
// Initialise DSP objects (called once from Core 0 setup)
// ---------------------------------------------------------------------------
static void initHypersaw()
{
    hypersaw.prepare(SAMPLE_RATE);
    hypersaw.setFreq(rpdsp::midiNoteToHz(static_cast<float>(g_currentMidiNote)));
    hypersaw.setDetune(0.381f);
    hypersaw.setMix(0.5f);
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
        float mixed_signal = hypersaw.process();
        mixed_signal *= 0.33f;

        int32_t s = rpdsp::toInt24x32(mixed_signal);
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

    initHypersaw();

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
    Serial.println("[CORE1] ScaleArp — HarmonyEngine + rpdsp");

    // Prime the first scale before the loop starts.
    buildScaleNotes(SCALES[0]);
    Serial.print  ("[CORE1] Scale 0 notes: ");
    Serial.println(g_scaleLength);
}

void loop1()
{
    // Cycle through the scale/mode catalogue. Each scale plays for a while.
    for (int s = 0; s < SCALE_COUNT; ++s)
    {
        buildScaleNotes(SCALES[s]);

        // Up-and-down walk: 0..len-1, then back down len-2..1.
        // Two passes (up + down) make a continuous looped contour.
        for (int pass = 0; pass < 2; ++pass)
        {
            int stepCount = g_scaleLength + (g_scaleLength - 2); // up + down
            for (int i = 0; i < stepCount; ++i)
            {
                int idx = (i < g_scaleLength) ? i : (2 * g_scaleLength - 3 - i);
                if (idx < 0 || idx >= g_scaleLength) idx = 0;

                // Core-safe handoff: write the MIDI note for Core 0 to render.
                g_currentMidiNote = g_scaleNotes[idx];

                // Blocking delay here is fine: Core 0 owns the real-time audio
                // path and never touches Core 1.
                delay(125); // ~2 notes/second (16th notes at 60 BPM-ish)
            }
        }
    }
}
