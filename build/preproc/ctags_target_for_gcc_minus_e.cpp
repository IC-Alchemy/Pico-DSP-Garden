# 1 "Z:\\Codezzz\\MusicCode\\Pico-DSP-Garden\\Examples\\Oscillators\\Oscillators.ino"
// Library discovery triggers — arduino-cli detects libraries via root-level src/ headers.
# 3 "Z:\\Codezzz\\MusicCode\\Pico-DSP-Garden\\Examples\\Oscillators\\Oscillators.ino" 2
# 4 "Z:\\Codezzz\\MusicCode\\Pico-DSP-Garden\\Examples\\Oscillators\\Oscillators.ino" 2

# 6 "Z:\\Codezzz\\MusicCode\\Pico-DSP-Garden\\Examples\\Oscillators\\Oscillators.ino" 2
# 7 "Z:\\Codezzz\\MusicCode\\Pico-DSP-Garden\\Examples\\Oscillators\\Oscillators.ino" 2

// Define the number of oscillators

//  pins for the I2S audio most should work fine, but I'm using PCM510x
int PICO_AUDIO_I2S_DATA_PIN = 15;
int PICO_AUDIO_I2S_CLOCK_PIN_BASE = 16; //  Pico forces you to use BasePin + 1 for the LRCK so LRCK is 17


float SAMPLE_RATE = 48000.0f;
float INT16_MAX_AS_FLOAT = 32767.0f;
float INT16_MIN_AS_FLOAT = -32768.0f;
int NUM_AUDIO_BUFFERS = 3;
int SAMPLES_PER_BUFFER = 32;
audio_buffer_pool_t *producer_pool = nullptr;

// Arrays for our carrier oscillators and LFOs
rpdsp::SineOscillator osc1;
rpdsp::SineOscillator osc2;

void initOscillators()
{


        // --- Initialize Carrier Oscillators ---
        osc1.prepare(SAMPLE_RATE);
        osc2.prepare(SAMPLE_RATE);
        // Convert MIDI note to frequency
osc1.setFreq(220.f);
osc2.setFreq(720.f);

    }

// --- Audio Buffer Conversion ---
static inline int16_t convertSampleToInt16(float sample)
{
    float scaled = sample * INT16_MAX_AS_FLOAT;
    scaled = roundf(scaled);
    scaled = rpdsp::clamp(scaled, INT16_MIN_AS_FLOAT, INT16_MAX_AS_FLOAT);
    return static_cast<int16_t>(scaled);
}


/// AUDIO LOOP
void fill_audio_buffer(audio_buffer_t *buffer)
{
    int N = buffer->max_sample_count;
    int16_t *out = reinterpret_cast<int16_t *>(buffer->buffer->bytes);

    for (int i = 0; i < N; ++i)
    {




        // Set the left and right output channels to the final mixed signal

        out[2 * i + 0] = convertSampleToInt16(osc1.process());
        out[2 * i + 1] = convertSampleToInt16(osc2.process());
    }

    buffer->sample_count = N;
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
    delay(150);
    initOscillators();
    static audio_format_t audioFormat = {
        .sample_freq = (uint32_t)SAMPLE_RATE,
        .format = 1 /*|< signed 16bit PCM*/,
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
void setup1()
{
    delay(100);

    Serial.begin(115200);
    Serial.print("[CORE1] Setup starting... ");
    Serial.print("SAMPLE_RATE: ");
    Serial.println(SAMPLE_RATE);
}

// --- Audio Loop (Core0) ---
void loop()
{
    audio_buffer_t *buf = take_audio_buffer(producer_pool, true);
    if (buf)
    {
        fill_audio_buffer(buf);
        give_audio_buffer(producer_pool, buf);
    }
}


void loop1()
{

    //  Do what ever you want in this loop on the other core, they share memory.

}
