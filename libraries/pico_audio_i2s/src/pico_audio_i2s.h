// pico_audio_i2s.h — Arduino library discovery shim
//
// This file exists solely so arduino-cli's library resolver can detect the
// pico_audio_i2s library. It is NOT intended to be included by user code.
// Include the namespaced headers directly, e.g.:
//   #include <pico_audio_i2s/audio.h>
//   #include <pico_audio_i2s/audio_i2s.h>
//
// The resolver only scans the src/ root for headers; placing a file here
// causes the library to be detected and src/ to be added to the include path,
// which then makes the nested <pico_audio_i2s/...> headers resolvable.

#pragma once
#include "pico_audio_i2s/audio.h"
#include "pico_audio_i2s/audio_i2s.h"
#include "pico_audio_i2s/buffer.h"
