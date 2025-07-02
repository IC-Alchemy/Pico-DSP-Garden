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


// --- Scale for Arpeggiation ---
int scale[] = {

    0, 3, 5, 10, 12, 6, 7, 12,
    7, 6, 7, 8, 10, 6, 4, 2,
    0, 2, 4, 6, 8, 4, 2, 0,
    7, 9, 10, 12, 6, 8, 7, 5
};


volatile int note_index = 0; // Use volatile for core-safe access

void initHypersaw()
{

    hypersaw.Init(SAMPLE_RATE);
    hypersaw.SetFreq(daisysp::mtof(scale[0] + 48)); // Set initial frequency
    hypersaw.SetAllWaveforms(daisysp::Oscillator::WAVE_POLYBLEP_SAW); // Use saw waves

//  adjust the detune and mix parameters here!!!! 
            hypersaw.SetDetune(.51f);
            hypersaw.SetMix(.3f);

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

   

            // 4. Get the final signal from the Hypersaw oscillator
            mixed_signal = hypersaw.Process();
            mixed_signal *= 0.8f;
        

        // Track maximum signal for debugging
        if (abs(mixed_signal) > max_signal) {
            max_signal = abs(mixed_signal);
        }

        out[2 * i + 0] = convertSampleToInt16(mixed_signal);
        out[2 * i + 1] = convertSampleToInt16(mixed_signal);
    }

    buffer->sample_count = N;

    
}


// --- Setup for Core 1 ---
void setup1()
{
    delay(100);
    Serial.println("[CORE1] Second core started for note progression");
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
        Serial.println("audio failed ");

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

}
//  Audio loop, all audio processing happens on this core leaving the other core completely free to do whatever you want
void loop()
{
    audio_buffer_t *buf = take_audio_buffer(producer_pool, true);
    if (buf)
    {
        fill_audio_buffer(buf);
        give_audio_buffer(producer_pool, buf);
    }
}
// --- Main Application Logic for Core 1 ---
void loop1()
{
    hypersaw.SetFreq(daisysp::mtof(scale[note_index] + 48));

//  This code works fine, but it shouldn't be used in "real life"
//  I am using a blocking delay here to show how you can actually do whatever you want
//  in the second core.  

    delay(500);

    note_index = (note_index + 1) % 32; // Cycle through the scale (32 notes total)
}

