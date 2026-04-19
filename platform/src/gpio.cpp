#include "platform/gpio.h"
#include <Arduino.h>   // digitalReadFast

namespace oc::platform {

GPIOImpl::GPIOImpl() = default;

void GPIOImpl::init() {
    for (int i = 0; i < 4; ++i) {
        pinMode(kPins[i], INPUT_PULLUP);  // TR1-TR4 are active-low gate inputs
    }
}

void GPIOImpl::scan() {
    // Gate inputs on O&C are active-low (inverted).
    const uint8_t current_state_mask =
        (static_cast<uint8_t>(!digitalReadFast(0)) << 0) |
        (static_cast<uint8_t>(!digitalReadFast(1)) << 1) |
        (static_cast<uint8_t>(!digitalReadFast(2)) << 2) |
        (static_cast<uint8_t>(!digitalReadFast(3)) << 3);

    current_state_mask_ = current_state_mask;
    edge_mask_ = static_cast<uint32_t>(current_state_mask_ & ~last_state_mask_);
    last_state_mask_ = current_state_mask_;
}

bool GPIOImpl::read_input(uint8_t ch) const {
    return ((current_state_mask_ >> ch) & 0x1u) != 0;
}

uint32_t GPIOImpl::get_edge_mask() const {
    return edge_mask_;
}

} // namespace oc::platform
