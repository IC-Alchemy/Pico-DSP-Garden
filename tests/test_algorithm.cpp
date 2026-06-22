#include <rpdsp/algorithm.h>
#include <rpdsp/realtime.h>

#include "doctest.h"

TEST_CASE("clamp respects bounds") {
    CHECK(rpdsp::clamp(5.0f, 0.0f, 3.0f) == doctest::Approx(3.0f));
    CHECK(rpdsp::clamp(-5.0f, 0.0f, 3.0f) == doctest::Approx(0.0f));
    CHECK(rpdsp::clamp(1.5f, 0.0f, 3.0f) == doctest::Approx(1.5f));
}

TEST_CASE("clamp01 maps to [0,1]") {
    CHECK(rpdsp::clamp01(2.0f) == doctest::Approx(1.0f));
    CHECK(rpdsp::clamp01(-1.0f) == doctest::Approx(0.0f));
    CHECK(rpdsp::clamp01(0.3f) == doctest::Approx(0.3f));
}

TEST_CASE("toInt24x32 packs 24-in-32 left-justified") {
    // Scale is 2^23 - 1 (8388607) with truncation toward zero, then << 8.
    CHECK(rpdsp::toInt24x32(0.0f) == 0);

    // +1.0 -> +8388607 (0x7FFFFF) << 8 = 0x7FFFFF00 (positive full-scale).
    CHECK(rpdsp::toInt24x32(1.0f) == static_cast<int32_t>(0x7FFFFF00));

    // -1.0 -> -8388607 (0xFF800001) << 8 = 0x80000100. Note: with symmetric
    // scaling this is one 24-bit LSB shy of true negative full-scale (0x80000000),
    // which is the standard, inaudible consequence of scaling by 2^23 - 1.
    CHECK(rpdsp::toInt24x32(-1.0f) == static_cast<int32_t>(0x80000100));

    // 0.5 -> 4194303 (0x3FFFFF, truncated from 4194303.5) << 8 = 0x3FFFFF00.
    CHECK(rpdsp::toInt24x32(0.5f) == static_cast<int32_t>(0x3FFFFF00));

    // Low 8 bits are always zero (DAC padding).
    CHECK((rpdsp::toInt24x32(0.123f) & 0xFF) == 0);
    CHECK((rpdsp::toInt24x32(-0.777f) & 0xFF) == 0);
}

TEST_CASE("toInt24x32 is monotonic across the range") {
    CHECK(rpdsp::toInt24x32(-0.5f) < rpdsp::toInt24x32(0.0f));
    CHECK(rpdsp::toInt24x32(0.0f) < rpdsp::toInt24x32(0.5f));
    CHECK(rpdsp::toInt24x32(0.25f) < rpdsp::toInt24x32(0.75f));
}

TEST_CASE("toInt24x32 clamps out-of-range input") {
    CHECK(rpdsp::toInt24x32(2.0f) == rpdsp::toInt24x32(1.0f));
    CHECK(rpdsp::toInt24x32(-2.0f) == rpdsp::toInt24x32(-1.0f));
    CHECK(rpdsp::toInt24x32(100.0f) == static_cast<int32_t>(0x7FFFFF00));
}

TEST_CASE("midiNoteToHz: A4 = 440, A5 = 880") {
    CHECK(rpdsp::midiNoteToHz(69) == doctest::Approx(440.0f).epsilon(0.001f));
    CHECK(rpdsp::midiNoteToHz(81) == doctest::Approx(880.0f).epsilon(0.001f));
}

TEST_CASE("dbToGain: 0 dB = unity, 20 dB = 10x, -20 dB = 0.1x") {
    CHECK(rpdsp::dbToGain(0.0f) == doctest::Approx(1.0f).epsilon(0.001f));
    CHECK(rpdsp::dbToGain(20.0f) == doctest::Approx(10.0f).epsilon(0.001f));
    CHECK(rpdsp::dbToGain(-20.0f) == doctest::Approx(0.1f).epsilon(0.001f));
}

TEST_CASE("softClip: 0 -> 0, bounded by 1, 1.0 -> 0.5") {
    CHECK(rpdsp::softClip(0.0f) == doctest::Approx(0.0f));
    // softClip(x) = x / (1 + |x|); as x -> inf, approaches 1.
    CHECK(rpdsp::softClip(1000.0f) < 1.0f);
    CHECK(rpdsp::softClip(-1000.0f) > -1.0f);
    CHECK(rpdsp::softClip(1.0f) == doctest::Approx(0.5f));
}

TEST_CASE("wrap01 wraps phase to [0,1)") {
    CHECK(rpdsp::wrap01(0.5f) == doctest::Approx(0.5f));
    CHECK(rpdsp::wrap01(1.5f) == doctest::Approx(0.5f));
    CHECK(rpdsp::wrap01(-0.5f) == doctest::Approx(0.5f));
}

TEST_CASE("zapDenormal zeroes tiny values, passes normal ones") {
    CHECK(rpdsp::zapDenormal(1e-25f) == doctest::Approx(0.0f));
    CHECK(rpdsp::zapDenormal(0.5f) == doctest::Approx(0.5f));
}
