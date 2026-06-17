// SimpleOscillators.ino
// ---------------------------------------------------------------------------
// Ladder Filter Demo – Pico-DSP-Garden
//
// Three polyBLEP sawtooth oscillators, slightly detuned, are summed and fed
// into a Huovilainen Moog ladder filter (LP24).  A slow triangle LFO sweeps
// the filter cutoff from ~100 Hz to ~3 kHz while resonance is held high
// (~0.85) for that classic "liquid" self-resonant sweep sound.
//
// Dual-core layout (see AGENTS.md):
//   Core 0  setup() / loop()   – real-time audio fill; must never block
//   Core 1  setup1()/ loop1()  – serial debug, parameter logging
// ---------------------------------------------------------------------------

#include "src/audio/audio.h"
#include "src/audio/audio_i2s.h"
#include "src/dsp/oscillator.h"
#include "src/dsp/ladder.h"

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

// Three slightly detuned polyBLEP saws for a rich, beating unison sound.
// Base note: A2 = 110 Hz.  Detune offsets in cents: 0, +7, -9
static const int   NUM_SAWS  = 3;
static const float BASE_FREQ = 110.0f; // A2

static daisysp::Oscillator saws[NUM_SAWS];
static daisysp::LadderFilter filter;

// LFO for cutoff sweep – use the Oscillator triangle wave at Core 0 sample rate
static daisysp::Oscillator cutoff_lfo;

// Shared volatile state for Core 1 monitoring (single-writer: Core 0)
volatile float g_cutoff_hz   = 0.0f;
volatile float g_resonance   = 0.0f;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Convert cents offset to a frequency multiplier
static inline float cents_to_ratio(float cents)
{
    // 2^(cents/1200)  – approximated with fast exp2 via: e^(cents*ln2/1200)
    // Using the standard powf; compile with -Os so it's inlined appropriately.
    return powf(2.0f, cents / 1200.0f);
}

static inline int16_t float_to_int16(float s)
{
    float scaled = s * INT16_MAX_F;
    // fclamp is from dsp.h / DaisySP
    scaled = daisysp::fclamp(scaled, INT16_MIN_F, INT16_MAX_F);
    return static_cast<int16_t>(scaled);
}

// ---------------------------------------------------------------------------
// Initialise DSP objects (called once from Core 0 setup)
// ---------------------------------------------------------------------------
static void initDSP()
{
    // Detune offsets in cents for each voice: root, slightly sharp, slightly flat
    const float detune_cents[NUM_SAWS] = { 0.0f, +7.0f, -9.0f };

    for (int i = 0; i < NUM_SAWS; ++i)
    {
        saws[i].Init(SAMPLE_RATE);
        saws[i].SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SAW);
        saws[i].SetFreq(BASE_FREQ * cents_to_ratio(detune_cents[i]));
        saws[i].SetAmp(1.0f);
    }

    // Ladder filter – LP24, high resonance, input drive gives warmth
    filter.Init(SAMPLE_RATE);
    filter.SetFilterMode(daisysp::LadderFilter::FilterMode::LP24);
    filter.SetRes(0.85f);          // 0.0 – 1.8; self-oscillates above ~1.6
    filter.SetInputDrive(1.5f);    // slight drive into the tanh stage
    filter.SetPassbandGain(0.3f);  // compensate for loudness drop at resonance
    filter.SetFreq(200.0f);        // start low; LFO will sweep this

    // Cutoff LFO – very slow triangle, period ≈ 8 s
    cutoff_lfo.Init(SAMPLE_RATE);
    cutoff_lfo.SetWaveform(daisysp::Oscillator::WAVE_TRI);
    cutoff_lfo.SetFreq(0.125f);    // 0.125 Hz → 8-second sweep
    cutoff_lfo.SetAmp(1.0f);

    // Publish initial values for Core 1
    g_cutoff_hz  = 200.0f;
    g_resonance  = 0.85f;
}

// ---------------------------------------------------------------------------
// Audio fill callback – Core 0 hot path; must not block
// ---------------------------------------------------------------------------
static void fill_audio_buffer(audio_buffer_t *buffer)
{
    const int    N   = static_cast<int>(buffer->max_sample_count);
    int16_t     *out = reinterpret_cast<int16_t *>(buffer->buffer->bytes);

    // Cutoff sweep range (Hz)
    static const float CUT_LO = 80.0f;
    static const float CUT_HI = 3200.0f;

    for (int i = 0; i < N; ++i)
    {
        // --- Cutoff LFO: triangle [-1, +1] → [CUT_LO, CUT_HI] ---
        float lfo_val = cutoff_lfo.Process(); // range [-1, 1]
        float t       = (lfo_val + 1.0f) * 0.5f; // remap to [0, 1]
        float cutoff  = CUT_LO + t * (CUT_HI - CUT_LO);
        filter.SetFreq(cutoff);

        // Update shared state (Core 1 reads this; single-writer is safe)
        g_cutoff_hz = cutoff;

        // --- Sum three detuned polyBLEP saws ---
        float osc_mix = 0.0f;
        for (int j = 0; j < NUM_SAWS; ++j)
        {
            osc_mix += saws[j].Process();
        }
        // Normalise to roughly -1..+1 range before filter
        osc_mix *= (1.0f / NUM_SAWS);

        // --- Ladder filter ---
        float filtered = filter.Process(osc_mix);

        // --- Output stereo (L = R) ---
        int16_t s = float_to_int16(filtered * 0.8f); // slight headroom
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
// Core 0 – setup & loop (real-time audio)
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
        .sample_stride = 4           // 2 channels × 2 bytes/sample
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
    // Real-time audio fill – Core 0 must never block here
    audio_buffer_t *buf = take_audio_buffer(producer_pool, true);
    if (buf)
    {
        fill_audio_buffer(buf);
        give_audio_buffer(producer_pool, buf);
    }
}

// ---------------------------------------------------------------------------
// Core 1 – setup1 & loop1 (non-real-time: serial debug, etc.)
// ---------------------------------------------------------------------------
void setup1()
{
    delay(200);
    Serial.begin(115200);
    Serial.println("[CORE1] Ladder Filter Demo starting...");
    Serial.print  ("[CORE1] Sample rate: ");
    Serial.println(SAMPLE_RATE);
    Serial.print  ("[CORE1] Base freq:   ");
    Serial.print  (BASE_FREQ);
    Serial.println(" Hz (A2)");
    Serial.println("[CORE1] Resonance:   0.85");
    Serial.println("[CORE1] Cutoff LFO:  0.125 Hz (8-second sweep, 80–3200 Hz)");
}

void loop1()
{
    // Print cutoff position approximately every 500 ms for monitoring
    static uint32_t last_print_ms = 0;
    uint32_t now = millis();
    if (now - last_print_ms >= 500)
    {
        last_print_ms = now;
        Serial.print("[CORE1] cutoff = ");
        Serial.print(g_cutoff_hz, 1);
        Serial.println(" Hz");
    }
}
