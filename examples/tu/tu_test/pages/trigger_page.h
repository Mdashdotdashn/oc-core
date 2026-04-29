#pragma once

#include <array>
#include <cstdint>

#include "pages/page_app.h"

namespace tu_test_pages {

class TriggerPage final : public PageApp {
public:
    TriggerPage() : PageApp("trigger") {}

    void init() override { reset(); }

    void reset() override {
        gates_ = {false, false};
        edge_count_ = {0, 0};
    }

    void audio_callback(const tu::Application::Input& in, tu::Outputs& /*out*/) override {
        gates_ = in.gate;

        if (in.gate_edges & 0x1u) ++edge_count_[0];
        if (in.gate_edges & 0x2u) ++edge_count_[1];
    }

    void draw_body(weegfx::Graphics& gfx) override {
        draw_trigger_value(gfx, "TR1", gates_[0], edge_count_[0], 0, 14);
        draw_trigger_value(gfx, "TR2", gates_[1], edge_count_[1], 64, 14);
    }

private:
    static void draw_trigger_value(weegfx::Graphics& gfx,
                                   const char* label,
                                   bool value,
                                   uint32_t edge_count,
                                   int16_t x,
                                   int16_t y) {
        gfx.setPrintPos(x, y);
        gfx.print(label);
        gfx.print(":");
        gfx.print(value ? "1" : "0");

        gfx.setPrintPos(x, y + 12);
        gfx.print("edge:");
        gfx.print(static_cast<int>(edge_count));
    }

    std::array<bool, 2> gates_ = {false, false};
    std::array<uint32_t, 2> edge_count_ = {0, 0};
};

} // namespace tu_test_pages
