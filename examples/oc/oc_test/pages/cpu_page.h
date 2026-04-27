#pragma once

#include <cstdint>

#include "pages/page_app.h"

namespace oc_test_pages {

template <typename RuntimeT>
class CpuPage final : public PageApp {
public:
    explicit CpuPage(RuntimeT& runtime)
        : PageApp("cpu"), runtime_(runtime) {}

    void audio_callback(const oc::Inputs& /*in*/, oc::Outputs& /*out*/) override {}

    void draw_body(weegfx::Graphics& gfx) override {
        const auto profile = runtime_.isr_profile();
        draw_load_bar(gfx, profile);
    }

private:
    void draw_load_bar(weegfx::Graphics& gfx, const typename RuntimeT::IsrProfile& profile) {
        const uint32_t total = profile.total_cycles;
        if (!total) {
            return;
        }

        const uint32_t app = profile.app_cycles;
        uint32_t largest_other = 0;
        const char* largest_label = "???";

        if (profile.display_cycles > largest_other) {
            largest_other = profile.display_cycles;
            largest_label = "DSP";
        }
        if (profile.dac_flush_cycles > largest_other) {
            largest_other = profile.dac_flush_cycles;
            largest_label = "DAC";
        }
        if (profile.scan_cycles > largest_other) {
            largest_other = profile.scan_cycles;
            largest_label = "SCN";
        }
        if (profile.marshal_cycles > largest_other) {
            largest_other = profile.marshal_cycles;
            largest_label = "MRS";
        }
        if (profile.output_cycles > largest_other) {
            largest_other = profile.output_cycles;
            largest_label = "OUT";
        }

        const int16_t bar_y = 16;
        const int16_t bar_height = 12;
        const int16_t bar_width = 120;

        const int16_t app_width = static_cast<int16_t>((static_cast<uint64_t>(app) * bar_width) / total);
        const int16_t other_width = static_cast<int16_t>((static_cast<uint64_t>(largest_other) * bar_width) / total);

        gfx.drawFrame(0, bar_y, bar_width, bar_height);
        for (int16_t x = 0; x < app_width; ++x) {
            gfx.drawVLine(x, bar_y, bar_height);
        }
        if (other_width > 0 && app_width + other_width <= bar_width) {
            gfx.invertRect(app_width, bar_y, other_width, bar_height);
        }

        gfx.setPrintPos(0, 36);
        gfx.print("TOT:");
        gfx.print(static_cast<int>(runtime_.cycles_to_load_percent(total)));
        gfx.print("%");

        gfx.setPrintPos(0, 46);
        gfx.print("APP:");
        gfx.print(static_cast<int>(runtime_.cycles_to_us(app)));
        gfx.print("u ");
        gfx.print(largest_label);
        gfx.print(":");
        gfx.print(static_cast<int>(runtime_.cycles_to_us(largest_other)));
        gfx.print("u");
    }

    RuntimeT& runtime_;
};

} // namespace oc_test_pages
