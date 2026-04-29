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

    void ui_callback(const std::array<tu::ButtonState, 2>& buttons,
                     const std::array<tu::EncoderState, 2>& encoders) override {
        if (buttons[0].just_pressed) toggle0_ = !toggle0_;
        if (buttons[1].just_pressed) toggle1_ = !toggle1_;
        encoder_delta_ = (encoders[0].delta != 0);
    }

    void audio_callback(const tu::Application::Input& in, tu::Outputs& out) override {
        // Gate pass-through
        out.gates[0] = in.gate[0];  // TR1 → CLK1
        out.gates[1] = in.gate[1];  // TR2 → CLK2

        // Toggle from ui_callback
        out.gates[2] = toggle0_;    // CLK3
        out.gates[4] = toggle1_;    // CLK5

        // One-shot from ui_callback
        out.gates[5] = encoder_delta_;  // CLK6

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
    bool encoder_delta_ = false;
};
