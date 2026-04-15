#pragma once
#include "oc/app.h"

/// ButtonTest — exercises the two front-panel push-buttons.
///
/// Output A: toggles between -2V and +2V on each UP press.
/// Output B: steps through 0V → 1V → 2V → 3V → 0V... on each DOWN press.
/// Outputs C, D: +0V (reference / scope ground).
///
/// Zero-code trimmed empirically: theoretical 19661, +4915 correction
/// for this board's -0.75V hardware offset.

class ButtonTest : public oc::Application {
public:
    void init() override {
        toggle_state_ = false;
        step_          = 0;
        update_outputs();
    }

    void audio_callback(const oc::AudioIn& in, oc::AudioOut& out) override {
        if (in.buttons[0].just_pressed) {   // UP
            toggle_state_ = !toggle_state_;
            update_outputs();
        }
        if (in.buttons[1].just_pressed) {   // DOWN
            step_ = (step_ + 1) % 4;
            update_outputs();
        }
        out = out_;
    }

    void main_loop() override {}

private:
    static constexpr float kZero = 24576.0f;
    static constexpr float kCpV  = 6553.5f;

    bool    toggle_state_;
    uint8_t step_;
    oc::AudioOut out_;

    void update_outputs() {
        out_.set_cv(0, toggle_state_ ? 2.0f : -2.0f, kZero, kCpV);  // A: toggle
        const float step_volts[4] = {0.0f, 1.0f, 2.0f, 3.0f};
        out_.set_cv(1, step_volts[step_], kZero, kCpV);               // B: step
        out_.set_cv(2, 0.0f, kZero, kCpV);                            // C: ref
        out_.set_cv(3, 0.0f, kZero, kCpV);                            // D: ref
    }
};
