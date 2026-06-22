// LadderFilter.ino
// ---------------------------------------------------------------------------
// Ladder-filter demo - Pico-DSP-Garden (RP2350 / Pico 2)
//
// Three detuned band-limited sawtooth oscillators feed a 4-pole Huovilainen
// ladder filter (LP24). A short ADSR shapes each note, and a 16th-note
// sequencer (A-minor blues) drives pitch from Core 1. The filter cutoff sweeps
// 65 Hz -> 1000 Hz across each bar; after each completed sweep the resonance
// advances to the next value in a small cycling list. A momentary button A/Bs
// the envelope so the filtering is audible both with and without amplitude
// gating.
//
// Core split follows the existing example convention (NOT Docs/realtime_rules):
//   Core 0 (setup/loop)   : real-time audio fill callback + DSP graph.
//   Core 1 (setup1/loop1) : sequencing, cutoff/res targets, button, Serial.
//
// Wiring
//   I2S DAC (PCM510x-class):
//     DATA  -> GPIO 15   (PICO_AUDIO_I2S_DATA_PIN)
//     LRCK  -> GPIO 16   (PICO_AUDIO_I2S_CLOCK_PIN_BASE)
//     BCLK  -> GPIO 17   (clock_pin_base + 1, assigned by the driver)
//   Momentary button (envelope A/B):
//     GPIO 0 <-> button <-> GND   (INPUT_PULLUP, active-low; press toggles env)
//
// Build:
//   arduino-cli compile \
//     --fqbn rp2040:rp2040:rpipico2:flash=4194304_0,freq=200,opt=Optimize3,usbstack=picosdk \
//     --library libraries/rpdsp --library libraries/pico_audio_i2s Examples/LadderFilter
// ---------------------------------------------------------------------------

// Library discovery triggers (root-level src/ headers put the libs on the path).
#include <rpdsp.h>
#include <pico_audio_i2s.h>

#include <pico_audio_i2s/audio.h>
#include <pico_audio_i2s/audio_i2s.h>
#include <rpdsp/oscillator.h>          // SecondOrderBSplineSawOscillator (anti-aliased)
#include <rpdsp/ladder.h>              // LadderFilter
#include <rpdsp/envelope.h>            // ADSR
#include <rpdsp/parameter_smoother.h>  // LinearSmoother (rpdsp's canonical smoother)

// ---------------------------------------------------------------------------
// Pin + engine constants
// ---------------------------------------------------------------------------
static const int   PICO_AUDIO_I2S_DATA_PIN       = 15;
static const int   PICO_AUDIO_I2S_CLOCK_PIN_BASE = 16;  // LRCK = 16, BCLK = 17

static const int   BUTTON_PIN  = 0;   // momentary switch, INPUT_PULLUP, active-low
static const int   DEBOUNCE_MS = 20;

static const float SAMPLE_RATE        = 48000.0f;  // == rpdsp::kDefaultSampleRate
static const int   NUM_AUDIO_BUFFERS  = 3;
static const int   SAMPLES_PER_BUFFER = 256;

// ---------------------------------------------------------------------------
// Sequencer: A-minor blues, root A2 = MIDI 45; -1 = rest. One bar of 16ths.
// ---------------------------------------------------------------------------
static const int PATTERN_LEN = 16;
static const int PATTERN[PATTERN_LEN] = {
    45,  -1,  48,  50,   // A2, rest,  C3, D3
    52,  -1,  51,  50,   // E3, rest,  Eb3,D3
    48,  52,  55,  -1,   // C3, E3,    G3, rest
    57,  60,  62,  -1    // A3, C4,    D4, rest   (span A2..D4)
};
static const unsigned long BPM     = 95;
static const unsigned long STEP_US = (60000000UL / BPM) / 4UL;   // 16th step (~157.9 ms)
static const unsigned long GATE_US = (STEP_US * 80UL) / 100UL;   // 80% gate, remainder = release
static const unsigned long BAR_US  = STEP_US * PATTERN_LEN;       // one full sequence

// Cutoff sweeps once per bar; resonance advances to the next entry each bar.
static const float CUTOFF_LOW  = 65.0f;
static const float CUTOFF_HIGH = 1000.0f;
static const int   RES_COUNT   = 4;
static const float RES_VALUES[RES_COUNT] = {0.2f, 0.4f, 0.7f, 0.9f};

