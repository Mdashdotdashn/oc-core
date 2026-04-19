#pragma once
#include "tu/inputs.h"
#include "tu/outputs.h"
#include "tu/display_fwd.h"

namespace tu {

/// Base class for a Temps Utile application.
///
/// The Runtime calls these methods at the appropriate points in each cycle:
///   audio_callback() — called from the ISR at ~16.6 kHz (60µs). Must be
///                      deterministic, no allocation, no blocking.
///   idle()           — called from the main loop, safe for Serial / UI.
///   draw()           — called from the main loop; renders into the OLED
///                      framebuffer. The ISR DMA-pages it to screen.

class Application {
public:
    virtual ~Application() = default;

    virtual void init() {}
    virtual void audio_callback(const Inputs& /*in*/, Outputs& /*out*/) {}
    virtual void idle() {}
    virtual void draw(Display* /*display*/) {}
};

} // namespace tu
