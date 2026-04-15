#pragma once
#include "oc/app.h"

/// EncoderTest — exercises both rotary encoders and their click switches.
///
/// LEFT  encoder: adjusts output A voltage in 0.5V steps (-3V to +3V).
///                Click resets A to 0V.
/// RIGHT encoder: adjusts output B voltage in 0.5V steps (-3V to +3V).
///                Click resets B to 0V.
/// Outputs C, D: fixed 0V (reference).

class EncoderTest : public oc::Application {
public:
    void init() override {
        volt_a_ = 0.0f;
        volt_b_ = 0.0f;
        update_outputs();
    }

    void audio_callback(const oc::AudioIn& in, oc::AudioOut& out) override {
        const auto& left  = in.encoders[0];
        const auto& right = in.encoders[1];

        if (left.delta != 0) {
            volt_a_ = clamp(volt_a_ + left.delta * kStep);
            update_outputs();
        }
        if (left.click_just_pressed) {
            volt_a_ = 0.0f;
            update_outputs();
        }

        if (right.delta != 0) {
            volt_b_ = clamp(volt_b_ + right.delta * kStep);
            update_outputs();
        }
        if (right.click_just_pressed) {
            volt_b_ = 0.0f;
            update_outputs();
        }

        out = out_;
    }

    void main_loop() override {}

private:
    static constexpr float kStep = 0.5f;
    static constexpr float kMin  = -3.0f;
    static constexpr float kMax  =  3.0f;
    static constexpr float kZero = 24576.0f;
    static constexpr float kCpV  = 6553.5f;

    float volt_a_ = 0.0f;
    float volt_b_ = 0.0f;
    oc::AudioOut out_;

    static float clamp(float v) {
        if (v < kMin) return kMin;
        if (v > kMax) return kMax;
        return v;
    }

    void update_outputs() {
        out_.set_cv(0, volt_a_, kZero, kCpV);
        out_.set_cv(1, volt_b_, kZero, kCpV);
        out_.set_cv(2, 0.0f,   kZero, kCpV);
        out_.set_cv(3, 0.0f,   kZero, kCpV);
    }
};
