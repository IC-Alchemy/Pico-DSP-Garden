#pragma once
#ifndef RPDSP_LADDER_H
#define RPDSP_LADDER_H

/* Ported from the Audio Library for Teensy, Ladder Filter.
 * Copyright (c) 2021, Richard van Hoesel
 * Copyright (c) 2024, Infrasonic Audio LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

//-----------------------------------------------------------
// Huovilainen New Moog (HNM) model as per CMJ Jun 2006.
// Richard van Hoesel, v.1.03, Feb. 14 2021.
// v1.7 (Infrasonic/Daisy): add configurable filter mode.
// v1.6 (Infrasonic/Daisy): removes polyphase FIR, uses 4x linear
//      oversampling for performance reasons.
// v1.4: FC extended to 18.7 kHz, max res to 1.8, 4x oversampling,
//       and a minor Q-tuning adjustment.
// v.1.03: adds oversampling, extended resonance, and exposes the
//         input_drive and passband_gain parameters.
// please retain this header if you use this code.
//
// rpdsp port: MIT; adapted to rpdsp conventions (lowercase API,
// prepare()/process(), header-only). The algorithm and all tuning
// constants are preserved verbatim from the source -- only the API
// surface, naming, a zapDenormal guard on the LPF state, and removal of
// two dead state members (beta_, drive_scaled_) were changed.
//-----------------------------------------------------------

#include "algorithm.h"
#include "realtime.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace rpdsp {

/** @brief 4-pole Huovilainen "New Moog" ladder filter.
 *
 * Selectable response (LP/BP/HP at 12 or 24 dB/oct), input drive into a
 * tanh clipper, passband-gain compensation, and stable self-oscillation.
 *
 * This is the heaviest filter in rpdsp: every output sample runs 4x
 * oversampling, each pass doing 4 one-pole stages plus a fastTanh. The
 * oversampling factor is the public constant kInterpolation -- drop it to
 * 2 on RP2350 if the realtime budget is blown. alpha_ and sr_int_recip_
 * both derive from it, so the tuning self-adjusts when it changes.
 */
class LadderFilter {
 public:
  enum class Mode {
    LP24,
    LP12,
    BP24,
    BP12,
    HP24,
    HP12
  };

  // Oversampling factor. Lowering it trades HF accuracy for CPU.
  static constexpr std::uint8_t kInterpolation = 4;

  void prepare(float sampleRate) {
    sample_rate_ = safeSampleRate(sampleRate);
    sr_int_recip_ = 1.0f / (sample_rate_ * kInterpolation);
    alpha_ = 1.0f;
    K_ = 1.0f;
    Fbase_ = 1000.0f;
    Qadjust_ = 1.0f;
    oldinput_ = 0.0f;
    mode_ = Mode::LP24;

    setPassbandGain(0.5f);
    setInputDrive(0.5f);
    setFreq(5000.0f);
    setRes(0.2f);
  }

  void reset() {
    for (int i = 0; i < 4; ++i) {
      z0_[i] = 0.0f;
      z1_[i] = 0.0f;
    }
    oldinput_ = 0.0f;
  }

  float process(float in) {
    float input = in * drive_;
    float total = 0.0f;
    float interp = 0.0f;
    for (std::size_t os = 0; os < kInterpolation; os++) {
      float u = (interp * oldinput_ + (1.0f - interp) * input)
                - (z1_[3] - pbg_ * input) * K_ * Qadjust_;
      u = fastTanh(u);
      float stage1 = lpf(u, 0);
      float stage2 = lpf(stage1, 1);
      float stage3 = lpf(stage2, 2);
      float stage4 = lpf(stage3, 3);
      total += weightedSumForCurrentMode(
                   {input, stage1, stage2, stage3, stage4})
               * kInterpolationRecip;
      interp += kInterpolationRecip;
    }
    oldinput_ = input;
    return total;
  }

#if defined(__GNUC__)
  __attribute__((optimize("unroll-loops")))
#endif
  void process(float* buf, std::size_t size) {
    for (std::size_t i = 0; i < size; i++) {
      buf[i] = process(buf[i]);
    }
  }

