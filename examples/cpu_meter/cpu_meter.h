#pragma once

#include "oc/app.h"
#include "oc/hal/display.h"
#include "drivers/weegfx.h"

template <typename RuntimeT>
class CpuMeter : public oc::Application {
public:
    explicit CpuMeter(RuntimeT& runtime) : runtime_(runtime) {}

    void init() override {}

    void audio_callback(const oc::AudioIn& /*in*/, oc::AudioOut& out) override {
        out.cv[0] = out.cv[1] = out.cv[2] = out.cv[3] = 0;
    }

    void draw(oc::hal::DisplayInterface* display) override {
        if (!display->begin_frame()) {
            return;
        }

        const uint8_t load = runtime_.isr_load_percent();
        const uint32_t avg_us = runtime_.isr_average_us();
        const int bar_w = (load * 120) / 100;

        gfx_.Begin(display->frame_buffer(), true);

        gfx_.setPrintPos(0, 0);
        gfx_.print("CPU meter");

        gfx_.setPrintPos(0, 14);
        gfx_.print("ISR us:");
        gfx_.print(static_cast<int>(avg_us));

        gfx_.setPrintPos(0, 26);
        gfx_.print("Load:");
        gfx_.print(static_cast<int>(load));
        gfx_.print("%");

        gfx_.drawFrame(4, 44, 120, 12);
        if (bar_w > 0) {
            gfx_.drawRect(4, 44, bar_w, 12);
        }

        gfx_.setPrintPos(0, 58);
        gfx_.print("0");
        gfx_.setPrintPos(110, 58);
        gfx_.print("100");

        gfx_.End();
        display->end_frame();
    }

private:
    RuntimeT& runtime_;
    weegfx::Graphics gfx_;
};