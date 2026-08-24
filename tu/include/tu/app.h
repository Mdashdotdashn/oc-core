#pragma once

#include <array>
#include <cstdint>

#include "tu/display_fwd.h"
#include "tu/inputs.h"
#include "tu/outputs.h"

namespace tu {

/// Base class for a Temps Utile application.
///
/// The Runtime calls these methods at the appropriate points in each cycle:
///   audio_callback() — called from the ISR (~10 kHz) with DSP inputs only.
///   ui_callback()    — called from the UI interrupt (~1 kHz) when UI changes.
///   idle()           — called from the main loop, safe for Serial / UI.
///   draw()           — called from the main loop; renders into the OLED
///                      framebuffer. The ISR DMA-pages it to screen.

class Application {
public:
    virtual ~Application() = default;

    /// Non-UI input snapshot: CV, gate, and edges only.
    /// Passed to audio_callback every ~100µs from ISR.
    struct Input {
        std::array<float,    4> cv;        ///< Calibrated CV values in volts
        std::array<int32_t,  4> cv_mv;     ///< Calibrated CV values in millivolts
        std::array<uint32_t, 4> cv_raw;    ///< Raw 12-bit ADC values
        std::array<bool,     2> gate;      ///< Gate input levels (TR1, TR2)
        uint32_t                gate_edges; ///< Rising-edge bitmask
    };

    /// Called once from main() before audio starts.
    virtual void init() {}

    /// Called when an application or sub-application should reset its local state.
    virtual void reset() {}

    /// Called every ~100µs from the hardware ISR (~10 kHz) with DSP inputs.
    ///
    /// Read inputs from in, write outputs to out.
    /// MUST be deterministic and complete well under budget.
    /// No heap allocation, no blocking I/O, no long loops.
    virtual void audio_callback(const Input& in, Outputs& out) = 0;

    /// Called from the UI interrupt (~1 kHz) when button or encoder changes.
    ///
    /// Safe for non-blocking operations: UI updates, parameter changes, etc.
    /// Do not perform long computations here.
    virtual void ui_callback(const std::array<ButtonState, 2>& buttons,
                             const std::array<EncoderState, 2>& encoders) {}

    /// Called from the main() while(1) loop as fast as possible.
    virtual void idle() {}

    /// Optional OLED drawing hook, called from the background loop.
    virtual void draw(Display* /*display*/) {}

    /// Called when the system is shutting down cleanly.
    virtual void shutdown() {}
};

} // namespace tu
