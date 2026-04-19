#pragma once
#include <cstdint>
#include <array>

/// Input state passed to Application::audio_callback() each ISR cycle.
/// Temps Utile variant: 2 gate inputs, 4 CV inputs, 2 buttons, 2 encoders.

namespace tu {

struct ButtonState {
    bool pressed;
    bool just_pressed;
    bool just_released;
};

struct EncoderState {
    int8_t delta;
    bool   click_pressed;
    bool   click_just_pressed;
    bool   click_just_released;
};

struct Inputs {
    std::array<int32_t,  4> cv;         ///< Calibrated CV values (offset-corrected raw ADC)
    std::array<uint32_t, 4> cv_raw;     ///< Smoothed raw 12-bit ADC values
    std::array<bool,     2> gate;       ///< Gate input logic levels (TR1, TR2)
    uint32_t                gate_edges; ///< Rising-edge bitmask, cleared each cycle
    std::array<ButtonState,  2> buttons;
    std::array<EncoderState, 2> encoders;
};

} // namespace tu
