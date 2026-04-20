#pragma once

#include "oc/app.h"
#include "platform/drivers/weegfx.h"

/// TriggerToCV — copies trigger input 1 to CV output 1.
///
/// Gate high → CV output 1 driven to 5 V (0xFFFF DAC code).
/// Gate low  → CV output 1 driven to 0 V (0x0000 DAC code).
///
/// The other three CV outputs are held at 0 V.
/// The display shows the current state of the gate and the edge counter.

class TriggerToCV : public oc::Application {
public:
    void init() override {
        gate_       = false;
        edge_count_ = 0;
        flash_      = 0;
    }

    void audio_callback(const oc::Inputs& in, oc::Outputs& out) override {
        gate_ = in.gate[0];

        // Drive CV out 1: full scale = 5 V on O_C hardware, zero = 0 V
        out.cv[0] = gate_ ? 0xFFFFu : 0u;
        out.cv[1] = out.cv[2] = out.cv[3] = 0u;

        if (in.gate_edges & 0x01u) {
            ++edge_count_;
            flash_ = kFlashFrames;
        } else if (flash_ > 0) {
            --flash_;
        }
    }

    void draw(oc::Display* display) override {
        if (!display->begin_frame()) return;

        gfx_.Begin(display->frame_buffer(), true);

        gfx_.setPrintPos(0, 0);
        gfx_.print("Trig->CV");

        gfx_.setPrintPos(0, 16);
        gfx_.print("IN1: ");
        gfx_.print(gate_ ? "HIGH" : "low ");

        gfx_.setPrintPos(0, 28);
        gfx_.print("OUT1: ");
        gfx_.print(gate_ ? "5V " : "0V ");

        gfx_.setPrintPos(0, 42);
        gfx_.print("edges: ");
        gfx_.print(static_cast<int>(edge_count_));

        gfx_.setPrintPos(112, 42);
        gfx_.print(flash_ > 0 ? "*" : ".");

        gfx_.End();
        display->end_frame();
    }

private:
    static constexpr uint8_t kFlashFrames = 20;

    weegfx::Graphics gfx_;
    bool     gate_       = false;
    uint32_t edge_count_ = 0;
    uint8_t  flash_      = 0;
};
