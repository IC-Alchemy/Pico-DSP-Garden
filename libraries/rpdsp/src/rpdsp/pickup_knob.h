#pragma once

#include <cmath>

namespace rpdsp {

// Software takeover helper: after a bank/preset switch the stored value is held
// until the physical control crosses back within the pickup window, avoiding jumps.
class PickupKnob {
 public:
  // Start picked up so the software value follows the physical control immediately.
  void reset(float currentValue = 0.0f, float pickupThreshold = 0.01f) {
    value_ = currentValue;
    targetOnSwitch_ = currentValue;
    threshold_ = pickupThreshold;
    pickedUp_ = true;
  }

  void arm(float storedValue) {
    targetOnSwitch_ = storedValue;
    value_ = storedValue;
    // Hold the stored value until the physical knob reaches the pickup window.
    pickedUp_ = false;
  }

  bool process(float physicalValue) {
    // Pickup avoids jumps after switching presets/banks; the threshold assumes normalized values.
    if (!pickedUp_)
      pickedUp_ = std::fabs(physicalValue - targetOnSwitch_) <= threshold_;

    if (pickedUp_)
      value_ = physicalValue;

    return pickedUp_;
  }

  [[nodiscard]] float value() const { return value_; }
  [[nodiscard]] bool pickedUp() const { return pickedUp_; }
  void setThreshold(float threshold) { threshold_ = threshold; }

 private:
  float value_ = 0.0f;
  float targetOnSwitch_ = 0.0f;
  float threshold_ = 0.01f;
  bool pickedUp_ = true;
};

}  // namespace rpdsp
