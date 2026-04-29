#pragma once

#include <cstdint>

#include "pages/page_app.h"

namespace tu_test_pages {

class EncoderPage final : public PageApp {
public:
    EncoderPage() : PageApp("encoders") {}

    void init() override { reset(); }

    void reset() override {
        left_val_ = 0;
        right_val_ = 0;
        left_click_ = false;
        right_click_ = false;
    }

    void audio_callback(const tu::Application::Input& /*in*/, tu::Outputs& /*out*/) override {}

    void ui_callback(const std::array<tu::ButtonState, 2>& /*buttons*/,
                     const std::array<tu::EncoderState, 2>& encoders) override {
        left_val_ += encoders[0].delta;
        right_val_ += encoders[1].delta;
        left_click_ = encoders[0].click_pressed;
        right_click_ = encoders[1].click_pressed;
    }

    void draw_body(weegfx::Graphics& gfx) override {
        gfx.setPrintPos(0, 14);
        gfx.print("L:");
        gfx.print(left_val_);

        gfx.setPrintPos(64, 14);
        gfx.print("R:");
        gfx.print(right_val_);

        gfx.setPrintPos(0, 28);
        gfx.print("LC:");
        gfx.print(left_click_ ? "1" : "0");

        gfx.setPrintPos(64, 28);
        gfx.print("RC:");
        gfx.print(right_click_ ? "1" : "0");
    }

private:
    int32_t left_val_ = 0;
    int32_t right_val_ = 0;
    bool left_click_ = false;
    bool right_click_ = false;
};

} // namespace tu_test_pages
