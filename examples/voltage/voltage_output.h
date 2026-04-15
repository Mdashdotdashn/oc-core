#pragma once
#include "oc/app.h"

/// VoltageOutput — outputs four fixed DC voltages on the four CV jacks.
///
/// kZeroVoltCode is trimmed empirically for this board (+4915 from the
/// theoretical 19661) because measuring 0V→-0.75V offset on uncalibrated
/// hardware. Run the O&C calibration procedure to get the exact value.
///
/// CV Outputs (jack order confirmed A=1, B=2, C=3, D=4):
///   A (jack 1)  →  −2 V
///   B (jack 2)  →   0 V
///   C (jack 3)  →  +1.5 V
///   D (jack 4)  →  +3 V

class VoltageOutput : public oc::Application {
public:
    void init() override {
        constexpr float kZero = 24576.0f;
        constexpr float kCpV  = 6553.5f;
        out_.set_cv(0, -2.0f, kZero, kCpV);   // A → -2V
        out_.set_cv(1,  0.0f, kZero, kCpV);   // B →  0V
        out_.set_cv(2,  1.5f, kZero, kCpV);   // C → +1.5V
        out_.set_cv(3,  3.0f, kZero, kCpV);   // D → +3V
    }

    void audio_callback(const oc::AudioIn& /*in*/, oc::AudioOut& out) override {
        out = out_;
    }

    void main_loop() override {}

private:
    oc::AudioOut out_;
};
