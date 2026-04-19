#pragma once

namespace oc {

/// Debounced state of a single button for one ISR cycle.
/// just_pressed / just_released fire for exactly one cycle on edge transitions.
struct ButtonState {
    bool pressed;       ///< True while held (all recent samples confirm pressed)
    bool just_pressed;  ///< True for exactly one cycle on press edge
    bool just_released; ///< True for exactly one cycle on release edge
};

} // namespace oc
