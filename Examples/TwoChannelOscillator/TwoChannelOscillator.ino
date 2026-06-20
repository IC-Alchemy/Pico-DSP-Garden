// Two Channel Oscillator
// Left and right channels each run an independent sine oscillator.
// Four analog sliders control frequency:
//   A0 — Left coarse  (20 Hz – 4000 Hz, log scale)
//   A1 — Left fine    (±50 Hz offset, linear, centered at mid-travel)
//   A2 — Right coarse (20 Hz – 4000 Hz, log scale)
//   A3 — Right fine   (±50 Hz offset, linear, centered at mid-travel)
//
// Hardware: Raspberry Pi Pico / Pico2 + PCM510x I2S DAC
//   DATA  → GPIO 15
//   LRCK  → GPIO 16
//   BCLK  → GPIO 17

// Library discovery triggers — arduino-cli detects libraries via root-level src/ headers.
#include <rpdsp.h>
#include <pico_audio_i2s.h>

#include <pico_audio_i2s/audio.h>
#include <pico_audio_i2s/audio_i2s.h>
#include <rpdsp/oscillator.h>

// ─── I2S pin config ───────────────────────────────────────────────────────────
int PICO_AUDIO_I2S_DATA_PIN       = 15;
int PICO_AUDIO_I2S_CLOCK_PIN_BASE = 16; // LRCK=16, BCLK=17

// ─── Audio engine constants ───────────────────────────────────────────────────
const float SAMPLE_RATE      = 48000.0f;
const float INT16_MAX_F      = 32767.0f;
const float INT16_MIN_F      = -32768.0f;
const int   NUM_AUDIO_BUFFERS  = 3;
const int   SAMPLES_PER_BUFFER = 32;

audio_buffer_pool_t *producer_pool = nullptr;

// ─── Oscillators ──────────────────────────────────────────────────────────────
rpdsp::SineOscillator osc_left;
rpdsp::SineOscillator osc_right;

// ─── Shared frequency state (Core 1 writes, Core 0 reads) ────────────────────
volatile float left_freq  = 440.0f;
volatile float right_freq = 440.0f;

// ─── Slider ADC pins ──────────────────────────────────────────────────────────
const int PIN_LEFT_COARSE  = A0; // GPIO 26
const int PIN_LEFT_FINE    = A1; // GPIO 27
const int PIN_RIGHT_COARSE = A2; // GPIO 28
const int PIN_RIGHT_FINE   = A3; // GPIO 29

// ─── Frequency mapping ────────────────────────────────────────────────────────
const float COARSE_MIN_HZ = 20.0f;
const float COARSE_MAX_HZ = 4000.0f;
const float FINE_RANGE_HZ = 50.0f; // slider center = 0 Hz offset

// Logarithmic mapping: gives musically even spacing across octaves.
static inline float mapCoarse(float adc12) {
    float t = adc12 / 4095.0f;
    return COARSE_MIN_HZ * powf(COARSE_MAX_HZ / COARSE_MIN_HZ, t);
}

// Linear mapping centered at slider midpoint: output is -FINE_RANGE to +FINE_RANGE.
static inline float mapFine(float adc12) {
    return ((adc12 / 4095.0f) - 0.5f) * (FINE_RANGE_HZ * 2.0f);
}

// ─── Audio helpers ────────────────────────────────────────────────────────────
static inline int16_t toInt16(float s) {
    float v = roundf(s * INT16_MAX_F);
    v = rpdsp::clamp(v, INT16_MIN_F, INT16_MAX_F);
    return static_cast<int16_t>(v);
}

// ─── Audio callback (Core 0) ──────────────────────────────────────────────────
void fill_audio_buffer(audio_buffer_t *buffer) {
    int      N   = buffer->max_sample_count;
    int16_t *out = reinterpret_cast<int16_t *>(buffer->buffer->bytes);

    // Snapshot frequencies once per buffer to avoid tearing mid-fill.
    osc_left.setFreq(left_freq);
    osc_right.setFreq(right_freq);

    for (int i = 0; i < N; ++i) {
        out[2 * i + 0] = toInt16(osc_left.process() * 0.8f);   // left
        out[2 * i + 1] = toInt16(osc_right.process() * 0.8f);  // right
    }
    buffer->sample_count = N;
}

// ─── I2S setup helper ─────────────────────────────────────────────────────────
void setupI2SAudio(audio_format_t *fmt, audio_i2s_config_t *cfg) {
    if (!audio_i2s_setup(fmt, cfg)) {
        Serial.println("I2S setup failed");
        delay(1000);
        return;
    }
    if (!audio_i2s_connect(producer_pool)) {
        Serial.println("I2S connect failed");
        delay(1000);
        return;
    }
    audio_i2s_set_enabled(true);
    Serial.println("Audio ready");
}

// ─── Core 0 setup ─────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(150);

    osc_left.prepare(SAMPLE_RATE);
    osc_left.setFreq(left_freq);

    osc_right.prepare(SAMPLE_RATE);
    osc_right.setFreq(right_freq);

    static audio_format_t fmt = {
        .sample_freq  = (uint32_t)SAMPLE_RATE,
        .format       = AUDIO_BUFFER_FORMAT_PCM_S16,
        .channel_count = 2
    };
    static audio_buffer_format_t bfmt = {
        .format        = &fmt,
        .sample_stride = 4
    };
    producer_pool = audio_new_producer_pool(&bfmt, NUM_AUDIO_BUFFERS, SAMPLES_PER_BUFFER);

    audio_i2s_config_t i2sCfg = {
        .data_pin       = PICO_AUDIO_I2S_DATA_PIN,
        .clock_pin_base = PICO_AUDIO_I2S_CLOCK_PIN_BASE,
        .dma_channel    = 0,
        .pio_sm         = 0
    };
    setupI2SAudio(&fmt, &i2sCfg);
}

// ─── Core 1 setup ─────────────────────────────────────────────────────────────
void setup1() {
    delay(200);
    analogReadResolution(12); // 12-bit ADC → 0–4095
    Serial.println("[CORE1] Slider reader ready");
}

// ─── Core 0 audio loop ────────────────────────────────────────────────────────
void loop() {
    audio_buffer_t *buf = take_audio_buffer(producer_pool, true);
    if (buf) {
        fill_audio_buffer(buf);
        give_audio_buffer(producer_pool, buf);
    }
}

// ─── Core 1 slider reader ─────────────────────────────────────────────────────
void loop1() {
    // Smoothed ADC state — exponential moving average (α = 0.1) reduces noise.
    static float sl_coarse = 2048.0f, sl_fine = 2048.0f;
    static float sr_coarse = 2048.0f, sr_fine = 2048.0f;

    sl_coarse = sl_coarse * 0.9f + analogRead(PIN_LEFT_COARSE)  * 0.1f;
    sl_fine   = sl_fine   * 0.9f + analogRead(PIN_LEFT_FINE)    * 0.1f;
    sr_coarse = sr_coarse * 0.9f + analogRead(PIN_RIGHT_COARSE) * 0.1f;
    sr_fine   = sr_fine   * 0.9f + analogRead(PIN_RIGHT_FINE)   * 0.1f;

    float lf = mapCoarse(sl_coarse) + mapFine(sl_fine);
    float rf = mapCoarse(sr_coarse) + mapFine(sr_fine);

    // Clamp to audible range before writing shared state.
    left_freq  = rpdsp::clamp(lf, 20.0f, 20000.0f);
    right_freq = rpdsp::clamp(rf, 20.0f, 20000.0f);

    delay(10); // poll at ~100 Hz, well below audio rate
}
