#pragma once

#include "tu/app.h"
#include "platform/drivers/weegfx.h"

/// Minimal Temps Utile demo: gates and CV pass-through.
///   TR1 → CLK1, TR2 → CLK2
///   Button 0 toggles CLK3
///   Button 1 toggles CLK5
///   Encoder 0 delta → CLK6 (pulsed one tick on each step)
///   CLK4 outputs CV1 raw value as 12-bit DAC voltage

template <typename RuntimeT>
class ClockTest : public tu::Application {
public:
    explicit ClockTest(RuntimeT& rt) : runtime_(rt) {}

    void init() override {}

    void audio_callback(const tu::Inputs& in, tu::Outputs& out) override {
        // Gate pass-through
        out.gates[0] = in.gate[0];  // TR1 → CLK1
        out.gates[1] = in.gate[1];  // TR2 → CLK2

        // Toggle on button press
        if (in.buttons[0].just_pressed) toggle0_ = !toggle0_;
        out.gates[2] = toggle0_;    // CLK3

        if (in.buttons[1].just_pressed) toggle1_ = !toggle1_;
        out.gates[4] = toggle1_;    // CLK5

        // One-shot on encoder step
        out.gates[5] = (in.encoders[0].delta != 0);  // CLK6

        // CLK4: route CV1 raw (0–4095) straight to the internal DAC
        out.analog = static_cast<uint16_t>(in.cv_raw[0] & 0x0FFF);
    }

    void idle() override {}

    void draw(tu::Display* display) override {
        if (!display->begin_frame()) return;

        gfx_.Begin(display->frame_buffer(), true);
        gfx_.setPrintPos(0, 0);
        gfx_.print("T_U clock test");

        gfx_.setPrintPos(0, 12);
        gfx_.print("ISR:");
        gfx_.print(static_cast<int>(runtime_.isr_average_us()));
        gfx_.print("us ");
        gfx_.print(static_cast<int>(runtime_.isr_load_percent()));
        gfx_.print("%");

        gfx_.End();
        display->end_frame();
    }

private:
    RuntimeT& runtime_;
    weegfx::Graphics gfx_;
    bool toggle0_ = false;
    bool toggle1_ = false;
};
