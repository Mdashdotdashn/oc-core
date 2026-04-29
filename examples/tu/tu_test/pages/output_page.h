#pragma once

#include <array>
#include <cstdint>

#include "pages/page_app.h"

namespace tu_test_pages {

class OutputPage final : public PageApp {
public:
    OutputPage() : PageApp("outputs") {}

    void init() override { reset(); }

    void reset() override {
        pattern_phase_ = 0;
        pattern_tick_counter_ = 0;
        output_states_ = {true, false, false, false, false, false};
    }

    void audio_callback(const tu::Application::Input& /*in*/, tu::Outputs& out) override {
        if (++pattern_tick_counter_ >= kPatternStepTicks) {
            pattern_tick_counter_ = 0;
            pattern_phase_ = static_cast<uint8_t>((pattern_phase_ + 1) % 6);
        }

        for (uint8_t i = 0; i < 6; ++i) {
            const bool active = (i == pattern_phase_);
            output_states_[i] = active;
            out.gates[i] = active;
        }

        // Keep CLK4 in digital mode for this page.
        out.analog = 0;
    }

    void draw_body(weegfx::Graphics& gfx) override {
        draw_output(gfx, "TR1", output_states_[0], 0, 14);
        draw_output(gfx, "TR2", output_states_[1], 64, 14);
        draw_output(gfx, "TR3", output_states_[2], 0, 26);
        draw_output(gfx, "TR4", output_states_[3], 64, 26);
        draw_output(gfx, "TR5", output_states_[4], 0, 38);
        draw_output(gfx, "TR6", output_states_[5], 64, 38);
    }

private:
    static constexpr uint32_t kPatternStepTicks = 8000;

    static void draw_output(weegfx::Graphics& gfx, const char* label, bool value, int16_t x, int16_t y) {
        gfx.setPrintPos(x, y);
        gfx.print(label);
        gfx.print(":");
        gfx.print(value ? "1" : "0");
    }

    uint8_t pattern_phase_ = 0;
    uint32_t pattern_tick_counter_ = 0;
    std::array<bool, 6> output_states_ = {true, false, false, false, false, false};
};

} // namespace tu_test_pages
