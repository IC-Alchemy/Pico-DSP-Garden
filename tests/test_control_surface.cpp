#include <rpdsp/control_surface.h>
#include <rpdsp/knob_bank.h>

#include "doctest.h"

#include <array>
#include <cstddef>
#include <vector>

TEST_CASE("MuxSliderScanner selects channels and median filters ADC reads") {
    rpdsp::MuxSliderScanner<4> scanner;
    scanner.prepare(1000.0f, 0.0f);

    std::vector<std::size_t> selected;
    std::array<std::array<int, 3>, 4> reads{{
        {{0, 1023, 0}},
        {{1023, 1023, 0}},
        {{512, 510, 900}},
        {{128, 128, 127}},
    }};
    std::size_t currentChannel = 0;
    std::array<std::size_t, 4> readIndex{{0, 0, 0, 0}};
    int settleCalls = 0;

    scanner.scan(
        [&](std::size_t channel) {
            currentChannel = channel;
            selected.push_back(channel);
        },
        [&]() { ++settleCalls; },
        [&]() {
            const int value = reads[currentChannel][readIndex[currentChannel]];
            ++readIndex[currentChannel];
            return value;
        });

    CHECK(selected == std::vector<std::size_t>{0, 1, 2, 3});
    CHECK(settleCalls == 4);
    CHECK(scanner.quantizedValue(0) == 0);
    CHECK(scanner.quantizedValue(1) == 127);
    CHECK(scanner.quantizedValue(2) == 64);
    CHECK(scanner.quantizedValue(3) == 16);
    CHECK(scanner.value(1) == doctest::Approx(1.0f));
    CHECK(scanner.value(2) == doctest::Approx(64.0f / 127.0f));
}

TEST_CASE("MuxSliderScanner ignores ADC noise inside the 7-bit hysteresis bucket") {
    rpdsp::MuxSliderScanner<1> scanner;
    scanner.prepare(1000.0f, 0.0f);

    int adc = 512;
    scanner.scan([](std::size_t) {}, []() {}, [&]() { return adc; });
    const int firstBucket = scanner.quantizedValue(0);
    const float firstValue = scanner.value(0);

    adc = 513;
    scanner.scan([](std::size_t) {}, []() {}, [&]() { return adc; });

    CHECK(scanner.quantizedValue(0) == firstBucket);
    CHECK(scanner.value(0) == doctest::Approx(firstValue));
}

TEST_CASE("DebouncedButton emits one pressed edge after stable debounce time") {
    rpdsp::DebouncedButton button;
    button.reset(false, 0);

    CHECK_FALSE(button.update(true, 1));
    CHECK_FALSE(button.held());

    CHECK_FALSE(button.update(true, 5));
    CHECK_FALSE(button.held());

    CHECK(button.update(true, 6));
    CHECK(button.pressed());
    CHECK(button.held());

    CHECK_FALSE(button.update(true, 7));
    CHECK_FALSE(button.pressed());
    CHECK(button.held());

    CHECK_FALSE(button.update(false, 8));
    CHECK(button.held());
    CHECK_FALSE(button.update(false, 14));
    CHECK_FALSE(button.held());
}

TEST_CASE("KnobBank pickup holds secondary layer until physical control crosses stored value") {
    rpdsp::KnobBank<2, 1> bank;
    bank.reset(0.25f, 0.02f);
    bank.processKnob(0, 0.75f);
    CHECK(bank.currentValue(0) == doctest::Approx(0.75f));

    bank.setBank(1);
    CHECK(bank.currentValue(0) == doctest::Approx(0.25f));
    CHECK_FALSE(bank.pickedUp(0));

    bank.processKnob(0, 0.70f);
    CHECK(bank.currentValue(0) == doctest::Approx(0.25f));
    CHECK_FALSE(bank.pickedUp(0));

    bank.processKnob(0, 0.26f);
    CHECK(bank.pickedUp(0));
    CHECK(bank.currentValue(0) == doctest::Approx(0.26f));
}
