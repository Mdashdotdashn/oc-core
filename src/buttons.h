#pragma once
#include <cstdint>

namespace oc {

struct ButtonEvent {
    bool pressed;
    bool just_pressed;
    bool just_released;
};

/// Teensy 3.2 button implementation.
/// Reads 2 front-panel push-buttons using 8-bit shift-register debounce
/// — identical algorithm to ArticCircle UI/ui_button.h.
///
/// Pins (from OC_gpio.h):
///   UP   button → pin 5  (but_top, active-low, INPUT_PULLUP)
///   DOWN button → pin 4  (but_bot, active-low, INPUT_PULLUP)
class ButtonsImpl final {
public:
    static constexpr int kCount = 2;

    void init();

    void scan();
    ButtonEvent get(uint8_t idx) const;

private:
    static constexpr uint8_t kPins[kCount] = {5, 4};
    uint8_t state_[kCount] = {0xFF, 0xFF};
};

} // namespace oc

