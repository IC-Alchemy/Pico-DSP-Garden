// Compile-every-header test. Include every rpdsp header to catch
// missing-type / doc-drift bugs at compile time. The body below ensures
// the includes are not optimized away.

#include <rpdsp/algorithm.h>
#include <rpdsp/analysis.h>
#include <rpdsp/clock_tracker.h>
#include <rpdsp/config.h>
#include <rpdsp/delay_line.h>
#include <rpdsp/dynamics.h>
#include <rpdsp/effects.h>
#include <rpdsp/envelope.h>
#include <rpdsp/filter.h>
#include <rpdsp/gate_pattern.h>
#include <rpdsp/hardware_interpolator.h>
#include <rpdsp/hypersaw.h>
#include <rpdsp/joystick_recorder.h>
#include <rpdsp/knob_bank.h>
#include <rpdsp/ladder.h>
#include <rpdsp/oscillator.h>
#include <rpdsp/parameter_smoother.h>
#include <rpdsp/pickup_knob.h>
#include <rpdsp/realtime.h>
#include <rpdsp/rhythm_sequencer.h>
#include <rpdsp/voice.h>
#include <rpdsp/waveguide.h>

#include "doctest.h"

TEST_CASE("rpdsp headers compile and link") {
    // Touch a few symbols so the compiler can't discard the includes.
    CHECK(rpdsp::clamp(5.0f, 0.0f, 3.0f) == doctest::Approx(3.0f));
    CHECK(rpdsp::midiNoteToHz(69) == doctest::Approx(440.0f).epsilon(0.01f));
}
