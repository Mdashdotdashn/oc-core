#pragma once

#include <array>
#include <cstdint>

#include "pages/page_app.h"

namespace oc_test_pages {

class TriggerPage final : public PageApp {
public:
    TriggerPage() : PageApp("trigger") {}

    void init() override { reset(); }

    void reset() override {
        gates_ = {false, false, false, false};
    }

    void audio_callback(const oc::Inputs& in, oc::Outputs& /*out*/) override {
        gates_ = in.gate;
    }

    void draw_body(weegfx::Graphics& gfx) override {
        draw_trigger_value(gfx, "INA", gates_[0], 0, 14);
        draw_trigger_value(gfx, "INB", gates_[1], 64, 14);
        draw_trigger_value(gfx, "INC", gates_[2], 0, 28);
        draw_trigger_value(gfx, "IND", gates_[3], 64, 28);
    }

private:
    static void draw_trigger_value(weegfx::Graphics& gfx, const char* label, bool value, int16_t x, int16_t y) {
        gfx.setPrintPos(x, y);
        gfx.print(label);
        gfx.print(":");
        gfx.print(value ? "1" : "0");
    }

    std::array<bool, 4> gates_ = {false, false, false, false};
};

} // namespace oc_test_pages
