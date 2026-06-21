#include <rpdsp/oscillator.h>

#include "doctest.h"

TEST_CASE("SineOscillator output stays in [-1, 1]") {
    rpdsp::SineOscillator osc;
    osc.prepare(48000.0f);
    osc.setFreq(440.0f);
    for (int i = 0; i < 1000; ++i) {
        float v = osc.process();
        CHECK(v >= -1.0f);
        CHECK(v <= 1.0f);
    }
}

TEST_CASE("SawOscillator output stays in [-1, 1]") {
    rpdsp::SawOscillator osc;
    osc.prepare(48000.0f);
    osc.setFreq(220.0f);
    for (int i = 0; i < 1000; ++i) {
        float v = osc.process();
        CHECK(v >= -1.0f);
        CHECK(v <= 1.0f);
    }
}

TEST_CASE("Phasor wraps in [0, 1)") {
    rpdsp::Phasor phasor;
    phasor.prepare(48000.0f);
    // 1 Hz at 48000 Hz: period = 48000 samples, advance = 1/48000 per sample.
    phasor.setFreq(1.0f);
    for (int i = 0; i < 100000; ++i) {
        float p = phasor.process();
        CHECK(p >= 0.0f);
        CHECK(p < 1.0f);
    }
}
