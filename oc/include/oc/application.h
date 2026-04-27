#pragma once

#include "oc/display_fwd.h"
#include "oc/inputs.h"
#include "oc/outputs.h"

namespace oc {

/// Base class for user algorithms.
///
/// Subclass this and implement audio_callback(). Optionally override init()
/// and idle(). Everything stays in your class — no global state needed.
class Application {
public:
    virtual ~Application() = default;

    /// Called once from main() before audio starts.
    /// Initialize oscillators, load presets, set defaults, etc.
    virtual void init() {}

    /// Called when an application or sub-application should reset its local state.
    /// Default implementation does nothing.
    virtual void reset() {}

    /// Called every 100us from the hardware ISR (~10 kHz).
    ///
    /// Read inputs from in, write outputs to out.
    /// MUST be deterministic and complete well under 100us.
    /// No heap allocation, no blocking I/O, no long loops.
    virtual void audio_callback(const Inputs& in, Outputs& out) = 0;

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
