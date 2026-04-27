#pragma once

#include <array>
#include <cstdint>

#include "oc/calibration.h"
#include "pages/page_app.h"
#include "pages/page_ui_utils.h"

namespace oc_test_pages {

class OutputPage final : public PageApp {
public:
    OutputPage() : PageApp("output") {}

    void init() override { reset(); }

    void reset() override {
        pattern_phase_ = 0;
        pattern_tick_counter_ = 0;
        output_volts_ = {-3, 0, 1, 2};
    }

    void audio_callback(const oc::Inputs& /*in*/, oc::Outputs& out) override {
        if (++pattern_tick_counter_ >= kPatternStepTicks) {
            pattern_tick_counter_ = 0;
            pattern_phase_ = static_cast<uint8_t>((pattern_phase_ + 1) & 0x3);
        }

        for (uint8_t i = 0; i < 4; ++i) {
            const uint8_t pattern_index = static_cast<uint8_t>((i + pattern_phase_) & 0x3);
            const int8_t volts = kPatternVolts[pattern_index];
            output_volts_[i] = volts;
            out.cv[i] = oc::calibration::volts_to_dac(i, static_cast<float>(volts));
        }
    }

    void draw_body(weegfx::Graphics& gfx) override {
        gfx.setPrintPos(0, 14);
        gfx.print("OUTA:");
        print_pattern_voltage(gfx, output_volts_[0]);

        gfx.setPrintPos(64, 14);
        gfx.print("OUTB:");
        print_pattern_voltage(gfx, output_volts_[1]);

        gfx.setPrintPos(0, 28);
        gfx.print("OUTC:");
        print_pattern_voltage(gfx, output_volts_[2]);

        gfx.setPrintPos(64, 28);
        gfx.print("OUTD:");
        print_pattern_voltage(gfx, output_volts_[3]);

        gfx.setPrintPos(0, 42);
        gfx.print("step:");
        gfx.print(static_cast<int>(pattern_phase_ + 1));
        gfx.print("/4");
    }

private:
    static constexpr uint32_t kPatternStepTicks = 20000;
    static constexpr std::array<int8_t, 4> kPatternVolts = {-3, 0, 1, 2};

    uint8_t pattern_phase_ = 0;
    uint32_t pattern_tick_counter_ = 0;
    std::array<int8_t, 4> output_volts_ = {-3, 0, 1, 2};
};

} // namespace oc_test_pages
