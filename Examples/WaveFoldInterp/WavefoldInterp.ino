// WaveFoldInterp.ino
// ---------------------------------------------------------------------------
// Wavefolder hardware-interpolator example - Pico-DSP-Garden
//
// A carrier sine wave is frequency-modulated by a modulator sine wave.
// The result is passed to a hardware-accelerated wavefolder (using Core 0 INTERP1)
// with LFO-modulated stage count and gain, then filtered through a biquad lowpass.
//
// Core split follows the existing example convention (NOT Docs/realtime_rules):
//   Core 0 (setup/loop)   : real-time audio fill callback + DSP graph.
//   Core 1 (setup1/loop1) : parameter monitoring and Serial output.
//
// Wiring
//   I2S DAC (PCM510x-class):
//     DATA  -> GPIO 15   (PICO_AUDIO_I2S_DATA_PIN)
//     LRCK  -> GPIO 16   (PICO_AUDIO_I2S_CLOCK_PIN_BASE)
//     BCLK  -> GPIO 17   (clock_pin_base + 1, assigned by the driver)
//
// Build:
//   arduino-cli compile \
//     --fqbn rp2040:rp2040:rpipico2:flash=4194304_0,freq=200,opt=Optimize3,usbstack=picosdk \
//     --library libraries/rpdsp --library libraries/pico_audio_i2s Examples/WaveFoldInterp
// ---------------------------------------------------------------------------

#include <rpdsp.h>
#include <pico_audio_i2s.h>

#include <pico_audio_i2s/audio.h>
#include <pico_audio_i2s/audio_i2s.h>
#include <rpdsp/oscillator.h>      
#include <rpdsp/hardware_interpolator.h>
#include <rpdsp/filter.h>

// ---------------------------------------------------------------------------
// Pin + engine constants
// ---------------------------------------------------------------------------
static const int   PICO_AUDIO_I2S_DATA_PIN       = 15;
static const int   PICO_AUDIO_I2S_CLOCK_PIN_BASE = 16;  // LRCK = 16, BCLK = 17

static const float SAMPLE_RATE        = 48000.0f;  // == rpdsp::kDefaultSampleRate
static const int   NUM_AUDIO_BUFFERS  = 3;
static const int   SAMPLES_PER_BUFFER = 32;

// ---------------------------------------------------------------------------
// Cross-core state (Core 0 writes, Core 1 reads; plain volatile)
// ---------------------------------------------------------------------------
volatile float g_lfoVal = 0.0f;
volatile float g_carrierFreq = 220.0f;

// ---------------------------------------------------------------------------
// DSP objects (owned by Core 0; used only inside the fill callback)
// ---------------------------------------------------------------------------
static rpdsp::SineOscillator carrier;
static rpdsp::SineOscillator modulator;
static rpdsp::SineOscillator fmRatio;
static rpdsp::SineOscillator lfo;
static rpdsp::BiquadLowpass tone;
static rpdsp::HardwareWavefolder wavefolder;

static audio_buffer_pool_t *producer_pool = nullptr;

// ---------------------------------------------------------------------------
// Initialise DSP objects (called once from Core 0 setup)
// ---------------------------------------------------------------------------
static void initDSP()
{
    carrier.prepare(SAMPLE_RATE);
    carrier.setFreq(220.0f);
    modulator.prepare(SAMPLE_RATE);
    modulator.setFreq(110.0f);
    fmRatio.prepare(SAMPLE_RATE);
    fmRatio.setFreq(0.055f);

    lfo.prepare(SAMPLE_RATE);
    lfo.setFreq(0.35f);

    tone.prepare(SAMPLE_RATE);
    tone.setCutoff(4800.0f);

    wavefolder.init(rpdsp::HardwareInterpolatorPool::Resource::Core0Interp1, 13);
    wavefolder.setStages(4);
    wavefolder.setGain(1.0f);
}

// ---------------------------------------------------------------------------
// Audio fill callback - Core 0 hard real-time path
// ---------------------------------------------------------------------------
static void fill_audio_buffer(audio_buffer_t *buffer)
{
    const int    N   = static_cast<int>(buffer->max_sample_count);
    int32_t     *dst = reinterpret_cast<int32_t *>(buffer->buffer->bytes);

    for (int i = 0; i < N; ++i)
    {
        const float lfoVal = lfo.process() * 0.5f + 0.5f;
        const float fmMod = fmRatio.process() * 0.5f + 0.5f;

        const float freqHz = 110.0f + fmMod * 440.0f;
        carrier.setFreq(freqHz);
        modulator.setFreq(freqHz * rpdsp::lerp(0.5f, 3.0f, lfoVal));

        g_lfoVal = lfoVal;
        g_carrierFreq = freqHz;

        const float fmDepth = 0.3f + lfoVal * 0.4f;
        const float carrierVal = (carrier.process() + modulator.process() * fmDepth) * 0.5f;

        const float gain = 1.5f + lfoVal * 3.5f;
        wavefolder.setGain(gain);
        wavefolder.setStages(static_cast<int>(2 + lfoVal * 4));

        std::int32_t intInput = static_cast<std::int32_t>(carrierVal * 8192.0f);
        std::int32_t intOutput = wavefolder.process(intInput);

        float folded = static_cast<float>(intOutput) / 8192.0f;
        tone.setCutoff(800.0f + lfoVal * 6000.0f);
        float filtered = tone.process(folded);

        // Convert the float sample to floating-point ±1.0 and emit it to left/right
        const float outf = filtered * 0.45f;
        const int32_t output = rpdsp::toInt24x32(outf);

        dst[2 * i + 0] = output; // Left
        dst[2 * i + 1] = output; // Right
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
    delay(150);  // let power rails settle

    initDSP();

    static audio_format_t audioFmt = {
        .sample_freq   = static_cast<uint32_t>(SAMPLE_RATE),
        .format        = AUDIO_BUFFER_FORMAT_PCM_S32,
        .channel_count = 2
    };
    static audio_buffer_format_t bufFmt = {
        .format        = &audioFmt,
        .sample_stride = 8            // 2 channels x 4 bytes/sample (24-in-32)
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
    // Real-time audio fill - Core 0 must never block here.
    audio_buffer_t *buf = take_audio_buffer(producer_pool, true);
    if (buf)
    {
        fill_audio_buffer(buf);
        give_audio_buffer(producer_pool, buf);
    }
}

// ---------------------------------------------------------------------------
// Core 1 - setup1 & loop1 (parameter monitoring, Serial, etc.)
// ---------------------------------------------------------------------------
void setup1()
{
    delay(200);
    Serial.begin(115200);
    Serial.println("[CORE1] WaveFoldInterp demo starting...");
    Serial.print  ("[CORE1] Sample rate: ");
    Serial.println(SAMPLE_RATE);
}

void loop1()
{
    static uint32_t last_print_ms = 0;
    uint32_t now = millis();
    if (now - last_print_ms >= 1000)
    {
        last_print_ms = now;
        Serial.print("[CORE1] Carrier Freq: ");
        Serial.print(g_carrierFreq, 1);
        Serial.print(" Hz, LFO: ");
        Serial.println(g_lfoVal, 3);
    }
}
