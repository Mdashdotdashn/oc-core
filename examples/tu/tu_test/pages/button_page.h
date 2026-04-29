#pragma once

#include <cstdint>

#include "pages/page_app.h"

namespace tu_test_pages {

class ButtonPage final : public PageApp {
public:
    ButtonPage() : PageApp("buttons") {}

    void init() override { reset(); }

    void reset() override {
        b0_pressed_ = false;
        b1_pressed_ = false;
        b0_just_pressed_hold_ = 0;
        b1_just_pressed_hold_ = 0;
        b0_just_released_hold_ = 0;
        b1_just_released_hold_ = 0;
    }

    void audio_callback(const tu::Application::Input& /*in*/, tu::Outputs& /*out*/) override {}

    void ui_callback(const std::array<tu::ButtonState, 2>& buttons,
                     const std::array<tu::EncoderState, 2>& /*encoders*/) override {
        b0_pressed_ = buttons[0].pressed;
        b1_pressed_ = buttons[1].pressed;

        if (b0_just_pressed_hold_ > 0) --b0_just_pressed_hold_;
        if (b1_just_pressed_hold_ > 0) --b1_just_pressed_hold_;
        if (b0_just_released_hold_ > 0) --b0_just_released_hold_;
        if (b1_just_released_hold_ > 0) --b1_just_released_hold_;

        if (buttons[0].just_pressed) b0_just_pressed_hold_ = kEventHoldTicks;
        if (buttons[1].just_pressed) b1_just_pressed_hold_ = kEventHoldTicks;
        if (buttons[0].just_released) b0_just_released_hold_ = kEventHoldTicks;
        if (buttons[1].just_released) b1_just_released_hold_ = kEventHoldTicks;
    }

    void draw_body(weegfx::Graphics& gfx) override {
        gfx.setPrintPos(0, 14);
        gfx.print("B1 p:");
        gfx.print(b0_pressed_ ? "1" : "0");
        gfx.print(" jp:");
        gfx.print(b0_just_pressed_hold_ ? "1" : "0");
        gfx.print(" jr:");
        gfx.print(b0_just_released_hold_ ? "1" : "0");

        gfx.setPrintPos(0, 28);
        gfx.print("B2 p:");
        gfx.print(b1_pressed_ ? "1" : "0");
        gfx.print(" jp:");
        gfx.print(b1_just_pressed_hold_ ? "1" : "0");
        gfx.print(" jr:");
        gfx.print(b1_just_released_hold_ ? "1" : "0");
    }

private:
    static constexpr uint8_t kEventHoldTicks = 25;

    bool b0_pressed_ = false;
    bool b1_pressed_ = false;
    uint8_t b0_just_pressed_hold_ = 0;
    uint8_t b1_just_pressed_hold_ = 0;
    uint8_t b0_just_released_hold_ = 0;
    uint8_t b1_just_released_hold_ = 0;
};

} // namespace tu_test_pages
