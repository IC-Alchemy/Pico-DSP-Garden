# 1 "D:\\CodePCB\\Code\\Pico\\Pico-DSP-Garden\\Examples\\Oscillators\\Oscillators.ino"
# 2 "D:\\CodePCB\\Code\\Pico\\Pico-DSP-Garden\\Examples\\Oscillators\\Oscillators.ino" 2
# 3 "D:\\CodePCB\\Code\\Pico\\Pico-DSP-Garden\\Examples\\Oscillators\\Oscillators.ino" 2
# 4 "D:\\CodePCB\\Code\\Pico\\Pico-DSP-Garden\\Examples\\Oscillators\\Oscillators.ino" 2
// Define the number of oscillators
int PICO_AUDIO_I2S_DATA_PIN = 15;
int PICO_AUDIO_I2S_CLOCK_PIN_BASE = 16;
float SAMPLE_RATE = 44100.0f;
float INT16_MAX_AS_FLOAT = 32767.0f;
float INT16_MIN_AS_FLOAT = -32768.0f;
int NUM_AUDIO_BUFFERS = 3;
int SAMPLES_PER_BUFFER = 256;
audio_buffer_pool_t *producer_pool = nullptr;

const int NUM_OSCILLATORS = 12;
// Arrays for our carrier oscillators and LFOs
daisysp::Oscillator carrier_osc[NUM_OSCILLATORS];
daisysp::Oscillator lfo_mod[NUM_OSCILLATORS];
    const float lfo_min_freq = 0.011f;
    const float lfo_max_freq = .3f;
int scale[48] = {
    // Pentatonic Mino
    0, 0, 3, 3, 5, 5, 7, 9, 10, 10, 12, 12, 15, 15, 17, 17,
    19, 21, 22, 22, 24, 24, 27, 29, 29, 31, 32, 32, 34, 34, 36, 36,
    39, 39, 41, 41, 43, 43, 46, 46, 48, 48, 51, 53, 53, 53, 53, 53};

int change = 1;
void initOscillators()
{

    // Loop to initialize all oscillators
    for (int i = 0; i < NUM_OSCILLATORS; i++)
    {
        // --- Initialize Carrier Oscillators ---
        carrier_osc[i].Init(SAMPLE_RATE);
        carrier_osc[i].SetWaveform(daisysp::Oscillator::WAVE_SIN);
        // Convert MIDI note to frequency
        carrier_osc[i].SetFreq(daisysp::mtof(scale[i*2 ] + 56));
        // Initial amplitude will be controlled by LFO, but we set it here for safety.
        carrier_osc[i].SetAmp(0.5f);

        // --- Initialize LFO Modulators ---
        lfo_mod[i].Init(SAMPLE_RATE);
        lfo_mod[i].SetWaveform(daisysp::Oscillator::WAVE_SIN);
        // Linearly distribute LFO frequencies from min to max
        float lfo_freq = lfo_min_freq + (static_cast<float>(i) / (NUM_OSCILLATORS - 1)) * (lfo_max_freq - lfo_min_freq);
        lfo_mod[i].SetFreq(lfo_freq);
        // LFO amplitude is 1.0 so its output is a full [-1.0, 1.0]
        lfo_mod[i].SetAmp(1.0f);
    }
}
// --- Audio Buffer Conversion ---
static inline int16_t convertSampleToInt16(float sample)
{
    float scaled = sample * INT16_MAX_AS_FLOAT;
    scaled = roundf(scaled);
    scaled = daisysp::fclamp(scaled, INT16_MIN_AS_FLOAT, INT16_MAX_AS_FLOAT);
    return static_cast<int16_t>(scaled);
}

void fill_audio_buffer(audio_buffer_t *buffer)
{
    int N = buffer->max_sample_count;
    int16_t *out = reinterpret_cast<int16_t *>(buffer->buffer->bytes);

    for (int i = 0; i < N; ++i)
    {

        float mixed_signal = 0.f;

        // Process and sum all 16 oscillators
        for (int j = 0; j < NUM_OSCILLATORS; j++)
        {
            // Get the LFO value, which is in the range [-1.0, 1.0]
            float lfo_out = lfo_mod[j].Process();

            // Remap the LFO output to the range [0.0, 1.0] for amplitude control
            // (lfo_out + 1) / 2 moves [-1, 1] to [0, 2] and then to [0, 1]
            float amp_mod = (lfo_out + 1.0f) * 0.5f;

            // Set the carrier oscillator's amplitude to the LFO's value
            carrier_osc[j].SetAmp(amp_mod);

            // Get the carrier's signal and add it to our mix
            mixed_signal += carrier_osc[j].Process();
        }

        // Attenuate the mixed signal to prevent clipping by dividing by the number of oscillators.
        // This is a simple but effective mixing strategy.
        mixed_signal *= 0.25f;

        // Set the left and right output channels to the final mixed signal

        out[2 * i + 0] = convertSampleToInt16(mixed_signal);
        out[2 * i + 1] = convertSampleToInt16(mixed_signal);
    }

    buffer->sample_count = N;
}

// --- Audio I2S Setup ---
void setupI2SAudio(audio_format_t *audioFormat, audio_i2s_config_t *i2sConfig)
{
    if (!audio_i2s_setup(audioFormat, i2sConfig))
    {
        Serial.print("We are melting!!!!! ");
        delay(1000);

        return;
    }
    if (!audio_i2s_connect(producer_pool))
    {

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

//  Do what ever you want in this loop on the other core, they share memory.

void loop1()
{
    if (random(0,1000)<1){
delay(100);
change = random(13);
for (int i = 0; i < NUM_OSCILLATORS; i++)
    {

        carrier_osc[i].SetFreq(daisysp::mtof(scale[i*2 ] + 36+change));

    }
}}
