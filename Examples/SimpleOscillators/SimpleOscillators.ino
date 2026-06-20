// SimpleOscillators.ino
// ---------------------------------------------------------------------------
// Simple Oscillators Demo - Pico-DSP-Garden
//
// A band-limited second-order B-spline hard-sync saw oscillator plays long
// C minor pentatonic notes from roughly 110-440 Hz. The slave frequency is the
// note pitch; the master oscillator is slowly modulated to sweep the hard-sync
// timbre while the pitch stays musical.
//

// Library discovery triggers — arduino-cli detects libraries via root-level src/ headers.
#include <rpdsp.h>
#include <pico_audio_i2s.h>

#include <pico_audio_i2s/audio.h>
#include <pico_audio_i2s/audio_i2s.h>
#include <rpdsp/oscillator.h>

// ---------------------------------------------------------------------------
// I2S pin assignment (see AGENTS.md wiring convention)
// ---------------------------------------------------------------------------
static const int PICO_AUDIO_I2S_DATA_PIN       = 15;
static const int PICO_AUDIO_I2S_CLOCK_PIN_BASE = 16; // LRCK = 16, BCLK = 17

// ---------------------------------------------------------------------------
// Audio engine constants
// ---------------------------------------------------------------------------
static const float    SAMPLE_RATE        = 44100.0f;
static const float    INT16_MAX_F        = 32767.0f;
static const float    INT16_MIN_F        = -32768.0f;
static const int      NUM_AUDIO_BUFFERS  = 3;
static const int      SAMPLES_PER_BUFFER = 256;

static audio_buffer_pool_t *producer_pool = nullptr;

// ---------------------------------------------------------------------------
// DSP objects
// ---------------------------------------------------------------------------
static rpdsp::SecondOrderBSplineHardSyncSawOscillator hard_sync_osc;
static rpdsp::SineOscillator master_lfo;

static const float ROOT_FREQ = 116.5409f; // Bb2, part of C minor pentatonic

static const int SCALE_LENGTH = 10;
static const float C_MINOR_PENTATONIC[SCALE_LENGTH] = {
    116.5409f, // Bb2
    130.8128f, // C3
    155.5635f, // Eb3
    174.6141f, // F3
    195.9977f, // G3
    233.0819f, // Bb3
    261.6256f, // C4
    311.1270f, // Eb4
    349.2282f, // F4
    391.9954f  // G4
};

volatile int g_note_index = 0;
volatile float g_slave_hz = ROOT_FREQ;

// Shared volatile state for Core 1 monitoring (single-writer: Core 0)
volatile float g_master_hz = ROOT_FREQ * 0.5f;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static inline int16_t float_to_int16(float s)
{
    float scaled = s * INT16_MAX_F;
    scaled = rpdsp::clamp(scaled, INT16_MIN_F, INT16_MAX_F);
    return static_cast<int16_t>(scaled);
}

// ---------------------------------------------------------------------------
// Initialise DSP objects (called once from Core 0 setup)
// ---------------------------------------------------------------------------
static void initDSP()
{
    hard_sync_osc.prepare(SAMPLE_RATE);
    hard_sync_osc.reset();
    hard_sync_osc.setSlaveFrequency(g_slave_hz);
    hard_sync_osc.setMasterFrequency(g_master_hz);

    master_lfo.prepare(SAMPLE_RATE);
    master_lfo.reset();
    master_lfo.setFreq(0.05f); // one timbre sweep about every 28 seconds
}

// ---------------------------------------------------------------------------
// Audio fill callback - Core 0 hot path; must not block
// ---------------------------------------------------------------------------
static void fill_audio_buffer(audio_buffer_t *buffer)
{
    const int    N   = static_cast<int>(buffer->max_sample_count);
    int16_t     *out = reinterpret_cast<int16_t *>(buffer->buffer->bytes);

    for (int i = 0; i < N; ++i)
    {
        float slave_hz = g_slave_hz;
        float lfo = (master_lfo.process() + 1.0f) * 0.5f;
        float master_hz = slave_hz * (0.35f + (1.35f * lfo));

        hard_sync_osc.setSlaveFrequency(slave_hz);
        hard_sync_osc.setMasterFrequency(master_hz);
        g_master_hz = master_hz;

        float signal = rpdsp::softClip(hard_sync_osc.process() * 0.15f);

        int16_t s = float_to_int16(signal * 0.12f);
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

    initDSP();

    static audio_format_t audioFmt = {
        .sample_freq   = static_cast<uint32_t>(SAMPLE_RATE),
        .format        = AUDIO_BUFFER_FORMAT_PCM_S16,
        .channel_count = 2
    };
    static audio_buffer_format_t bufFmt = {
        .format        = &audioFmt,
        .sample_stride = 4           // 2 channels x 2 bytes/sample
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
// Core 1 - setup1 & loop1 (non-real-time: serial debug, etc.)
// ---------------------------------------------------------------------------
void setup1()
{
    delay(200);
    Serial.begin(115200);
    Serial.println("[CORE1] Simple Oscillators hard-sync demo starting...");
    Serial.print  ("[CORE1] Sample rate: ");
    Serial.println(SAMPLE_RATE);
    Serial.println("[CORE1] Scale:       C minor pentatonic, long notes");
    Serial.println("[CORE1] Slave range: 110-440 Hz");
    Serial.println("[CORE1] Master LFO:  0.035 Hz");
}

void loop1()
{
    static const uint32_t NOTE_MS = 15000;
    static uint32_t last_note_ms = 0;

    uint32_t now = millis();
    if (now - last_note_ms >= NOTE_MS)
    {
        last_note_ms = now;
        g_note_index = (g_note_index + 1) % SCALE_LENGTH;

        float note_hz = C_MINOR_PENTATONIC[g_note_index];
        g_slave_hz = note_hz;
    }

    // Print the current pitch and sync sweep approximately every 500 ms.
    static uint32_t last_print_ms = 0;
    if (now - last_print_ms >= 500)
    {
        last_print_ms = now;
        Serial.print("[CORE1] slave = ");
        Serial.print(g_slave_hz, 1);
        Serial.print(" Hz, master = ");
        Serial.print(g_master_hz, 1);
        Serial.println(" Hz");
    }
}