// Oscillator detune in cents: osc1 = 0, osc2 = +7, osc3 = -9.
static const float DETUNE_CENTS[3] = {0.0f, 7.0f, -9.0f};

// Output gain staging: makeup gain into a soft-clipper keeps resonant peaks
// from clipping the DAC while adding a touch of saturation.
static const float OUT_MAKEUP    = 2.0f;
static const float OUT_HEADROOM  = 0.75f;

// ---------------------------------------------------------------------------
// Cross-core state (Core 1 writes, Core 0 reads; plain volatile, no locks)
// ---------------------------------------------------------------------------
volatile int   g_note           = -1;          // current MIDI note (-1 = rest)
volatile bool  g_noteOnPending  = false;       // edge: trigger noteOn in audio core
volatile bool  g_noteOffPending = false;       // edge: trigger noteOff in audio core
volatile float g_cutoffTarget   = CUTOFF_LOW;  // per-bar sweep target (Hz)
volatile float g_resTarget      = RES_VALUES[0];
volatile bool  g_envEnabled     = true;        // button toggles; false = bypass ADSR

// ---------------------------------------------------------------------------
// DSP objects (owned by Core 0; used only inside the fill callback)
// ---------------------------------------------------------------------------
static rpdsp::SecondOrderBSplineSawOscillator osc[3];
static rpdsp::LadderFilter   ladder;
static rpdsp::ADSR           adsr;
static rpdsp::LinearSmoother cutoffSmoother;   // click-free cutoff ramp
static rpdsp::LinearSmoother resSmoother;      // click-free resonance ramp

static audio_buffer_pool_t *producer_pool = nullptr;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static inline float centsToRatio(float cents) {
    return powf(2.0f, cents / 1200.0f);
}

// Point all three detuned oscillators at a MIDI note (called on noteOn edges).
static void setNote(int midiNote) {
    const float base = rpdsp::midiNoteToHz(static_cast<float>(midiNote));
    for (int i = 0; i < 3; ++i) {
        osc[i].setFreq(base * centsToRatio(DETUNE_CENTS[i]));
    }
}

