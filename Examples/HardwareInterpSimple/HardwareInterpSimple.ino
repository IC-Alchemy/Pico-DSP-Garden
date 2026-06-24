// HardwareInterpSimple.ino
// ---------------------------------------------------------------------------
// Minimal hardware-interpolator example - Pico-DSP-Garden
//
// A single wavetable oscillator plays a fixed A4 (440 Hz) sine. The phase
// accumulator lives in the RP2xxx INTERP0 peripheral, and every sample is
// linearly interpolated between two table entries by the hardware blend
// datapath — no CPU multiply in the audio loop.
//
// Core split follows the existing example convention (NOT Docs/realtime_rules):
//   Core 0 (setup/loop)   : real-time audio fill callback + DSP graph.
//   Core 1 (setup1/loop1) : monitoring and Serial output.
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
//     --library libraries/rpdsp --library libraries/pico_audio_i2s Examples/HardwareInterpSimple
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

// 16-entry sine wavetable; hardware interpolation band-limits the steps.
static constexpr std::uint32_t kTableSize = 16;
static const std::int16_t kSineTable[kTableSize] = {
    0, 12539, 23170, 30273,
    32767, 30273, 23170, 12539,
    0, -12539, -23170, -30273,
    -32768, -30273, -23170, -12539
};

// ---------------------------------------------------------------------------
// DSP objects (owned by Core 0; used only inside the fill callback)
// ---------------------------------------------------------------------------
static rpdsp::HardwareOscillator osc;
static audio_buffer_pool_t *producer_pool = nullptr;

// ---------------------------------------------------------------------------
// Initialise DSP objects (called once from Core 0 setup)
// ---------------------------------------------------------------------------
static void initDSP()
{
    // Q12.20 phase: 12 integer bits index the 16-entry table (4 used),
    // 20 fractional bits feed the interpolator's 8-bit blend weight.
    constexpr std::uint8_t kFractionalBits = 20;

    std::uint32_t tuningWord = 0;
    rpdsp::HardwareOscillator::tuningWordFromFrequency(
        kTableSize, kFractionalBits, static_cast<std::uint32_t>(SAMPLE_RATE),
        440, tuningWord);

    // INTERP0 is required: only interp0 supports the blend mode used for
    // linear interpolation. init() claims the lane and configures both lanes.
    osc.init(interp0, kSineTable, kTableSize, kFractionalBits, /*phase=*/0, tuningWord);
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
        // Advance the hardware phase accumulator and read the interpolated
        // 16-bit sample for this step.
        std::int32_t sample = 0;
        osc.nextSample(sample);

        // Convert the int16 wavetable sample to floating-point ±1.0 and emit it
        // to both channels at a conservative level.
        const float outf = static_cast<float>(sample) / 32768.0f * 0.5f;
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
    Serial.println("[CORE1] HardwareInterpSimple demo starting...");
    Serial.print  ("[CORE1] Sample rate: ");
    Serial.println(SAMPLE_RATE);
}

void loop1()
{
    static uint32_t last_print_ms = 0;
    uint32_t now = millis();
    if (now - last_print_ms >= 2000)
    {
        last_print_ms = now;
        Serial.println("[CORE1] Simple oscillator running (440 Hz Sine)...");
    }
}