  void setFreq(float freq) {
    Fbase_ = freq;
    computeCoeffs(freq);
  }

  void setRes(float res) {
    // Maps resonance 0..1 onto K = 0..4 (clamped to kMaxResonance first).
    res = clamp(res, 0.0f, kMaxResonance);
    K_ = 4.0f * res;
  }

  /** Passband-gain compensation, 0..0.5. Mitigates passband loss at high Q. */
  void setPassbandGain(float pbg) {
    pbg_ = clamp(pbg, 0.0f, 0.5f);
    setInputDrive(drive_);
  }

  /** Drive into the tanh clipper, 0..4. */
  void setInputDrive(float drv) {
    drive_ = std::max(drv, 0.0f);
    if (drive_ > 1.0f) {
      drive_ = std::min(drive_, 4.0f);
    }
  }

  void setMode(Mode mode) { mode_ = mode; }

 private:
  static constexpr float kInterpolationRecip = 1.0f / kInterpolation;
  static constexpr float kMaxResonance = 1.8f;

  // Huovilainen one-pole section. The hardcoded 1/1.3 and 0.3/1.3 are the
  // section coefficients -- do not simplify or retune them.
  float lpf(float s, int i) {
    float ft = s * 0.76923077f + 0.23076923f * z0_[i] - z1_[i];
    ft = ft * alpha_ + z1_[i];
    z1_[i] = zapDenormal(ft);
    z0_[i] = s;
    return z1_[i];
  }

  void computeCoeffs(float freq) {
    // Model-tuned clamp [5, sr*0.425]: the 0.425 (not 0.5) keeps the alpha_
    // polynomial stable -- do NOT swap in rpdsp::clampCutoff here.
    freq = clamp(freq, 5.0f, sample_rate_ * 0.425f);
    float wc = freq * kTwoPi * sr_int_recip_;
    float wc2 = wc * wc;
    alpha_ = 0.9892f * wc - 0.4324f * wc2 + 0.1381f * wc * wc2
             - 0.0202f * wc2 * wc2;
    // Qadjust_ is a matched pair with alpha_ (revised hfQ, rvh Feb 14 2021).
    Qadjust_ = 1.006f + 0.0536f * wc - 0.095f * wc2 - 0.05f * wc2 * wc2;
  }

  // Weighted stage mixing per Valimaki & Huovilainen, CMJ 2006.
  float weightedSumForCurrentMode(const std::array<float, 5>& stage_outs) {
    switch (mode_) {
      case Mode::LP24: return stage_outs[4];
      case Mode::LP12: return stage_outs[2];
      case Mode::BP24:
        return (stage_outs[2] + stage_outs[4]) * 4.0f
               - stage_outs[3] * 8.0f;
      case Mode::BP12: return (stage_outs[1] - stage_outs[2]) * 2.0f;
      case Mode::HP24:
        return stage_outs[0] + stage_outs[4]
               - ((stage_outs[1] + stage_outs[3]) * 4.0f)
               + stage_outs[2] * 6.0f;
      case Mode::HP12:
        return stage_outs[0] + stage_outs[2] - stage_outs[1] * 2.0f;
      default: return 0.0f;
    }
  }

  float sample_rate_ = kDefaultSampleRate;
  float sr_int_recip_ = 0.0f;
  float alpha_ = 1.0f;
  float z0_[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  float z1_[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  float K_ = 1.0f;
  float Fbase_ = 1000.0f;
  float Qadjust_ = 1.0f;
  float pbg_ = 0.5f;
  float drive_ = 0.5f;
  float oldinput_ = 0.0f;
  Mode mode_ = Mode::LP24;
};

}  // namespace rpdsp

#endif  // RPDSP_LADDER_H
