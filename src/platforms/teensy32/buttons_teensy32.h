#pragma once
#include "oc/hal/buttons.h"
#include <cstdint>

/// Teensy 3.2 button implementation.
/// Reads 2 front-panel push-buttons using 8-bit shift-register debounce
/// — identical algorithm to ArticCircle UI/ui_button.h.
///
/// Pins (from OC_gpio.h):
///   UP   button → pin 5  (but_top, active-low, INPUT_PULLUP)
///   DOWN button → pin 4  (but_bot, active-low, INPUT_PULLUP)

namespace oc::platform::teensy32 {

class ButtonsImpl : public hal::ButtonsInterface {
public:
    void init();

    /// Poll all button pins and advance shift registers.
    /// Must be called at a fixed rate from the ISR or a UI timer.
    void scan() override;

    /// Return debounced event for button idx (0=UP, 1=DOWN).
    hal::ButtonEvent get(uint8_t idx) const override;

private:
    // Physical pins: UP=5, DOWN=4 (active-low, INPUT_PULLUP)
    static constexpr uint8_t kPins[kCount] = {5, 4};

    // 8-bit shift register per button.
    // digitalReadFast() inserts 1 (not pressed) or 0 (pressed) each poll.
    // 0xFF = idle, 0x80 = just_pressed, 0x00 = held, 0x7F = just_released.
    uint8_t state_[kCount] = {0xFF, 0xFF};
};

} // namespace oc::platform::teensy32
