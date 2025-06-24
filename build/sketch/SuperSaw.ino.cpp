#include <Arduino.h>
#line 1 "D:\\CodePCB\\Code\\Pico\\Pico-DSP-Garden\\Examples\\SuperSaw\\SuperSaw.ino"
#include "src/dsp/SuperSaw.h" // Include the Hypersaw class definition
#include "src/audio/audio.h"
#include "src/audio/audio_i2s.h"
#include "src/dsp/oscillator.h"
// Define the number of oscillators

//  pins for the I2S audio most should work fine, but I'm using PCM510x
int PICO_AUDIO_I2S_DATA_PIN = 15;
int PICO_AUDIO_I2S_CLOCK_PIN_BASE = 16; //  Pico forces you to use BasePin + 1 for the LRCK so LRCK is 17

float SAMPLE_RATE = 48000.0f;
float INT16_MAX_AS_FLOAT = 32767.0f;
float INT16_MIN_AS_FLOAT = -32768.0f;
int NUM_AUDIO_BUFFERS = 3;
int SAMPLES_PER_BUFFER = 256;
audio_buffer_pool_t *producer_pool = nullptr;


// Create an instance of our Hypersaw oscillator
daisysp::Hypersaw hypersaw;

// We'll use two LFOs to modulate the Hypersaw's parameters
daisysp::Oscillator lfo_detune;
daisysp::Oscillator lfo_mix;

// Test oscillator for debugging
daisysp::Oscillator test_osc;
bool use_test_tone = false; // Set to true to use simple test tone instead of hypersaw

// --- Scale for Arpeggiation ---
int scale[24] = {
    // Pentatonic Minor
    0, 3, 5, 7, 9, 10, 12, 15, 17, 19, 21, 22,
    24, 27, 29, 31, 32, 34, 36, 39, 41, 43, 46, 48
};
volatile int note_index = 0; // Use volatile for core-safe access

#line 38 "D:\\CodePCB\\Code\\Pico\\Pico-DSP-Garden\\Examples\\SuperSaw\\SuperSaw.ino"
void initHypersaw();
#line 71 "D:\\CodePCB\\Code\\Pico\\Pico-DSP-Garden\\Examples\\SuperSaw\\SuperSaw.ino"
static int16_t convertSampleToInt16(float sample);
#line 82 "D:\\CodePCB\\Code\\Pico\\Pico-DSP-Garden\\Examples\\SuperSaw\\SuperSaw.ino"
void fill_audio_buffer(audio_buffer_t *buffer);
#line 138 "D:\\CodePCB\\Code\\Pico\\Pico-DSP-Garden\\Examples\\SuperSaw\\SuperSaw.ino"
void setup1();
#line 145 "D:\\CodePCB\\Code\\Pico\\Pico-DSP-Garden\\Examples\\SuperSaw\\SuperSaw.ino"
void loop1();
#line 159 "D:\\CodePCB\\Code\\Pico\\Pico-DSP-Garden\\Examples\\SuperSaw\\SuperSaw.ino"
void setupI2SAudio(audio_format_t *audioFormat, audio_i2s_config_t *i2sConfig);
#line 183 "D:\\CodePCB\\Code\\Pico\\Pico-DSP-Garden\\Examples\\SuperSaw\\SuperSaw.ino"
void setup();
#line 213 "D:\\CodePCB\\Code\\Pico\\Pico-DSP-Garden\\Examples\\SuperSaw\\SuperSaw.ino"
void loop();
#line 38 "D:\\CodePCB\\Code\\Pico\\Pico-DSP-Garden\\Examples\\SuperSaw\\SuperSaw.ino"
void initHypersaw()
{
    Serial.println("Initializing Hypersaw...");

    // --- Initialize Hypersaw Oscillator ---
    hypersaw.Init(SAMPLE_RATE);
    hypersaw.SetFreq(daisysp::mtof(scale[0] + 48)); // Set initial frequency
    hypersaw.SetAllWaveforms(daisysp::Oscillator::WAVE_SAW); // Use saw waves
    Serial.print("Hypersaw frequency set to: ");
    Serial.println(daisysp::mtof(scale[0] + 48));

    // --- Initialize LFO for Detune Modulation ---
    lfo_detune.Init(SAMPLE_RATE);
    lfo_detune.SetWaveform(daisysp::Oscillator::WAVE_SIN);
    lfo_detune.SetFreq(0.1f); // Slow modulation
    lfo_detune.SetAmp(1.0f);

    // --- Initialize LFO for Mix Modulation ---
    lfo_mix.Init(SAMPLE_RATE);
    lfo_mix.SetWaveform(daisysp::Oscillator::WAVE_SIN);
    lfo_mix.SetFreq(0.07f); // Even slower modulation
    lfo_mix.SetAmp(1.0f);

    // Initialize test oscillator
    test_osc.Init(SAMPLE_RATE);
    test_osc.SetWaveform(daisysp::Oscillator::WAVE_SIN);
    test_osc.SetFreq(440.0f); // A4 note
    test_osc.SetAmp(0.3f);

    Serial.println("Hypersaw initialization complete!");
}

