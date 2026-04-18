#pragma once

#include "oc/app.h"
#include "drivers/weegfx.h"

/// TriggerTest — shows the 4 gate inputs and counts rising edges.
///
/// Each row displays one trigger input:
///   Tn <0/1> E:<count> *
///
/// `0/1` is the current logic state, `E` is the accumulated rising-edge count,
/// and `*` lights briefly after each detected edge.

class TriggerTest : public oc::Application {
public:
    void init() override {
        gate_state_.fill(false);
        edge_count_.fill(0);
        edge_flash_.fill(0);
    }

    void audio_callback(const oc::AudioIn& in, oc::AudioOut& out) override {
        out.cv[0] = out.cv[1] = out.cv[2] = out.cv[3] = 0;

        for (int i = 0; i < 4; ++i) {
            gate_state_[i] = in.gate[i];

            if (in.gate_edges & (1u << i)) {
                ++edge_count_[i];
                edge_flash_[i] = kFlashFrames;
            } else if (edge_flash_[i] > 0) {
                --edge_flash_[i];
            }
        }
    }

    void draw(oc::Display* display) override {
        if (!display->begin_frame()) {
            return;
        }

        gfx_.Begin(display->frame_buffer(), true);

        gfx_.setPrintPos(0, 0);
        gfx_.print("Trigger test");

        for (int i = 0; i < 4; ++i) {
            const int y = 14 + i * 12;
            gfx_.setPrintPos(0, y);
            gfx_.print("T");
            gfx_.print(i + 1);
            gfx_.print(" ");
            gfx_.print(gate_state_[i] ? "1" : "0");

            gfx_.setPrintPos(34, y);
            gfx_.print("E:");
            gfx_.print(static_cast<int>(edge_count_[i]));

            gfx_.setPrintPos(104, y);
            gfx_.print(edge_flash_[i] > 0 ? "*" : ".");
        }

        gfx_.End();
        display->end_frame();
    }

private:
    static constexpr uint8_t kFlashFrames = 20;

    weegfx::Graphics gfx_;
    std::array<bool, 4> gate_state_{};
    std::array<uint32_t, 4> edge_count_{};
    std::array<uint8_t, 4> edge_flash_{};
};