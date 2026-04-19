#include "platform/buttons.h"
#include <Arduino.h>  // pinMode, INPUT_PULLUP, digitalReadFast

namespace platform {

void Buttons::init() {
    for (int i = 0; i < kCount; ++i) {
        pinMode(kPins[i], INPUT_PULLUP);
        state_[i] = 0xFF;  // start in released state
    }
}

void Buttons::scan() {
    for (int i = 0; i < kCount; ++i) {
        // Shift history left, insert current pin sample at LSB.
        // Pin HIGH = not pressed (1), Pin LOW = pressed (0) — active-low.
        state_[i] = (state_[i] << 1) | digitalReadFast(kPins[i]);
    }
}

ButtonEvent Buttons::get(uint8_t idx) const {
    const uint8_t s = state_[idx];
    return {
        .pressed       = (s == 0x00),   // all 8 samples pressed
        .just_pressed  = (s == 0x80),   // 7th consecutive press sample
        .just_released = (s == 0x7F),   // 7th consecutive release sample
    };
}

} // namespace platform
