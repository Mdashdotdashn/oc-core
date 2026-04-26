#pragma once

#include "oc/app.h"
#include "oc/calibration.h"
#include "platform/drivers/weegfx.h"

enum class CalWizardPhase { kDac, kAdc };

template <typename RuntimeT>
class CalibrationApp : public oc::Application {
public:
    explicit CalibrationApp(RuntimeT& runtime) : runtime_(runtime) {}

    void init() override {
        phase_ = CalWizardPhase::kDac;
        load_dac_point();
    }

    void audio_callback(const oc::Inputs& in, oc::Outputs& out) override {
        if (phase_ == CalWizardPhase::kDac) {
            handle_dac(in);
        } else {
            handle_adc(in);
        }

        if (save_flash_ > 0) {
            --save_flash_;
        }
        if (capture_flash_ > 0) {
            --capture_flash_;
        }

        if (phase_ == CalWizardPhase::kDac) {
            for (int i = 0; i < 4; ++i) {
                out.cv[i] = oc::calibration::volts_to_dac(i, 0.0f);
            }
            out.cv[dac_channel_] = oc::calibration::dac_value_at(dac_channel_, dac_point_);
        } else {
            const float calibration_voltage =
                static_cast<float>(oc::calibration::kAdcCalibrationVoltages[adc_point_]);
            for (int i = 0; i < 4; ++i) {
                out.cv[i] = oc::calibration::volts_to_dac(i, calibration_voltage);
            }
        }
    }

    void draw(oc::Display* display) override {
        if (!display->begin_frame()) {
            return;
        }

        gfx_.Begin(display->frame_buffer(), true);

        if (phase_ == CalWizardPhase::kDac) {
            draw_dac();
        } else {
            draw_adc();
        }

        gfx_.End();
        display->end_frame();
    }

private:
    void handle_dac(const oc::Inputs& in) {
        if (in.buttons[0].just_pressed) {
            dac_channel_ = (dac_channel_ + 3) & 0x3;
            load_dac_point();
        }
        if (in.buttons[1].just_pressed) {
            save_calibration();
            return;
        }
        if (in.encoders[0].delta != 0) {
            dac_point_ = clamp(
                dac_point_ + in.encoders[0].delta,
                0,
                static_cast<int>(oc::calibration::kDacVoltagePointCount) - 1);
            load_dac_point();
        }
        if (in.encoders[0].click_just_pressed) {
            phase_ = CalWizardPhase::kAdc;
            return;
        }
        if (in.encoders[1].click_just_pressed) {
            fine_step_ = !fine_step_;
        }
        if (in.encoders[1].delta != 0) {
            const int step = fine_step_ ? 1 : 16;
            const int value = clamp(static_cast<int>(dac_working_value_) + in.encoders[1].delta * step, 0, 65535);
            dac_working_value_ = static_cast<uint16_t>(value);
            oc::calibration::mutable_data().dac.calibrated_octaves[dac_channel_][dac_point_] = dac_working_value_;
        }
    }

    void draw_dac() {
        gfx_.setPrintPos(0, 0);
        gfx_.print("DAC cal");
        draw_save_status(66);

        gfx_.setPrintPos(0, 11);
        gfx_.print("CH:");
        gfx_.print(dac_channel_ + 1);
        gfx_.setPrintPos(48, 11);
        gfx_.print("V:");
        print_voltage(dac_point_ + oc::calibration::kDacVoltageMin);

        gfx_.setPrintPos(0, 22);
        gfx_.print("DAC:");
        print_hex16(dac_working_value_);
        gfx_.setPrintPos(72, 22);
        gfx_.print("S:");
        gfx_.print(fine_step_ ? "1" : "16");

        gfx_.setPrintPos(0, 33);
        gfx_.print("L:V  R:code");
        gfx_.setPrintPos(0, 44);
        gfx_.print("LC:ADC> DN:save");
    }

