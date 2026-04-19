#pragma once
#include <cstdint>
#include <array>

/// Output state written by Application::audio_callback() each ISR cycle.
/// Temps Utile variant: 6 gate/trigger outputs; CLK4 also supports 12-bit DAC.

namespace tu {

struct Outputs {
    /// CLK1–CLK6 gate states. true = HIGH.
    /// For CLK4 (index 3), also see analog below.
    std::array<bool, 6> gates = {};

    /// 12-bit analog value for CLK4 (0–4095).
    /// When non-zero, overrides gates[3] for CLK4.
    uint16_t analog = 0;
};

} // namespace tu
