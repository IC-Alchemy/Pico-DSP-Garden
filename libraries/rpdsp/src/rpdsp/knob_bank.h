#pragma once

#include "pickup_knob.h"

#include <array>
#include <cstddef>

namespace rpdsp {

// Each bank stores independent parameter values while sharing one physical knob set.
template <size_t NumBanks, size_t NumKnobs>
class KnobBank {
 public:
  void reset(float defaultValue = 0.0f, float pickupThreshold = 0.01f) {
    currentBank_ = 0;
    for (size_t bank = 0; bank < NumBanks; ++bank) {
      for (size_t knob = 0; knob < NumKnobs; ++knob)
        values_[bank][knob] = defaultValue;
    }
    for (size_t knob = 0; knob < NumKnobs; ++knob)
      pickups_[knob].reset(defaultValue, pickupThreshold);
  }

  void setBank(size_t bank) {
    if (bank >= NumBanks || bank == currentBank_)
      return;

    currentBank_ = bank;
    // Re-arm every physical knob so the new bank's stored values do not jump on selection.
    for (size_t knob = 0; knob < NumKnobs; ++knob)
      pickups_[knob].arm(values_[currentBank_][knob]);
  }

  void processKnob(size_t knob, float mappedValue) {
    if (knob >= NumKnobs)
      return;

    // Stored bank values update only after their physical controls have picked up.
    if (pickups_[knob].process(mappedValue))
      values_[currentBank_][knob] = mappedValue;
  }

  void process(const std::array<float, NumKnobs>& mappedValues) {
    for (size_t knob = 0; knob < NumKnobs; ++knob)
      processKnob(knob, mappedValues[knob]);
  }

  [[nodiscard]] float value(size_t bank, size_t knob) const {
    if (bank >= NumBanks || knob >= NumKnobs)
      return 0.0f;
    return values_[bank][knob];
  }

  [[nodiscard]] float currentValue(size_t knob) const { return values_[currentBank_][knob]; }
  [[nodiscard]] bool pickedUp(size_t knob) const { return pickups_[knob].pickedUp(); }
  [[nodiscard]] size_t currentBank() const { return currentBank_; }

 private:
  size_t currentBank_ = 0;
  std::array<std::array<float, NumKnobs>, NumBanks> values_{};
  std::array<PickupKnob, NumKnobs> pickups_{};
};

}  // namespace rpdsp