    void handle_adc(const oc::Inputs& in) {
        if (in.buttons[0].just_pressed) {
            phase_ = CalWizardPhase::kDac;
            load_dac_point();
            return;
        }
        if (in.buttons[1].just_pressed) {
            save_calibration();
            return;
        }
        if (in.encoders[0].delta != 0) {
            adc_channel_ = static_cast<uint8_t>(
                clamp(static_cast<int>(adc_channel_) + in.encoders[0].delta, 0, 3));
        }
        if (in.encoders[1].click_just_pressed) {
            capture_adc_point();
        }
    }

    void draw_adc() {
        gfx_.setPrintPos(0, 0);
        gfx_.print("ADC cal");
        draw_save_status(66);

        gfx_.setPrintPos(0, 11);
        gfx_.print("C");
        gfx_.print(adc_channel_ + 1);
        gfx_.print(" V:");
        print_voltage(oc::calibration::kAdcCalibrationVoltages[adc_point_]);
        gfx_.setPrintPos(84, 11);
        gfx_.print(static_cast<int>(adc_point_ + 1));
        gfx_.print("/");
        gfx_.print(static_cast<int>(oc::calibration::kAdcCalibrationPointCount));

        gfx_.setPrintPos(0, 22);
        {
            const int32_t raw = static_cast<int32_t>(runtime_.hardware().adc()->get_smoothed(adc_channel_));
            const int32_t stored =
                static_cast<int32_t>(oc::calibration::data().adc.points[adc_channel_][adc_point_]);
            const int32_t delta = raw - stored;
            if (capture_flash_ > 0) {
                gfx_.print("* ");
            }
            gfx_.print("d:");
            if (delta >= 0) {
                gfx_.print("+");
            }
            gfx_.print(static_cast<int>(delta));
        }

        gfx_.setPrintPos(0, 33);
        gfx_.print("RC:cap  L:ch");
        gfx_.setPrintPos(0, 44);
        gfx_.print("UP:DAC  DN:sav");
    }

    void draw_save_status(uint8_t x) {
        if (save_flash_ > 0) {
            gfx_.setPrintPos(x, 0);
            gfx_.print(saved_ok_ ? "SAVED" : "ERROR");
        }
    }

    void save_calibration() {
        saved_ok_ = oc::calibration::save(runtime_.storage());
        runtime_.hardware().apply_calibration(oc::calibration::data());
        save_flash_ = 60;
    }

    void capture_adc_point() {
        const uint32_t smoothed = runtime_.hardware().adc()->get_smoothed(adc_channel_);
        last_captured_ = static_cast<uint16_t>(smoothed > 65535u ? 65535u : smoothed);

        auto& points = oc::calibration::mutable_data().adc.points[adc_channel_];
        points[adc_point_] = last_captured_;
        runtime_.hardware().adc()->set_calibration_points(
            adc_channel_,
            points.data(),
            oc::calibration::kAdcCalibrationPointCount);
        capture_flash_ = 25;

        if (++adc_point_ >= oc::calibration::kAdcCalibrationPointCount) {
            adc_point_ = 0;
            if (adc_channel_ < 3) {
                ++adc_channel_;
            }
        }
    }

    void load_dac_point() {
        dac_working_value_ = oc::calibration::dac_value_at(dac_channel_, dac_point_);
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

    static int clamp(int value, int low, int high) {
        return value < low ? low : (value > high ? high : value);
    }

    RuntimeT& runtime_;
    weegfx::Graphics gfx_;

    CalWizardPhase phase_ = CalWizardPhase::kDac;

    int dac_point_ = 3;
    uint8_t dac_channel_ = 0;
    uint16_t dac_working_value_ = 0;
    bool fine_step_ = false;

    uint8_t adc_channel_ = 0;
    uint8_t adc_point_ = 0;
    uint16_t last_captured_ = 0;
    uint8_t capture_flash_ = 0;

    bool saved_ok_ = false;
    uint8_t save_flash_ = 0;
};