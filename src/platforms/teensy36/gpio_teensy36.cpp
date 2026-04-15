#include "gpio_teensy36.h"
#include <Arduino.h>   // digitalReadFast

namespace oc::platform::teensy36 {

GPIOImpl::GPIOImpl() {
    last_state_.fill(false);
    current_state_.fill(false);
}

void GPIOImpl::init() {
    for (int i = 0; i < 4; ++i) {
        pinMode(kPins[i], INPUT);
    }
}

void GPIOImpl::scan() {
    edge_mask_ = 0;
    for (int i = 0; i < 4; ++i) {
        // Gate inputs on O&C are active-low (inverted)
        const bool state = !digitalReadFast(kPins[i]);
        current_state_[i] = state;
        if (state && !last_state_[i]) {
            edge_mask_ |= (1u << i);  // rising edge detected
        }
        last_state_[i] = state;
    }
}

bool GPIOImpl::read_input(uint8_t ch) const {
    return current_state_[ch];
}

uint32_t GPIOImpl::get_edge_mask() const {
    return edge_mask_;
}

} // namespace oc::platform::teensy36
