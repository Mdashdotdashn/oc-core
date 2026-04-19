#pragma once

#include "oc/app.h"
#include "oc/calibration.h"
#include "platform/drivers/weegfx.h"

template <typename RuntimeT>
class CalibrationApp : public oc::Application {
public:
    explicit CalibrationApp(RuntimeT& runtime) : runtime_(runtime) {}

    void init() override {
        load_point();
    }

    void audio_callback(const oc::Inputs& in, oc::Outputs& out) override {
        if (in.buttons[0].just_pressed) {
            selected_channel_ = (selected_channel_ + 3) & 0x3;
            load_point();
        }
        if (in.buttons[1].just_pressed) {
            save_requested_ = true;
        }

        if (in.encoders[0].delta != 0) {
            selected_point_ += in.encoders[0].delta;
            if (selected_point_ < 0) {
                selected_point_ = 0;
            }
            if (selected_point_ >= static_cast<int>(oc::calibration::kDacVoltagePointCount)) {
                selected_point_ = oc::calibration::kDacVoltagePointCount - 1;
            }
            load_point();
        }

        if (in.encoders[0].click_just_pressed) {
            selected_channel_ = (selected_channel_ + 1) & 0x3;
            load_point();
        }

        if (in.encoders[1].click_just_pressed) {
            fine_step_ = !fine_step_;
        }

        if (in.encoders[1].delta != 0) {
            const int step = fine_step_ ? 1 : 16;
            int value = static_cast<int>(working_value_) + in.encoders[1].delta * step;
            if (value < 0) {
                value = 0;
            }
            if (value > 65535) {
                value = 65535;
            }
            working_value_ = static_cast<uint16_t>(value);
            oc::calibration::mutable_data().dac.calibrated_octaves[selected_channel_][selected_point_] = working_value_;
        }

        if (save_requested_) {
            saved_ok_ = oc::calibration::save(runtime_.hardware().storage_impl());
            save_flash_ = 40;
            save_requested_ = false;
        } else if (save_flash_ > 0) {
            --save_flash_;
        }

        for (int i = 0; i < 4; ++i) {
            out.cv[i] = oc::calibration::volts_to_dac(i, 0.0f);
        }
        out.cv[selected_channel_] = oc::calibration::dac_value_at(selected_channel_, selected_point_);
    }

    void draw(oc::Display* display) override {
        if (!display->begin_frame()) {
            return;
        }

        gfx_.Begin(display->frame_buffer(), true);

        gfx_.setPrintPos(0, 0);
        gfx_.print("DAC calib");
        if (save_flash_ > 0) {
            gfx_.setPrintPos(72, 0);
            gfx_.print(saved_ok_ ? "saved" : "error");
        }

        gfx_.setPrintPos(0, 11);
        gfx_.print("CH:");
        gfx_.print(selected_channel_ + 1);
        gfx_.setPrintPos(48, 11);
        gfx_.print("V:");
        print_voltage(selected_point_ + oc::calibration::kDacVoltageMin);

        gfx_.setPrintPos(0, 22);
        gfx_.print("DAC:");
        print_hex16(working_value_);
        gfx_.setPrintPos(72, 22);
        gfx_.print("S:");
        gfx_.print(fine_step_ ? "1" : "16");

        gfx_.setPrintPos(0, 33);
        gfx_.print("L:V  R:code");
        gfx_.setPrintPos(0, 44);
        gfx_.print("LC:CH DN:save");

        gfx_.End();
        display->end_frame();
    }

private:
    void load_point() {
        working_value_ = oc::calibration::dac_value_at(selected_channel_, selected_point_);
    }

    void print_voltage(int volts) {
        if (volts >= 0) {
            gfx_.print("+");
        }
        gfx_.print(volts);
    }

    void print_hex16(uint16_t value) {
        static const char kHexDigits[] = "0123456789ABCDEF";
        char buffer[5];

        buffer[0] = kHexDigits[(value >> 12) & 0xF];
        buffer[1] = kHexDigits[(value >> 8) & 0xF];
        buffer[2] = kHexDigits[(value >> 4) & 0xF];
        buffer[3] = kHexDigits[value & 0xF];
        buffer[4] = '\0';
        gfx_.print(buffer);
    }

    RuntimeT& runtime_;
    weegfx::Graphics gfx_;
    int selected_point_ = 3;
    uint8_t selected_channel_ = 0;
    uint16_t working_value_ = 0;
    bool fine_step_ = false;
    bool save_requested_ = false;
    bool saved_ok_ = false;
    uint8_t save_flash_ = 0;
};