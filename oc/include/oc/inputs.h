#pragma once

#include <array>
#include <cstdint>

#include "oc/button_state.h"
#include "oc/encoder_state.h"

namespace oc {

/// Hardware input snapshot for one audio ISR cycle.
struct Inputs {
    /// Calibrated CV values from ADC (signed, scaled to pitch CV units).
    /// Use these for pitch/frequency calculations.
    std::array<int32_t, 4> cv;

    /// Raw 12-bit ADC readings (0–4095) before calibration.
    /// Use these if you need the raw voltage ratio.
    std::array<uint32_t, 4> cv_raw;

    /// Current logic level of each gate/trigger input (true = high).
    std::array<bool, 4> gate;

    /// Bitmask of gate channels that had a rising edge this cycle.
    /// Bit N is set when gate[N] just went high.
    /// Example: if (in.gate_edges & (1 << 2)) { /* gate 3 triggered */ }
    uint32_t gate_edges;

    /// Front-panel push-buttons. [0] = UP (pin 5), [1] = DOWN (pin 4).
    /// Poll from audio_callback(); cache values in member vars if needed in idle().
    std::array<ButtonState, 2> buttons;

    /// Rotary encoders. [0] = LEFT, [1] = RIGHT.
    /// delta: +1 CW, -1 CCW, 0 no movement this cycle.
    std::array<EncoderState, 2> encoders;
};

} // namespace oc
