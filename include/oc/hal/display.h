#pragma once
#include <stdint.h>

/// Ornament & Crime Hardware Abstraction Layer
/// Display Interface
///
/// The SH1106 128×64 OLED shares SPI0 with the DAC (DAC8565).
/// Transfers use DMA to overlap with DAC updates in the ISR.
///
/// Usage pattern:
///   ISR:       hw.display()->flush();   // finish previous DMA page
///              hw.dac()->flush();       // DAC SPI (same bus, interleaved)
///              hw.display()->update();  // start next DMA page
///
///   main_loop: if (hw.display()->begin_frame()) {
///                  // draw using weegfx::Graphics into the framebuffer
///                  hw.display()->end_frame();
///              }

namespace oc::hal {

class DisplayInterface {
public:
    virtual ~DisplayInterface() = default;

    /// Called at ISR entry: finalize previous DMA page transfer.
    virtual void flush() = 0;

    /// Called at ISR exit: start DMA transfer for next page (if frame ready).
    virtual void update() = 0;

    /// Try to acquire a writable framebuffer. Returns true if one is available.
    /// Call from main_loop() only. If true, draw, then call end_frame().
    virtual bool begin_frame() = 0;

    /// Mark the current frame as written and ready for DMA transfer.
    virtual void end_frame() = 0;

    /// Direct access to a writeable framebuffer (128×64 / 8 = 1024 bytes).
    /// Only valid between begin_frame() and end_frame().
    virtual uint8_t* frame_buffer() = 0;
};

} // namespace oc::hal