// --- Audio Buffer Conversion (remains the same) ---
static inline int16_t convertSampleToInt16(float sample)
{
    float scaled = sample * INT16_MAX_AS_FLOAT;
    scaled = roundf(scaled);
    // Use fclamp from DaisySP for clamping
    scaled = daisysp::fclamp(scaled, INT16_MIN_AS_FLOAT, INT16_MAX_AS_FLOAT);
    return static_cast<int16_t>(scaled);
}



void fill_audio_buffer(audio_buffer_t *buffer)
{
    int N = buffer->max_sample_count;
    int16_t *out = reinterpret_cast<int16_t *>(buffer->buffer->bytes);

    static int debug_counter = 0;
    static float max_signal = 0.0f;

    for (int i = 0; i < N; ++i)
    {
        float mixed_signal;

        if (use_test_tone) {
            // Simple test tone for debugging
            mixed_signal = test_osc.Process();
        } else {
            // 1. Process LFOs to get modulation values
            float lfo_detune_out = lfo_detune.Process(); // -1.0 to 1.0
            float lfo_mix_out = lfo_mix.Process();       // -1.0 to 1.0

            // 2. Remap LFO outputs to the [0.0, 1.0] range for Hypersaw parameters
            float detune_mod = (lfo_detune_out + 1.0f) * 0.5f;
            float mix_mod = (lfo_mix_out + 1.0f) * 0.5f;

            // 3. Set the Hypersaw parameters based on the LFOs
            hypersaw.SetDetune(detune_mod);
            hypersaw.SetMix(mix_mod);

            // 4. Get the final signal from the Hypersaw oscillator
            mixed_signal = hypersaw.Process();
            mixed_signal *= 0.8f;
        }

        // Track maximum signal for debugging
        if (abs(mixed_signal) > max_signal) {
            max_signal = abs(mixed_signal);
        }

        out[2 * i + 0] = convertSampleToInt16(mixed_signal);
        out[2 * i + 1] = convertSampleToInt16(mixed_signal);
    }

    buffer->sample_count = N;

    // Debug output every 1000 buffers (about every 5 seconds at 48kHz)
    debug_counter++;
    if (debug_counter >= 1000) {
        Serial.print("Audio buffer filled, max signal: ");
        Serial.println(max_signal);
        max_signal = 0.0f;
        debug_counter = 0;
    }
}


// --- Setup for Core 1 ---
void setup1()
{
    delay(100);
    Serial.println("[CORE1] Second core started for note progression");
}

// --- Main Application Logic for Core 1 ---
void loop1()
{
    // This loop on the second core will change the note every second.
    // This demonstrates controlling the audio engine from another thread.
    hypersaw.SetFreq(daisysp::mtof(scale[note_index] + 48));
    note_index = (note_index + 1) % 24; // Cycle through the scale

    // Add delay so notes change every second
    delay(1000);
}



// --- Audio I2S Setup ---
void setupI2SAudio(audio_format_t *audioFormat, audio_i2s_config_t *i2sConfig)
{
    if (!audio_i2s_setup(audioFormat, i2sConfig))
    {
        Serial.print("audio failed ");
        delay(1000);

        return;
    }
    if (!audio_i2s_connect(producer_pool))
    {
        Serial.print("audio failed ");

        Serial.print("We are melting!!!!! ");
        delay(1000);

        return;
    }
    audio_i2s_set_enabled(true);
    Serial.println("Audio is ready to go!!!!! ");
    delay(1000);
}

// --- Arduino Setup (Core0) ---
void setup()
{
    // Initialize Serial for debugging
    Serial.begin(115200);
    delay(150);
    Serial.println("Starting SuperSaw...");

    // Initialize the Hypersaw oscillator - THIS WAS MISSING!
    initHypersaw();
    Serial.println("Hypersaw initialized");

    static audio_format_t audioFormat = {
        .sample_freq = (uint32_t)SAMPLE_RATE,
        .format = AUDIO_BUFFER_FORMAT_PCM_S16,
        .channel_count = 2};
    static audio_buffer_format_t bufferFormat = {
        .format = &audioFormat,
        .sample_stride = 4};
    producer_pool = audio_new_producer_pool(&bufferFormat, NUM_AUDIO_BUFFERS, SAMPLES_PER_BUFFER);
    audio_i2s_config_t i2sConfig = {
        .data_pin = PICO_AUDIO_I2S_DATA_PIN,
        .clock_pin_base = PICO_AUDIO_I2S_CLOCK_PIN_BASE,
        .dma_channel = 0,
        .pio_sm = 0};
    setupI2SAudio(&audioFormat, &i2sConfig);

    // Start the second core for note progression - THIS WAS MISSING!
    Serial.println("Starting second core...");
}

void loop()
{
    audio_buffer_t *buf = take_audio_buffer(producer_pool, true);
    if (buf)
    {
        fill_audio_buffer(buf);
        give_audio_buffer(producer_pool, buf);
    }
}


