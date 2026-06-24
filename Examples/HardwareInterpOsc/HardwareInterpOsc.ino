// HardwareInterpOsc.ino
// ---------------------------------------------------------------------------
// Dual-oscillator wavetable patch with hardware interpolation.
//
// Two oscillators (sine + triangle) tuned a minor third apart are independently
// pitch-modulated by two slow LFOs at different rates. A shared unipolar LFO
// drives an auto-pan crossfade between them, producing a slowly evolving,
// dissonant drone that breathes and wanders across the stereo field.
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
//     --library libraries/rpdsp --library libraries/pico_audio_i2s Examples/HardwareInterpOsc
// ---------------------------------------------------------------------------

#include <rpdsp.h>
#include <pico_audio_i2s.h>

#include <pico_audio_i2s/audio.h>
#include <pico_audio_i2s/audio_i2s.h>
#include <rpdsp/oscillator.h>      
#include <rpdsp/hardware_interpolator.h>

// ---------------------------------------------------------------------------
// Pin + engine constants
// ---------------------------------------------------------------------------
static const int   PICO_AUDIO_I2S_DATA_PIN       = 15;
static const int   PICO_AUDIO_I2S_CLOCK_PIN_BASE = 16;  // LRCK = 16, BCLK = 17

static const float SAMPLE_RATE        = 48000.0f;  // == rpdsp::kDefaultSampleRate
static const int   NUM_AUDIO_BUFFERS  = 3;
static const int   SAMPLES_PER_BUFFER = 32;

// 16-sample wavetables with hardware linear interpolation for band-limiting
static constexpr std::uint32_t kTableSize = 16;
static const std::int16_t kSineTable[kTableSize] = {
    0, 12539, 23170, 30273,
    32767, 30273, 23170, 12539,
    0, -12539, -23170, -30273,
    -32768, -30273, -23170, -12539
};

static const std::int16_t kTriangleTable[kTableSize] = {
    0, 8192, 16384, 24576,
    32767, 24576, 16384, 8192,
    0, -8192, -16384, -24576,
    -32768, -24576, -16384, -8192
};

// ---------------------------------------------------------------------------
// Cross-core state (Core 0 writes, Core 1 reads; plain volatile)
// ---------------------------------------------------------------------------
volatile float g_freqHz1 = 330.0f;
volatile float g_freqHz2 = 220.0f;

// ---------------------------------------------------------------------------
// DSP objects (owned by Core 0; used only inside the fill callback)
// ---------------------------------------------------------------------------
static rpdsp::HardwareOscillator osc;    // Sine — crisp, pure, the "lead" voice
static rpdsp::HardwareOscillator osc2;   // Triangle — warmer, harmonic-rich foil
static rpdsp::SineOscillator lfo;        // Bipolar LFO, ~6 s cycle
static rpdsp::SineOscillator lfo2;       // Unipolar LFO, ~19 s cycle

static audio_buffer_pool_t *producer_pool = nullptr;

// ---------------------------------------------------------------------------
// Initialise DSP objects (called once from Core 0 setup)
// ---------------------------------------------------------------------------
static void initDSP()
{
    // Two slow LFOs for independent pitch drift — emulates organic detuning
    lfo.prepare(SAMPLE_RATE);
    lfo.setFreq(0.17f);   // ~6-second cycle, gentle wandering
    lfo2.prepare(SAMPLE_RATE);
    lfo2.setFreq(0.053f);  // ~19-second cycle, slow evolving backdrop

    // Osc 1: sine wave tuned to A4 (440 Hz)
    std::uint32_t tuningWord = 0;
    rpdsp::HardwareOscillator::tuningWordFromFrequency(
        kTableSize, 20, static_cast<std::uint32_t>(SAMPLE_RATE), 440, tuningWord);

    osc.init(interp0, kSineTable, kTableSize, 20, 0, tuningWord);

    // Osc 2: triangle wave tuned to C5 (523 Hz), a minor third above A4
    // Together they create a tense, dissonant interval that the LFOs bring to life
    std::uint32_t tuningWord2 = 0;
    rpdsp::HardwareOscillator::tuningWordFromFrequency(
        kTableSize, 20, static_cast<std::uint32_t>(SAMPLE_RATE), 523, tuningWord2);

    osc2.init(interp1, kTriangleTable, kTableSize, 20, 0, tuningWord2);
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
        // LFO1: bipolar, sweeps pitch above and below center
        const float lfoVal = lfo.process();
        // LFO2: unipolar (0…1), used for both slow pitch drift and stereo panning
        const float lfo2Val = lfo2.process() * 0.5f + 0.5f;

        // Osc 1 base ~330 Hz with ±220 Hz wiggle from LFO1 + subtle 55 Hz drift from LFO2
        const float freqHz1 = 330.0f + lfoVal * 220.0f + lfo2Val * 55.0f;
        // Osc 2 base ~220 Hz with stronger LFO2 sway (±440 Hz) and tighter LFO1 coupling
        const float freqHz2 = 220.0f + lfo2Val * 440.0f + lfoVal * 110.0f;

        g_freqHz1 = freqHz1;
        g_freqHz2 = freqHz2;

        // Recompute tuning words every sample for continuous LFO-driven pitch modulation
        std::uint32_t tuningWord = 0;
        rpdsp::HardwareOscillator::tuningWordFromFrequency(
            kTableSize, 20, static_cast<std::uint32_t>(SAMPLE_RATE), static_cast<std::uint32_t>(freqHz1), tuningWord);
        osc.setPhaseIncrement(tuningWord);

        std::uint32_t tuningWord2 = 0;
        rpdsp::HardwareOscillator::tuningWordFromFrequency(
            kTableSize, 20, static_cast<std::uint32_t>(SAMPLE_RATE), static_cast<std::uint32_t>(freqHz2), tuningWord2);
        osc2.setPhaseIncrement(tuningWord2);

        // Let the hardware interpolator advance and produce the next sample
        std::int32_t sample1 = 0;
        std::int32_t sample2 = 0;
        osc.nextSample(sample1);
        osc2.nextSample(sample2);

        // Convert 16-bit integer wavetable samples to floating-point ±1.0
        float out1 = static_cast<float>(sample1) / 32768.0f;
        float out2 = static_cast<float>(sample2) / 32768.0f;

        // Stereo crossfade pan: LFO2 sweeps pan from 0.3 to 0.8 (slightly off-center)
        // When pan=0, out1 is hard-left and out2 hard-right; when pan=1, positions swap
        const float pan = lfo2Val * 0.5f + 0.3f;
        const float left = out1 * (1.0f - pan) + out2 * pan;
        const float right = out1 * pan + out2 * (1.0f - pan);

        // Master level: -8.4 dB to prevent clipping when both oscillators sum
        const float finalLeft  = left * 0.38f;
        const float finalRight = right * 0.38f;

        dst[2 * i + 0] = rpdsp::toInt24x32(finalLeft);   // left
        dst[2 * i + 1] = rpdsp::toInt24x32(finalRight);  // right
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
// Core 1 - setup1 & loop1 (sequencing, parameter monitoring, and Serial)
// ---------------------------------------------------------------------------
void setup1()
{
    delay(200);
    Serial.begin(115200);
    Serial.println("[CORE1] HardwareInterpOsc demo starting...");
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
        Serial.print("[CORE1] Osc1: ");
        Serial.print(g_freqHz1, 1);
        Serial.print(" Hz, Osc2: ");
        Serial.print(g_freqHz2, 1);
        Serial.println(" Hz");
    }
}
