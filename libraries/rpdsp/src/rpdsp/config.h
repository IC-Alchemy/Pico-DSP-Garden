#pragma once

#include <cstddef>

#ifndef RPDSP_BLOCK_SIZE
#define RPDSP_BLOCK_SIZE 32
#endif

namespace rpdsp {

// The portable DSP layer is tuned around short, predictable audio callbacks.
inline constexpr float kDefaultSampleRate = 48000.0f;
inline constexpr size_t kDefaultBlockSize = RPDSP_BLOCK_SIZE;
inline constexpr float kPi = 3.14159265358979323846f;
inline constexpr float kTwoPi = 6.28318530717958647692f;

// Keep host tests and firmware examples on the same small set of realtime block budgets.
static_assert(kDefaultBlockSize == 16 || kDefaultBlockSize == 32 || kDefaultBlockSize == 64,
              "RPDSP_BLOCK_SIZE must be 16, 32, or 64");

}  // namespace rpdsp
