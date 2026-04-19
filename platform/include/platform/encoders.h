#pragma once
#include <cstdint>
#include <Arduino.h>

struct EncoderEvent {
    int8_t  delta;
    bool    click_pressed;
    bool    click_just_pressed;
    bool    click_just_released;
};

/// Teensy encoder implementation.
/// Rotation: 2-bit shift-register state machine. Click: 8-bit debounce.
///
/// Traits must provide:
///   static constexpr int     kCount;
///   struct PinSet { uint8_t a, b, sw; };
///   static constexpr PinSet  kPins[kCount];  // active-low, INPUT_PULLUP

namespace platform {

template <typename Traits>
class Encoders final {
public:
    static constexpr int kCount = Traits::kCount;

    void init() {
        for (int i = 0; i < kCount; ++i) {
            pinMode(Traits::kPins[i].a,  INPUT_PULLUP);
            pinMode(Traits::kPins[i].b,  INPUT_PULLUP);
            pinMode(Traits::kPins[i].sw, INPUT_PULLUP);
            state_[i] = {};
            state_[i].state_a  = 0xFF;
            state_[i].state_b  = 0xFF;
            state_[i].state_sw = 0xFF;
        }
    }

    void scan() {
        for (int i = 0; i < kCount; ++i) {
            auto& e = state_[i];
            e.state_a = (e.state_a << 1) | digitalReadFast(Traits::kPins[i].a);
            e.state_b = (e.state_b << 1) | digitalReadFast(Traits::kPins[i].b);

            // 2-bit window: rising edge on A with B state determines direction.
            const uint8_t a = e.state_a & 0x03;
            const uint8_t b = e.state_b & 0x03;
            if      (a == 0x02 && b == 0x00) { e.delta =  1; }  // CW
            else if (b == 0x02 && a == 0x00) { e.delta = -1; }  // CCW
            else                             { e.delta =  0; }

            e.state_sw = (e.state_sw << 1) | digitalReadFast(Traits::kPins[i].sw);
        }
    }

    EncoderEvent get(uint8_t idx) const {
        const auto& e  = state_[idx];
        const uint8_t sw = e.state_sw;
        return {
            .delta               = e.delta,
            .click_pressed       = (sw == 0x00),
            .click_just_pressed  = (sw == 0x80),
            .click_just_released = (sw == 0x7F),
        };
    }

private:
    struct EncoderState {
        uint8_t state_a  = 0xFF;
        uint8_t state_b  = 0xFF;
        uint8_t state_sw = 0xFF;
        int8_t  delta    = 0;
    };

    EncoderState state_[kCount];
};

} // namespace platform
