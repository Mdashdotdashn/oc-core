#pragma once
#include "oc/hal/display.h"
#include "drivers/SH1106_128x64_driver.h"
#include "drivers/framebuffer.h"
#include "drivers/page_display_driver.h"

/// Teensy 3.2 SH1106 OLED display implementation.
///
/// Double-buffered 1024-byte framebuffer. ISR calls flush()+update() once per
/// cycle to DMA one page (128 bytes) to the display. A full 128×64 screen
/// refresh therefore takes 8 ISR cycles (0.8 ms at 10 kHz).

namespace oc::platform::teensy32 {

class DisplayImpl final : public hal::DisplayInterface {
public:
    void init() {
        frame_buf_.Init();
        driver_.Init();
    }

    /// ISR: finish previous DMA page transfer.
    void flush() override {
        if (driver_.Flush())
            frame_buf_.read();
    }

    /// ISR: start next DMA page transfer if a rendered frame is available.
    void update() override {
        if (driver_.frame_valid()) {
            driver_.Update();
        } else if (frame_buf_.readable()) {
            driver_.Begin(frame_buf_.readable_frame());
        }
    }

    /// idle: acquire a writable framebuffer.
    bool begin_frame() override {
        current_frame_ = frame_buf_.writeable() ? frame_buf_.writeable_frame() : nullptr;
        return current_frame_ != nullptr;
    }

    /// idle: submit the rendered frame for DMA transfer.
    void end_frame() override {
        if (current_frame_) {
            frame_buf_.written();
            current_frame_ = nullptr;
        }
    }

    uint8_t* frame_buffer() override { return current_frame_; }

    void set_offset(uint8_t offset) {
        SH1106_128x64_Driver::AdjustOffset(offset);
    }

private:
    using FrameBuf = FrameBuffer<SH1106_128x64_Driver::kFrameSize, 2>;
    using Driver   = PagedDisplayDriver<SH1106_128x64_Driver>;

    FrameBuf  frame_buf_;
    Driver    driver_;
    uint8_t*  current_frame_ = nullptr;
};

} // namespace oc::platform::teensy32
