#pragma once
#include <cstdint>

/// Ornament & Crime Hardware Abstraction Layer
/// Encoders Interface
///
/// Covers the 2 rotary encoders (LEFT and RIGHT) with clickable switches.
/// Encoder switches (clicks) are debounced with the same 8-bit shift-register
/// as buttons. Rotation uses a 2-bit state machine matching ArticCircle's
/// UI/ui_encoder.h algorithm.
///
/// scan() must be called at a fixed rate (the ISR or a UI timer).
/// Read the resulting delta and click state via get().

namespace oc::hal {

/// State of one encoder for the current scan cycle.
struct EncoderEvent {
    int8_t  delta;         ///< Rotation since last scan: +1 CW, -1 CCW, 0 none
    bool    click_pressed;       ///< True while switch held
    bool    click_just_pressed;  ///< True for one cycle on press edge
    bool    click_just_released; ///< True for one cycle on release edge
};

class EncodersInterface {
public:
    virtual ~EncodersInterface() = default;

    /// Read all encoder pins and advance state machines.
    /// Call once per ISR cycle (or UI tick at the same rate as buttons).
    virtual void scan() = 0;

    /// Return event state for encoder at index.
    /// 0 = LEFT encoder, 1 = RIGHT encoder.
    virtual EncoderEvent get(uint8_t idx) const = 0;

    static constexpr int kCount = 2;
};

} // namespace oc::hal