// ---------------------------------------------------------------------------
// Audio fill callback - Core 0 hard real-time path
// (no alloc, no Serial/USB, no blocking, no unbounded loops)
// ---------------------------------------------------------------------------
static void fill_audio_buffer(audio_buffer_t *buffer)
{
    const int    N   = static_cast<int>(buffer->max_sample_count);
    int32_t     *dst = reinterpret_cast<int32_t *>(buffer->buffer->bytes);

    // Consume cross-core note edges once per block. Steps are ~158 ms and blocks
    // are ~5 ms, so per-block consumption sits well inside the gate timing.
    // SPSC ordering: Core 1 writes g_note before setting its flag; read note first.
    if (g_noteOnPending) {
        const int n = g_note;
        if (n >= 0) {
            setNote(n);
            adsr.noteOn();
        } else {
            adsr.noteOff();   // defensive: a rest arrived as a note-on edge
        }
        g_noteOnPending = false;
    }
    if (g_noteOffPending) {
        adsr.noteOff();
        g_noteOffPending = false;
    }

    // Publish new control targets once per block; the smoothers ramp per sample.
    cutoffSmoother.setTarget(g_cutoffTarget);
    resSmoother.setTarget(g_resTarget);
    const bool envEnabled = g_envEnabled;   // snapshot once per block

    for (int i = 0; i < N; ++i)
    {
        // 1) Three detuned band-limited saws, gain-staged by /3.
        const float s = (osc[0].process() + osc[1].process() + osc[2].process())
                        * (1.0f / 3.0f);

        // 2) Smoothed cutoff + resonance into the ladder (LP24, default mode).
        ladder.setFreq(cutoffSmoother.next());
        ladder.setRes(resSmoother.next());
        const float filt = ladder.process(s);

        // 3) ADSR amplitude. When bypassed, notes play at full level (A/B).
        const float amp = envEnabled ? adsr.process() : 1.0f;

        // 4) Makeup + soft-clip so resonant peaks never clip the DAC.
        const float outf = rpdsp::softClip(filt * amp * OUT_MAKEUP) * OUT_HEADROOM;

        const int32_t sample = rpdsp::toInt24x32(outf);
        dst[2 * i + 0] = sample;   // left
        dst[2 * i + 1] = sample;   // right (mono -> stereo)
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

    for (int i = 0; i < 3; ++i) osc[i].prepare(SAMPLE_RATE);
    ladder.prepare(SAMPLE_RATE);                 // LP24, drive 0.5, pbg 0.5 by default
    adsr.prepare(SAMPLE_RATE);
    adsr.set(0.010f, 0.100f, 0.5f, 0.2f);      // A 10 ms, D 100 ms, S 0.5, R 88 ms
    cutoffSmoother.prepare(SAMPLE_RATE, 30.0f);  // ~30 ms ramp absorbs the per-bar reset
    cutoffSmoother.reset(CUTOFF_LOW);
    resSmoother.prepare(SAMPLE_RATE, 30.0f);
    resSmoother.reset(RES_VALUES[0]);

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
// Core 1 - setup1 & loop1 (sequencing, cutoff/res targets, button, Serial)
// ---------------------------------------------------------------------------
void setup1()
{
    delay(200);
    Serial.begin(115200);
    Serial.println("[CORE1] LadderFilter demo starting...");
    Serial.print  ("[CORE1] Sample rate: ");
    Serial.println(SAMPLE_RATE);
    Serial.print  ("[CORE1] Tempo:       ");
    Serial.print  (BPM);
    Serial.println(" BPM, 16th-note sequence, A-minor blues");
    Serial.println("[CORE1] Press the GPIO0 button to toggle the envelope A/B");

    pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop1()
{
    static bool          started          = false;
    static unsigned long nextStepUs       = 0;
    static unsigned long nextNoteOffUs    = 0;
    static unsigned long barStartUs       = 0;
    static int           stepIndex        = 0;
    static int           resIndex         = 0;
    static bool          noteOffScheduled = false;

    // button debounce state
    static int           lastButtonState  = HIGH;
    static int           buttonState      = HIGH;
    static unsigned long lastDebounceMs   = 0;

    const unsigned long now = micros();

    if (!started) {
        started     = true;
        nextStepUs  = now;          // fire step 0 immediately
        barStartUs  = now;
        g_resTarget = RES_VALUES[0];
    }

    // ---- Step boundary: publish note-on (or rest) -------------------------
    if (now >= nextStepUs) {
        nextStepUs += STEP_US;
        const int note = PATTERN[stepIndex];
        if (note >= 0) {
            g_note           = note;    // write note first...
            g_noteOnPending  = true;    // ...then the flag (SPSC handoff)
            nextNoteOffUs    = now + GATE_US;   // release at ~80% of the step
            noteOffScheduled = true;
            if (Serial) {
                Serial.print("[CORE1] noteOn  MIDI ");
                Serial.println(note);
            }
        } else {
            g_noteOffPending = true;    // rest: release any ringing note
            noteOffScheduled = false;
        }

        stepIndex = (stepIndex + 1) % PATTERN_LEN;
        if (stepIndex == 0) {           // full sequence complete -> new sweep + next res
            barStartUs  = now;
            resIndex    = (resIndex + 1) % RES_COUNT;
            g_resTarget = RES_VALUES[resIndex];
            if (Serial) {
                Serial.print("[CORE1] bar complete -> resonance ");
                Serial.println(g_resTarget, 2);
            }
        }
    }

    // ---- Note-off at ~80% of the gate -------------------------------------
    if (noteOffScheduled && now >= nextNoteOffUs) {
        g_noteOffPending = true;
        noteOffScheduled = false;
    }

    // ---- Cutoff sweep: 65 -> 1000 Hz over one bar (one full sequence) -----
    const unsigned long elapsed = now - barStartUs;
    float progress = static_cast<float>(elapsed) / static_cast<float>(BAR_US);
    if (progress > 1.0f) progress = 1.0f;
    g_cutoffTarget = CUTOFF_LOW + (CUTOFF_HIGH - CUTOFF_LOW) * progress;

    // ---- Button debounce: toggle envelope A/B -----------------------------
    const int reading = digitalRead(BUTTON_PIN);
    if (reading != lastButtonState) {
        lastDebounceMs = millis();
    }
    if (millis() - lastDebounceMs > DEBOUNCE_MS) {
        if (reading != buttonState) {
            buttonState = reading;
            if (buttonState == LOW) {     // pressed (active-low)
                g_envEnabled = !g_envEnabled;
                if (Serial) {
                    Serial.print("[CORE1] envelope ");
                    Serial.println(g_envEnabled ? "ENABLED" : "BYPASSED");
                }
            }
        }
    }
    lastButtonState = reading;
}
