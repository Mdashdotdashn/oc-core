#pragma once
#include <cstdint>
#include <Arduino.h>

struct ButtonEvent {
    bool pressed;
    bool just_pressed;
    bool just_released;
};

/// Teensy button implementation.
/// Reads N front-panel push-buttons using 8-bit shift-register debounce
/// — identical algorithm to ArticCircle UI/ui_button.h.
///
/// Traits must provide:
///   static constexpr int     kCount;
///   static constexpr uint8_t kPins[kCount];  // active-low, INPUT_PULLUP

namespace platform {

template <typename Traits>
class Buttons final {
public:
    static constexpr int kCount = Traits::kCount;

    void init() {
        for (int i = 0; i < kCount; ++i) {
            pinMode(Traits::kPins[i], INPUT_PULLUP);
            state_[i] = 0xFF;
        }
    }

    /// Poll all button pins and advance shift registers.
    /// Must be called at a fixed rate from the ISR or a UI timer.
    void scan() {
        for (int i = 0; i < kCount; ++i) {
            // Shift history left; insert current sample at LSB.
            // Active-low: HIGH = not pressed (1), LOW = pressed (0).
            state_[i] = (state_[i] << 1) | digitalReadFast(Traits::kPins[i]);
        }
    }

    /// Return debounced event for button idx.
    ButtonEvent get(uint8_t idx) const {
        const uint8_t s = state_[idx];
        return {
            .pressed       = (s == 0x00),
            .just_pressed  = (s == 0x80),
            .just_released = (s == 0x7F),
        };
    }

private:
    // 8-bit shift register per button.
    // 0xFF = idle, 0x80 = just_pressed, 0x00 = held, 0x7F = just_released.
    uint8_t state_[kCount] = {};
};

} // namespace platform
