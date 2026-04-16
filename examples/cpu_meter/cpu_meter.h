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

        const auto profile = runtime_.isr_profile();
        const uint32_t known_cycles = profile.display_cycles + profile.dac_flush_cycles + profile.scan_cycles
            + profile.marshal_cycles + profile.app_cycles + profile.output_cycles;
        const uint32_t other_cycles = profile.total_cycles > known_cycles
            ? profile.total_cycles - known_cycles
            : 0;

        gfx_.Begin(display->frame_buffer(), true);

        gfx_.setPrintPos(0, 0);
        gfx_.print("CPU buckets");

        print_bucket(10, "TOT", profile.total_cycles);
        print_bucket(18, "DSP", profile.display_cycles);
        print_bucket(26, "DAC", profile.dac_flush_cycles);
        print_bucket(34, "SCN", profile.scan_cycles);
        print_bucket(42, "MRS", profile.marshal_cycles);
        print_bucket(50, "APP", profile.app_cycles);
        print_bucket(58, "OTH", other_cycles + profile.output_cycles);

        gfx_.End();
        display->end_frame();
    }

private:
    void print_bucket(int y, const char* label, uint32_t cycles) {
        gfx_.setPrintPos(0, y);
        gfx_.print(label);
        gfx_.print(" ");
        gfx_.print(static_cast<int>(runtime_.cycles_to_load_percent(cycles)));
        gfx_.print("% ");
        gfx_.print(static_cast<int>(runtime_.cycles_to_us(cycles)));
        gfx_.print("us");
    }

    RuntimeT& runtime_;
    weegfx::Graphics gfx_;
};