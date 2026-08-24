#pragma once

#include "oc/app.h"
#include "oc/calibration.h"
#include "platform/drivers/weegfx.h"

enum class CalWizardPhase { kDac, kAdc, kDisplay };

template <typename RuntimeT>
class CalibrationApp : public oc::Application {
public:
    explicit CalibrationApp(RuntimeT& runtime) : runtime_(runtime) {}

    void init() override {
        phase_ = CalWizardPhase::kDisplay;
        display_offset_ = oc::calibration::data().display_offset;
        runtime_.hardware().display_impl().set_offset(display_offset_);
        load_dac_point();
        reset_adc_auto();
    }

    void ui_callback(const std::array<oc::ButtonState, 2>& buttons,
                     const std::array<oc::EncoderState, 2>& encoders) override {
        handle_page_navigation(buttons);
        if (phase_ == CalWizardPhase::kDac) {
            handle_dac(encoders);
        } else if (phase_ == CalWizardPhase::kAdc) {
            // ADC calibration auto-sequence runs in the audio ISR.
        } else {
            handle_display(encoders);
        }
    }

    void audio_callback(const oc::Application::Input& in, oc::Outputs& out) override {
        if (phase_ == CalWizardPhase::kDac) {
            // DAC phase is driven by UI encoder updates.
        } else if (phase_ == CalWizardPhase::kAdc) {
            handle_adc(in);
        } else {
            // Display phase is driven by UI encoder updates.
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
        } else if (phase_ == CalWizardPhase::kAdc) {
            render_adc_outputs(out);
        } else {
            for (int i = 0; i < 4; ++i) {
                out.cv[i] = oc::calibration::volts_to_dac(i, 0.0f);
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
        } else if (phase_ == CalWizardPhase::kAdc) {
            draw_adc();
        } else {
            draw_display();
        }

        draw_footer();

        gfx_.End();
        display->end_frame();
    }

private:
    enum class AdcAutoState {
        kStandby,
        kSettling,
        kAwaitUnplug,
    };

    void handle_page_navigation(const std::array<oc::ButtonState, 2>& buttons) {
        if (buttons[0].just_pressed) {
            save_calibration();
            switch_phase(previous_phase(phase_));
            return;
        }

        if (buttons[1].just_pressed) {
            save_calibration();
            switch_phase(next_phase(phase_));
        }
    }

    static CalWizardPhase previous_phase(CalWizardPhase phase) {
        switch (phase) {
        case CalWizardPhase::kDisplay: return CalWizardPhase::kAdc;
        case CalWizardPhase::kDac: return CalWizardPhase::kDisplay;
        case CalWizardPhase::kAdc: return CalWizardPhase::kDac;
        }
        return CalWizardPhase::kDisplay;
    }

    static CalWizardPhase next_phase(CalWizardPhase phase) {
        switch (phase) {
        case CalWizardPhase::kDisplay: return CalWizardPhase::kDac;
        case CalWizardPhase::kDac: return CalWizardPhase::kAdc;
        case CalWizardPhase::kAdc: return CalWizardPhase::kDisplay;
        }
        return CalWizardPhase::kDisplay;
    }

    void switch_phase(CalWizardPhase next) {
        phase_ = next;
        if (phase_ == CalWizardPhase::kDisplay) {
            display_offset_ = oc::calibration::data().display_offset;
            runtime_.hardware().display_impl().set_offset(display_offset_);
        } else if (phase_ == CalWizardPhase::kDac) {
            load_dac_point();
        } else {
            reset_adc_auto();
        }
    }

    void handle_dac(const std::array<oc::EncoderState, 2>& encoders) {
        if (encoders[0].click_just_pressed) {
            dac_channel_ = (dac_channel_ + 3) & 0x3;
            load_dac_point();
        }
        if (encoders[1].click_just_pressed) {
            dac_channel_ = (dac_channel_ + 1) & 0x3;
            load_dac_point();
        }
        if (encoders[0].delta != 0) {
            dac_point_ = clamp(
                dac_point_ + encoders[0].delta,
                0,
                static_cast<int>(oc::calibration::kDacVoltagePointCount) - 1);
            load_dac_point();
        }
        if (encoders[1].delta != 0) {
            const int value = clamp(static_cast<int>(dac_working_value_) + encoders[1].delta, 0, 65535);
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

        gfx_.setPrintPos(0, 33);
        gfx_.print("ENC0:V  ENC1:code");
        gfx_.setPrintPos(0, 44);
        gfx_.print("LC/RC:CH");
    }

    void handle_adc(const oc::Application::Input& in) {
        if (++adc_update_div_ < kAdcUpdateDivider) {
            return;
        }
        adc_update_div_ = 0;

        if (adc_auto_state_ == AdcAutoState::kStandby) {
            update_adc_standby(in);
        } else if (adc_auto_state_ == AdcAutoState::kSettling) {
            update_adc_settling(in);
        } else {
            update_adc_unplug(in);
        }
    }

    void update_adc_standby(const oc::Application::Input& in) {
        const uint8_t channel = adc_probe_channel_;
        adc_probe_sum_ += in.cv_raw[channel];
        ++adc_probe_count_;

        if (adc_probe_count_ < kProbeSampleCount) {
            return;
        }

        const uint16_t mean = static_cast<uint16_t>(adc_probe_sum_ / adc_probe_count_);
        if (adc_probe_high_phase_) {
            adc_probe_high_mean_ = mean;
            adc_probe_has_high_ = true;
        } else {
            adc_probe_low_mean_ = mean;
            adc_probe_has_low_ = true;
        }

        adc_probe_sum_ = 0;
        adc_probe_count_ = 0;
        adc_probe_high_phase_ = !adc_probe_high_phase_;

        if (!(adc_probe_has_low_ && adc_probe_has_high_)) {
            return;
        }

        const uint16_t delta = adc_probe_high_mean_ > adc_probe_low_mean_
            ? static_cast<uint16_t>(adc_probe_high_mean_ - adc_probe_low_mean_)
            : static_cast<uint16_t>(adc_probe_low_mean_ - adc_probe_high_mean_);

        adc_probe_has_low_ = false;
        adc_probe_has_high_ = false;

        if (delta >= kCableDetectDeltaRaw) {
            adc_active_channel_ = adc_probe_channel_;
            adc_point_ = 0;
            adc_auto_state_ = AdcAutoState::kSettling;
            reset_adc_window();
            return;
        }

        adc_probe_channel_ = static_cast<uint8_t>((adc_probe_channel_ + 1) & 0x3);
    }

    void update_adc_settling(const oc::Application::Input& in) {
        const uint16_t sample = static_cast<uint16_t>(in.cv_raw[adc_active_channel_]);
        if (sample < adc_window_min_) adc_window_min_ = sample;
        if (sample > adc_window_max_) adc_window_max_ = sample;
        adc_window_sum_ += sample;
        ++adc_window_count_;

        if (adc_window_count_ < kSettledSampleCount) {
            return;
        }

        const uint16_t span = static_cast<uint16_t>(adc_window_max_ - adc_window_min_);
        if (span > kSettledSpanRaw) {
            reset_adc_window();
            return;
        }

        const uint16_t captured = static_cast<uint16_t>(adc_window_sum_ / adc_window_count_);
        auto& points = oc::calibration::mutable_data().adc.points[adc_active_channel_];
        points[adc_point_] = captured;
        runtime_.hardware().adc()->set_calibration_points(
            adc_active_channel_,
            points.data(),
            oc::calibration::kAdcCalibrationPointCount);

        capture_flash_ = 25;
        ++adc_point_;
        reset_adc_window();

        if (adc_point_ >= oc::calibration::kAdcCalibrationPointCount) {
            adc_auto_state_ = AdcAutoState::kAwaitUnplug;
            adc_unplug_grace_windows_ = kUnplugGraceWindows;
            adc_unplug_confirm_windows_ = 0;
            adc_unplug_high_phase_ = false;
            adc_unplug_has_low_ = false;
            adc_unplug_has_high_ = false;
            adc_unplug_low_mean_ = 0;
            adc_unplug_high_mean_ = 0;
            adc_unplug_sum_ = 0;
            adc_unplug_count_ = 0;
            reset_adc_window();
        }
    }

    void update_adc_unplug(const oc::Application::Input& in) {
        adc_unplug_sum_ += in.cv_raw[adc_active_channel_];
        ++adc_unplug_count_;

        if (adc_unplug_count_ < kProbeSampleCount) {
            return;
        }

        if (adc_unplug_grace_windows_ > 0) {
            --adc_unplug_grace_windows_;
            adc_unplug_sum_ = 0;
            adc_unplug_count_ = 0;
            adc_unplug_high_phase_ = !adc_unplug_high_phase_;
            return;
        }

        const uint16_t mean = static_cast<uint16_t>(adc_unplug_sum_ / adc_unplug_count_);
        if (adc_unplug_high_phase_) {
            adc_unplug_high_mean_ = mean;
            adc_unplug_has_high_ = true;
        } else {
            adc_unplug_low_mean_ = mean;
            adc_unplug_has_low_ = true;
        }

        adc_unplug_sum_ = 0;
        adc_unplug_count_ = 0;
        adc_unplug_high_phase_ = !adc_unplug_high_phase_;

        if (!(adc_unplug_has_low_ && adc_unplug_has_high_)) {
            return;
        }

        const uint16_t delta = adc_unplug_high_mean_ > adc_unplug_low_mean_
            ? static_cast<uint16_t>(adc_unplug_high_mean_ - adc_unplug_low_mean_)
            : static_cast<uint16_t>(adc_unplug_low_mean_ - adc_unplug_high_mean_);

        adc_unplug_has_low_ = false;
        adc_unplug_has_high_ = false;

        const bool looks_unplugged = delta <= kCableReleaseDeltaRaw;

        if (looks_unplugged) {
            ++adc_unplug_confirm_windows_;
            if (adc_unplug_confirm_windows_ >= kUnplugConfirmWindows) {
                reset_adc_auto();
                return;
            }
        } else {
            adc_unplug_confirm_windows_ = 0;
        }
    }

    void reset_adc_window() {
        adc_window_min_ = 4095;
        adc_window_max_ = 0;
        adc_window_sum_ = 0;
        adc_window_count_ = 0;
    }

    void reset_adc_auto() {
        adc_auto_state_ = AdcAutoState::kStandby;
        adc_probe_channel_ = 0;
        adc_probe_high_phase_ = false;
        adc_probe_has_low_ = false;
        adc_probe_has_high_ = false;
        adc_probe_low_mean_ = 0;
        adc_probe_high_mean_ = 0;
        adc_probe_sum_ = 0;
        adc_probe_count_ = 0;
        adc_active_channel_ = 0;
        adc_point_ = 0;
        adc_update_div_ = 0;
        capture_flash_ = 0;
        adc_unplug_grace_windows_ = 0;
        adc_unplug_confirm_windows_ = 0;
        adc_unplug_high_phase_ = false;
        adc_unplug_has_low_ = false;
        adc_unplug_has_high_ = false;
        adc_unplug_low_mean_ = 0;
        adc_unplug_high_mean_ = 0;
        adc_unplug_sum_ = 0;
        adc_unplug_count_ = 0;
        reset_adc_window();
    }

    void render_adc_outputs(oc::Outputs& out) {
        for (int i = 0; i < 4; ++i) {
            out.cv[i] = oc::calibration::volts_to_dac(i, 0.0f);
        }

        if (adc_auto_state_ == AdcAutoState::kStandby) {
            const float probe_voltage = adc_probe_high_phase_ ? kProbeHighVoltage : kProbeLowVoltage;
            out.cv[kAdcSourceOutputChannel] = oc::calibration::volts_to_dac(kAdcSourceOutputChannel, probe_voltage);
            return;
        }

        if (adc_auto_state_ == AdcAutoState::kSettling) {
            const int index = adc_point_ < oc::calibration::kAdcCalibrationPointCount
                ? adc_point_
                : static_cast<int>(oc::calibration::kAdcCalibrationPointCount) - 1;
            const float voltage = static_cast<float>(oc::calibration::kAdcCalibrationVoltages[index]);
            out.cv[kAdcSourceOutputChannel] = oc::calibration::volts_to_dac(kAdcSourceOutputChannel, voltage);
            return;
        }

        if (adc_auto_state_ == AdcAutoState::kAwaitUnplug) {
            const float probe_voltage = adc_unplug_high_phase_ ? kProbeHighVoltage : kProbeLowVoltage;
            out.cv[kAdcSourceOutputChannel] = oc::calibration::volts_to_dac(kAdcSourceOutputChannel, probe_voltage);
        }
    }

    void draw_adc() {
        gfx_.setPrintPos(0, 0);
        gfx_.print("ADC cal");
        draw_save_status(66);

        if (adc_auto_state_ == AdcAutoState::kStandby) {
            gfx_.setPrintPos(0, 11);
            gfx_.print("standby");

            gfx_.setPrintPos(0, 22);
            gfx_.print("Patch OUT1->CV");
            gfx_.print(static_cast<int>(adc_probe_channel_ + 1));

            gfx_.setPrintPos(0, 33);
            gfx_.print("Probe:");
            print_voltage(static_cast<int>(adc_probe_high_phase_ ? kProbeHighVoltage : kProbeLowVoltage));
            return;
        }

        if (adc_auto_state_ == AdcAutoState::kSettling) {
            const int point_index = adc_point_ < oc::calibration::kAdcCalibrationPointCount
                ? adc_point_
                : static_cast<int>(oc::calibration::kAdcCalibrationPointCount) - 1;

            gfx_.setPrintPos(0, 11);
            gfx_.print("OUT1->CV");
            gfx_.print(static_cast<int>(adc_active_channel_ + 1));

            gfx_.setPrintPos(0, 22);
            gfx_.print("V:");
            print_voltage(oc::calibration::kAdcCalibrationVoltages[point_index]);
            gfx_.setPrintPos(72, 22);
            gfx_.print(static_cast<int>(point_index + 1));
            gfx_.print("/");
            gfx_.print(static_cast<int>(oc::calibration::kAdcCalibrationPointCount));

            gfx_.setPrintPos(0, 33);
            gfx_.print(capture_flash_ > 0 ? "captured" : "wait stable");
            return;
        }

        gfx_.setPrintPos(0, 11);
        gfx_.print("OUT1->CV");
        gfx_.print(static_cast<int>(adc_active_channel_ + 1));

        gfx_.setPrintPos(0, 22);
        gfx_.print("Done");

        gfx_.setPrintPos(0, 33);
        gfx_.print("Remove cable");
    }

    void handle_display(const std::array<oc::EncoderState, 2>& encoders) {
        if (encoders[0].delta != 0) {
            const int value = clamp(
                static_cast<int>(display_offset_) + encoders[0].delta,
                0,
                15);
            display_offset_ = static_cast<uint8_t>(value);
            runtime_.hardware().display_impl().set_offset(display_offset_);
        }
    }

    void draw_display() {
        gfx_.setPrintPos(8, 0);
        gfx_.print("Display");
        // Header bar in reverse, plus one title-height strip with black edge/center marks.
        gfx_.invertRect(0, 0, 128, 8);
        draw_save_status(66);
        gfx_.drawRect(0, 8, 128, 8);
        gfx_.clearRect(0, 8, 2, 8);
        gfx_.clearRect(63, 8, 2, 8);
        gfx_.clearRect(126, 8, 2, 8);

        gfx_.setPrintPos(0, 22);
        gfx_.print("offset:");
        gfx_.print(static_cast<int>(display_offset_));

        gfx_.setPrintPos(0, 33);
        gfx_.print("ENC0:+/-");
    }

    void draw_footer() {
        static constexpr char kPrevLabel[] = "< dn";
        static constexpr char kNextLabel[] = "up >";
        constexpr int16_t kLabelWidth = static_cast<int16_t>((sizeof(kPrevLabel) - 1) * weegfx::Graphics::kFixedFontW);
        constexpr int16_t next_x = 128 - kLabelWidth;

        gfx_.setPrintPos(0, 56);
        gfx_.print(kPrevLabel);
        gfx_.setPrintPos(next_x, 56);
        gfx_.print(kNextLabel);
        gfx_.invertRect(0, 56, kLabelWidth, 8);
        gfx_.invertRect(next_x, 56, kLabelWidth, 8);
    }

    void draw_save_status(uint8_t x) {
        if (save_flash_ > 0) {
            gfx_.setPrintPos(x, 0);
            gfx_.print(saved_ok_ ? "SAVED" : "ERROR");
        }
    }

    void save_calibration() {
        oc::calibration::mutable_data().display_offset = display_offset_;
        saved_ok_ = oc::calibration::save(runtime_.storage());
        runtime_.hardware().apply_calibration(oc::calibration::data());
        save_flash_ = 60;
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

    static constexpr uint16_t kAdcUpdateDivider = 100;
    static constexpr uint8_t kAdcSourceOutputChannel = 0;
    static constexpr uint16_t kProbeSampleCount = 25;
    static constexpr uint16_t kCableDetectDeltaRaw = 180;
    static constexpr uint16_t kSettledSampleCount = 35;
    static constexpr uint16_t kSettledSpanRaw = 14;
    static constexpr uint16_t kCableReleaseDeltaRaw = 100;
    static constexpr uint16_t kUnplugGraceWindows = 4;
    static constexpr uint16_t kUnplugConfirmWindows = 3;
    static constexpr float kProbeLowVoltage = -2.0f;
    static constexpr float kProbeHighVoltage = 3.0f;

    RuntimeT& runtime_;
    weegfx::Graphics gfx_;

    CalWizardPhase phase_ = CalWizardPhase::kDac;

    int dac_point_ = 3;
    uint8_t dac_channel_ = 0;
    uint16_t dac_working_value_ = 0;

    AdcAutoState adc_auto_state_ = AdcAutoState::kStandby;
    uint8_t adc_probe_channel_ = 0;
    bool adc_probe_high_phase_ = false;
    bool adc_probe_has_low_ = false;
    bool adc_probe_has_high_ = false;
    uint16_t adc_probe_low_mean_ = 0;
    uint16_t adc_probe_high_mean_ = 0;
    uint32_t adc_probe_sum_ = 0;
    uint16_t adc_probe_count_ = 0;
    uint8_t adc_active_channel_ = 0;
    uint8_t adc_point_ = 0;
    uint16_t adc_window_min_ = 4095;
    uint16_t adc_window_max_ = 0;
    uint32_t adc_window_sum_ = 0;
    uint16_t adc_window_count_ = 0;
    uint16_t adc_update_div_ = 0;
    uint16_t adc_unplug_grace_windows_ = 0;
    uint16_t adc_unplug_confirm_windows_ = 0;
    bool adc_unplug_high_phase_ = false;
    bool adc_unplug_has_low_ = false;
    bool adc_unplug_has_high_ = false;
    uint16_t adc_unplug_low_mean_ = 0;
    uint16_t adc_unplug_high_mean_ = 0;
    uint32_t adc_unplug_sum_ = 0;
    uint16_t adc_unplug_count_ = 0;
    uint8_t display_offset_ = 2;
    uint8_t capture_flash_ = 0;

    bool saved_ok_ = false;
    uint8_t save_flash_ = 0;
};