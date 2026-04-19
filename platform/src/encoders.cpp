#include "platform/encoders.h"
#include <Arduino.h>  // pinMode, digitalReadFast

namespace oc::platform {

void EncodersImpl::init() {
    for (auto& e : enc_) {
        pinMode(e.pin_a,  INPUT_PULLUP);
        pinMode(e.pin_b,  INPUT_PULLUP);
        pinMode(e.pin_sw, INPUT_PULLUP);
        e.state_a  = 0xFF;
        e.state_b  = 0xFF;
        e.state_sw = 0xFF;
        e.delta    = 0;
    }
}

void EncodersImpl::scan() {
    for (auto& e : enc_) {
        // Advance 8-bit shift registers for both quadrature pins
        e.state_a = (e.state_a << 1) | digitalReadFast(e.pin_a);
        e.state_b = (e.state_b << 1) | digitalReadFast(e.pin_b);

        // 2-bit window: kPinMask=0x03, rising edge=0x02 (b10)
        const uint8_t a = e.state_a & 0x03;
        const uint8_t b = e.state_b & 0x03;

        if (a == 0x02 && b == 0x00) {
            e.delta =  1;   // CW
        } else if (b == 0x02 && a == 0x00) {
            e.delta = -1;   // CCW
        } else {
            e.delta =  0;
        }

        // Switch: 8-bit shift register debounce (active-low)
        e.state_sw = (e.state_sw << 1) | digitalReadFast(e.pin_sw);
    }
}

EncoderEvent EncodersImpl::get(uint8_t idx) const {
    const auto& e = enc_[idx];
    const uint8_t sw = e.state_sw;
    return {
        .delta              = e.delta,
        .click_pressed      = (sw == 0x00),
        .click_just_pressed = (sw == 0x80),
        .click_just_released= (sw == 0x7F),
    };
}

} // namespace oc::platform
