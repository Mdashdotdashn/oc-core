#pragma once

#include <array>

#include "oc/app.h"
#include "oc/calibration.h"
#include "platform/drivers/weegfx.h"

/// TriggerToCV — multi-page hardware test app.
///
/// UP/DOWN buttons move through test pages (encoders/output/inputs).
/// CV output 1 still mirrors trigger input 1 for quick signal verification.

class TriggerToCV : public oc::Application {
public:
    void init() override {
        gate_          = false;
        gate_edges_    = 0;
        edge_count_    = 0;
        flash_         = 0;
        current_page_  = 0;
        left_val_      = 0;
        right_val_     = 0;
        up_held_       = false;
        down_held_     = false;
        left_click_    = false;
        right_click_   = false;
        cv_            = {0, 0, 0, 0};
        cv_raw_        = {0, 0, 0, 0};
    }

    void audio_callback(const oc::Inputs& in, oc::Outputs& out) override {
        if (in.buttons[0].just_pressed) {
            current_page_ = (current_page_ + kPageCount - 1) % kPageCount;
        }
        if (in.buttons[1].just_pressed) {
            current_page_ = (current_page_ + 1) % kPageCount;
        }

        gate_ = in.gate[0];
        gates_ = in.gate;
        gate_edges_ = in.gate_edges;
        left_val_  += in.encoders[0].delta;
        right_val_ += in.encoders[1].delta;
        up_held_    = in.buttons[0].pressed;
        down_held_  = in.buttons[1].pressed;
        left_click_ = in.encoders[0].click_pressed;
        right_click_= in.encoders[1].click_pressed;
        cv_         = in.cv;
        cv_raw_     = in.cv_raw;

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
        draw_page_body();
        draw_header();
        draw_footer();

        gfx_.End();
        display->end_frame();
    }

private:
    static constexpr uint8_t kFlashFrames = 20;
    static constexpr const char* kPageTitles[] = {"encoders", "cv inputs", "output"};
    static constexpr uint8_t kPageCount = sizeof(kPageTitles) / sizeof(kPageTitles[0]);

    void draw_page_body() {
        switch (current_page_) {
        case 0:
            draw_page_encoders();
            break;
        case 1:
            draw_page_cv_inputs();
            break;
        case 2:
            draw_page_output();
            break;
        default:
            break;
        }
    }

    void draw_page_encoders() {
        gfx_.setPrintPos(0, 14);
        gfx_.print("L:");
        gfx_.print(left_val_);

        gfx_.setPrintPos(64, 14);
        gfx_.print("R:");
        gfx_.print(right_val_);

        gfx_.setPrintPos(0, 28);
        gfx_.print("LC:");
        gfx_.print(left_click_ ? "1" : "0");

        gfx_.setPrintPos(64, 28);
        gfx_.print("RC:");
        gfx_.print(right_click_ ? "1" : "0");
    }

    void draw_page_cv_inputs() {
        draw_cv_input(0, 0, 14);
        draw_cv_input(1, 64, 14);
        draw_cv_input(2, 0, 28);
        draw_cv_input(3, 64, 28);
    }

    void draw_page_output() {
        gfx_.setPrintPos(0, 14);
        gfx_.print("IN1:");
        gfx_.print(gate_ ? "HIGH" : "low ");

        gfx_.setPrintPos(0, 28);
        gfx_.print("OUT1:");
        gfx_.print(gate_ ? "5V" : "0V");

        gfx_.setPrintPos(0, 42);
        gfx_.print("edges:");
        gfx_.print(static_cast<int>(edge_count_));

        gfx_.setPrintPos(112, 42);
        gfx_.print(flash_ > 0 ? "*" : ".");
    }

    void draw_page_inputs() {
        const uint32_t gates = gate_mask();

        gfx_.setPrintPos(0, 14);
        gfx_.print("G:");
        for (int i = 0; i < 4; ++i) {
            gfx_.print(gates & (1u << i) ? "1" : "0");
        }

        gfx_.setPrintPos(64, 14);
        gfx_.print("E:");
        gfx_.print(static_cast<int>(gate_edges_));

        gfx_.setPrintPos(0, 28);
        gfx_.print("CV1:");
        gfx_.print(static_cast<int>(cv_raw_[0]));

        gfx_.setPrintPos(0, 42);
        gfx_.print("CV2:");
        gfx_.print(static_cast<int>(cv_raw_[1]));
    }

    void draw_cv_input(uint8_t channel, int16_t x, int16_t y) {
        gfx_.setPrintPos(x, y);
        gfx_.print("CV");
        gfx_.print(static_cast<int>(channel + 1));
        gfx_.print(":");
        print_voltage(cv_[channel]);
    }

    void print_voltage(int32_t millivolts) {
        int32_t centivolts = millivolts;
        if (centivolts >= 0) {
            centivolts = (centivolts + 5) / 10;
        } else {
            centivolts = (centivolts - 5) / 10;
        }

        if (centivolts >= 0) {
            gfx_.print("+");
        } else {
            gfx_.print("-");
            centivolts = -centivolts;
        }

        gfx_.print(static_cast<int>(centivolts / 100));
        gfx_.print(".");
        const int32_t fractional = centivolts % 100;
        if (fractional < 10) {
            gfx_.print("0");
        }
        gfx_.print(static_cast<int>(fractional));
        gfx_.print("V");
    }

    void draw_header() {
        const char* title = kPageTitles[current_page_];
        const uint8_t title_len = string_length(title);
        const int16_t title_x = (128 - static_cast<int16_t>(title_len) * weegfx::Graphics::kFixedFontW) / 2;
        gfx_.setPrintPos(title_x < 0 ? 0 : title_x, 0);
        gfx_.print(title);
        gfx_.invertRect(0, 0, 128, 8);
    }

    void draw_footer() {
        static constexpr char kPrevLabel[] = " < ";
        static constexpr char kNextLabel[] = " > ";
        constexpr int16_t kLabelWidth = static_cast<int16_t>((sizeof(kPrevLabel) - 1) * weegfx::Graphics::kFixedFontW);
        constexpr int16_t next_x = 128 - kLabelWidth;

        gfx_.setPrintPos(0, 56);
        gfx_.print(kPrevLabel);
        gfx_.setPrintPos(next_x, 56);
        gfx_.print(kNextLabel);
        gfx_.invertRect(0, 56, kLabelWidth, 8);
        gfx_.invertRect(next_x, 56, kLabelWidth, 8);
    }

    uint32_t gate_mask() const {
        uint32_t mask = 0;
        for (uint8_t i = 0; i < 4; ++i) {
            if (gates_[i]) {
                mask |= (1u << i);
            }
        }
        return mask;
    }

    static uint8_t string_length(const char* text) {
        uint8_t len = 0;
        while (text[len] != '\0') {
            ++len;
        }
        return len;
    }

    weegfx::Graphics gfx_;
    std::array<bool, 4> gates_ = {false, false, false, false};
    std::array<int32_t, 4> cv_ = {0, 0, 0, 0};
    std::array<uint32_t, 4> cv_raw_ = {0, 0, 0, 0};
    bool gate_ = false;
    uint32_t gate_edges_ = 0;
    uint32_t edge_count_ = 0;
    uint8_t flash_ = 0;
    uint8_t current_page_ = 0;
    int32_t left_val_ = 0;
    int32_t right_val_ = 0;
    bool up_held_ = false;
    bool down_held_ = false;
    bool left_click_ = false;
    bool right_click_ = false;
};
