// rpdsp.h — Arduino library discovery shim
//
// This file exists solely so arduino-cli's library resolver can detect the
// rpdsp library. It is NOT intended to be included by user code.
// Include the namespaced headers directly, e.g.:
//   #include <rpdsp/oscillator.h>
//   #include <rpdsp/filter.h>
//
// The resolver only scans the src/ root for headers; placing a file here
// causes the library to be detected and src/ to be added to the include path,
// which then makes the nested <rpdsp/...> headers resolvable.

#pragma once
#include "rpdsp/algorithm.h"
#include "rpdsp/analysis.h"
#include "rpdsp/clock_tracker.h"
#include "rpdsp/config.h"
#include "rpdsp/delay_line.h"
#include "rpdsp/dynamics.h"
#include "rpdsp/effects.h"
#include "rpdsp/envelope.h"
#include "rpdsp/filter.h"
#include "rpdsp/gate_pattern.h"
#include "rpdsp/hypersaw.h"
#include "rpdsp/joystick_recorder.h"
#include "rpdsp/knob_bank.h"
#include "rpdsp/ladder.h"
#include "rpdsp/oscillator.h"
#include "rpdsp/parameter_smoother.h"
#include "rpdsp/pickup_knob.h"
#include "rpdsp/realtime.h"
#include "rpdsp/rhythm_sequencer.h"
#include "rpdsp/voice.h"
#include "rpdsp/waveguide.h"
