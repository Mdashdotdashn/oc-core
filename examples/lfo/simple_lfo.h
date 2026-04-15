#pragma once
#include "oc/app.h"

/// SimpleLFO — example O&C algorithm using oc-core framework.
///
/// CV Inputs:
///   cv[0]  — frequency modulation (higher CV = faster LFO)
/// Gate Inputs:
///   gate[0] — rising edge resets phase to 0 (hard sync)
///
/// CV Outputs:
///   cv[0..3] — triangle wave (all 4 outputs in phase)

class SimpleLFO : public oc::Application {
public:
    void init() override {
        phase_ = 0;
        // Base increment: tune to taste.
        // At 10kHz ISR rate, 100 (~0.001 of full cycle per tick) ≈ 10Hz LFO.
        base_increment_ = 100;
    }

    void audio_callback(const oc::AudioIn& in, oc::AudioOut& out) override {
        // Hard sync: reset phase on gate 0 rising edge
        if (in.gate_edges & 0x1) {
            phase_ = 0;
        }

        // Frequency modulation from cv[0]
        // A positive CV value speeds the LFO up.
        const int32_t inc = base_increment_ + (in.cv[0] >> 8);
        const uint32_t safe_inc = (inc < 1) ? 1 : static_cast<uint32_t>(inc);

        // Phase accumulation (wraps naturally at 32-bit overflow)
        phase_ += safe_inc;

        // Triangle wave: rising half then falling half
        uint16_t output;
        if ((phase_ >> 31) == 0) {
            output = static_cast<uint16_t>(phase_ >> 16);
        } else {
            output = static_cast<uint16_t>(0xFFFF - (phase_ >> 16));
        }

        // Drive all 4 CV outputs with the same waveform
        out.cv[0] = out.cv[1] = out.cv[2] = out.cv[3] = output;
    }

    void main_loop() override {
        // Nothing needed for this example.
        // Real apps would update parameters here.
    }

private:
    uint32_t phase_          = 0;
    uint32_t base_increment_ = 100;
};
