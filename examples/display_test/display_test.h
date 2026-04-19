#pragma once
#include "oc/app.h"
#include "platform/drivers/weegfx.h"

/// DisplayTest — shows live encoder and button state on the OLED.
///
/// Line 0: "L enc: <value>  R enc: <value>"
/// Line 1: "UP: <0/1>  DOWN: <0/1>"
/// Line 2: "L click: <0/1>  R click: <0/1>"
///
/// Encoder values accumulate (no clamping) to show direction clearly.

class DisplayTest : public oc::Application {
public:
    void init() override {
        left_val_ = right_val_ = 0;
        up_held_ = down_held_ = false;
        left_click_ = right_click_ = false;
    }

    void audio_callback(const oc::AudioIn& in, oc::AudioOut& /*out*/) override {
        left_val_  += in.encoders[0].delta;
        right_val_ += in.encoders[1].delta;
        up_held_    = in.buttons[0].pressed;
        down_held_  = in.buttons[1].pressed;
        left_click_ = in.encoders[0].click_pressed;
        right_click_= in.encoders[1].click_pressed;
    }

    void idle() override { /* drawing happens in draw() called from main */ }

    void draw(oc::Display* display) override {
        if (!display->begin_frame()) return;

        gfx_.Begin(display->frame_buffer(), true);  // true = clear before drawing

        gfx_.setPrintPos(0, 0);
        gfx_.print("L:");
        gfx_.print(left_val_);
        gfx_.setPrintPos(64, 0);
        gfx_.print("R:");
        gfx_.print(right_val_);

        gfx_.setPrintPos(0, 10);
        gfx_.print("UP:");
        gfx_.print(up_held_ ? "1" : "0");
        gfx_.setPrintPos(64, 10);
        gfx_.print("DN:");
        gfx_.print(down_held_ ? "1" : "0");

        gfx_.setPrintPos(0, 20);
        gfx_.print("LC:");
        gfx_.print(left_click_ ? "1" : "0");
        gfx_.setPrintPos(64, 20);
        gfx_.print("RC:");
        gfx_.print(right_click_ ? "1" : "0");

        gfx_.End();
        display->end_frame();
    }

private:
    weegfx::Graphics gfx_;
    int32_t left_val_  = 0;
    int32_t right_val_ = 0;
    bool up_held_      = false;
    bool down_held_    = false;
    bool left_click_   = false;
    bool right_click_  = false;
};
