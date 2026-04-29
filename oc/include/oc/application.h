#pragma once

#include <array>
#include <cstdint>

#include "oc/button_state.h"
#include "oc/display_fwd.h"
#include "oc/encoder_state.h"
#include "oc/outputs.h"

namespace oc {

/// Base class for user algorithms.
///
/// Subclass this and implement audio_callback(). Optionally override init(),
/// idle(), and ui_callback(). Everything stays in your class — no global
/// state needed.
class Application {
public:
    virtual ~Application() = default;

    /// Non-UI input snapshot: CV, gate, and edges only.
    /// Passed to audio_callback every ~100us from ISR.
    struct Input {
        std::array<int32_t, 4> cv;        ///< Calibrated CV values
        std::array<uint32_t, 4> cv_raw;   ///< Raw 12-bit ADC values
        std::array<bool, 4> gate;         ///< Gate input levels
        uint32_t gate_edges;              ///< Rising-edge bitmask
    };

    /// Called once from main() before audio starts.
    /// Initialize oscillators, load presets, set defaults, etc.
    virtual void init() {}

    /// Called when an application or sub-application should reset its local state.
    /// Default implementation does nothing.
    virtual void reset() {}

    /// Called every ~100us from the hardware ISR (~10 kHz) with DSP inputs.
    ///
    /// Read inputs from in, write outputs to out.
    /// MUST be deterministic and complete well under 100us.
    /// No heap allocation, no blocking I/O, no long loops.
    virtual void audio_callback(const Input& in, Outputs& out) = 0;

    /// Called from the UI interrupt (~1 kHz) when button or encoder changes.
    ///
    /// Safe for non-blocking operations: UI updates, parameter changes, etc.
    /// Do not perform long computations here.
    virtual void ui_callback(const std::array<ButtonState, 2>& buttons,
                             const std::array<EncoderState, 2>& encoders) {}

    /// Called from the main() while(1) loop as fast as possible.
    ///
    /// Safe for: parameter updates, UI, display, file I/O, Serial.
    /// Not timing-critical — will be preempted by the ISR timer.
    virtual void idle() {}

    /// Optional OLED drawing hook, called from the background loop.
    /// Default implementation does nothing.
    virtual void draw(Display* /*display*/) {}

    /// Called when the system is shutting down cleanly.
    virtual void shutdown() {}
};

} // namespace oc
