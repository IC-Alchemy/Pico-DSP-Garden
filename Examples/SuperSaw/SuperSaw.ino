// SuperSaw.ino
// ---------------------------------------------------------------------------
// Hypersaw ("Super Saw") demo - Pico-DSP-Garden
//
// Seven detuned band-limited sawtooth oscillators after the Roland JP-8000,
// arpeggiating a 32-step sequence. Built entirely on the rpdsp library.
//

// Library discovery triggers — arduino-cli detects libraries via root-level src/ headers.
// Once loaded, src/ is on the include path so the namespaced headers below resolve.
#include <rpdsp.h>
#include <pico_audio_i2s.h>

#include <pico_audio_i2s/audio.h>
#include <pico_audio_i2s/audio_i2s.h>
#include <rpdsp/oscillator.h>
#include <rpdsp/hypersaw.h>

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

// 32-step arpeggio, semitone offsets added to MIDI note 48 (C3).
static const int SCALE_LENGTH = 32;
static const int SCALE[SCALE_LENGTH] = {
    0, 3, 5, 10, 12, 6, 7, 12,
    7, 6, 7, 8, 10, 6, 4, 2,
    0, 2, 4, 6, 8, 4, 2, 0,
    7, 9, 10, 12, 6, 8, 7, 5
};

volatile int note_index = 0; // core-safe: Core 1 writes, Core 0 reads

// ---------------------------------------------------------------------------
// Initialise DSP objects (called once from Core 0 setup)
// ---------------------------------------------------------------------------
static void initHypersaw()
{
    hypersaw.prepare(SAMPLE_RATE);
    hypersaw.setFreq(rpdsp::midiNoteToHz(static_cast<float>(SCALE[0] + 48)));
    hypersaw.setDetune(0.381f);
    hypersaw.setMix(0.5f);
}

// ---------------------------------------------------------------------------
// Audio fill callback - Core 0 hot path; must not block
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
// Core 0 - setup & loop (real-time audio)
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
    // Real-time audio fill - Core 0 must never block here
    audio_buffer_t *buf = take_audio_buffer(producer_pool, true);
    if (buf)
    {
        fill_audio_buffer(buf);
        give_audio_buffer(producer_pool, buf);
    }
}

// ---------------------------------------------------------------------------
// Core 1 - setup1 & loop1 (non-real-time: sequencing, serial debug)
// ---------------------------------------------------------------------------
void setup1()
{
    delay(200);
    Serial.begin(115200);
    Serial.println("[CORE1] SuperSaw hypersaw demo starting...");
    Serial.print  ("[CORE1] Sample rate: ");
    Serial.println(SAMPLE_RATE);
    Serial.println("[CORE1] Sequence:    32-step arpeggio");
}

void loop1()
{
    hypersaw.setFreq(rpdsp::midiNoteToHz(static_cast<float>(SCALE[note_index] + 48)));

    // Blocking delay here is fine: Core 0 owns the real-time audio path and
    // never touches Core 1. In a real sequencer you'd schedule on a timer.
    delay(122);

    note_index = (note_index + 1) % SCALE_LENGTH;
}
