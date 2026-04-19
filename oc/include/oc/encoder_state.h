#pragma once

#include <cstdint>

namespace oc {

/// Rotary encoder state captured for one ISR cycle.
struct EncoderState {
    int8_t delta;
    bool   click_pressed;
    bool   click_just_pressed;
    bool   click_just_released;
};

} // namespace oc
