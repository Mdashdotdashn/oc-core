#pragma once
#include <cstdint>

/// Ornament & Crime Hardware Abstraction Layer
/// Buttons Interface
///
/// Covers the 2 physical push-buttons (UP/DOWN) on the front panel.
/// Encoder clicks are handled separately in the encoder HAL.
///
/// Debounce uses an 8-bit shift register (same algorithm as ArticCircle
/// UI/ui_button.h). At 10 kHz polling the debounce window is ~0.7 ms.

namespace oc::hal {

/// State of a single button for the current ISR cycle.
struct ButtonEvent {
    bool pressed;       ///< True while held (all 8 recent samples pressed)
    bool just_pressed;  ///< True exactly once, on the 7th consecutive press sample
    bool just_released; ///< True exactly once, on the 7th consecutive release sample
};

class ButtonsInterface {
public:
    virtual ~ButtonsInterface() = default;

    /// Read all button pins and advance shift-register state.
    /// Call once per ISR cycle (or UI tick).
    virtual void scan() = 0;

    /// Return the debounced event state for button at index.
    /// 0 = UP (top button, pin 5), 1 = DOWN (bottom button, pin 4).
    virtual ButtonEvent get(uint8_t idx) const = 0;

    static constexpr int kCount = 2;
};

} // namespace oc::hal
