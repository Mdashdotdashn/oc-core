#pragma once

#include <Arduino.h>
#include "oc/app.h"
#include "oc/core/periodic_core.h"

namespace oc {

/// Runtime facade that owns the hardware platform, PeriodicCore, and the
/// validated ISR sequencing for an Application instance.
///
/// One Runtime instance may be active at a time; the timer HAL currently takes
/// a plain function pointer, so the ISR is dispatched through a static
/// trampoline.
template <typename Platform, bool WithDisplay = false>
class Runtime {
public:
    static constexpr uint32_t kDefaultUiIntervalUs = 1000;
    static constexpr uint8_t kNoTimingPin = 0xFF;

    void set_timing_pin(uint8_t pin) {
        timing_pin_ = pin;
    }

    void set_ui_interval_us(uint32_t interval_us) {
        ui_interval_us_ = interval_us;
    }

    void init(Application& app) {
        app_ = &app;
        active_instance_ = this;

        if (timing_pin_ != kNoTimingPin) {
            pinMode(timing_pin_, OUTPUT);
            digitalWriteFast(timing_pin_, LOW);
        }

        hw_.init_base();
        if constexpr (WithDisplay) {
            hw_.init_display();
        }
        core_.init(hw_.adc(), hw_.dac(), hw_.gpio());
        app_->init();
    }

    void start(uint32_t interval_us) {
        hw_.timer()->start(interval_us, isr_trampoline);
        if (ui_interval_us_ > 0) {
            hw_.timer()->start_ui(ui_interval_us_, ui_trampoline);
        }
    }

    void stop() {
        hw_.timer()->stop();
        hw_.timer()->stop_ui();
    }

    void poll() {
        app_->idle();
        if constexpr (WithDisplay) {
            app_->draw(hw_.display());
        }
    }

    Platform& hardware() { return hw_; }
    const Platform& hardware() const { return hw_; }

private:
    static void FASTRUN isr_trampoline() {
        if (active_instance_) {
            active_instance_->isr();
        }
    }

    static void FASTRUN ui_trampoline() {
        if (active_instance_) {
            active_instance_->ui_service();
        }
    }

    void FASTRUN ui_service() {
        hw_.buttons()->scan();
        hw_.encoders()->scan();
        ++ui_scan_generation_;
    }

    void FASTRUN isr() {
        if (timing_pin_ != kNoTimingPin) {
            digitalWriteFast(timing_pin_, HIGH);
        }

        if constexpr (WithDisplay) {
            // Display apps must start the next OLED page early in the ISR.
            hw_.display()->flush();
            hw_.dac()->flush();
            hw_.display()->update();
        }

        core_.isr_cycle();

        const uint32_t ui_generation = ui_scan_generation_;
        const bool has_new_ui_scan = ui_generation != last_ui_generation_consumed_;
        last_ui_generation_consumed_ = ui_generation;

        const core::CoreState& st = core_.get_state();
        AudioIn in{};
        for (int i = 0; i < 4; ++i) {
            in.cv[i] = st.inputs.cv[i];
            in.cv_raw[i] = st.inputs.cv_raw[i];
            in.gate[i] = st.inputs.gate[i];
        }
        in.gate_edges = st.inputs.edges;

        for (int i = 0; i < 2; ++i) {
            const auto button = hw_.buttons()->get(i);
            in.buttons[i] = {
                button.pressed,
                has_new_ui_scan ? button.just_pressed : false,
                has_new_ui_scan ? button.just_released : false,
            };

            const auto encoder = hw_.encoders()->get(i);
            in.encoders[i] = {
                static_cast<int8_t>(has_new_ui_scan ? encoder.delta : 0),
                encoder.click_pressed,
                has_new_ui_scan ? encoder.click_just_pressed : false,
                has_new_ui_scan ? encoder.click_just_released : false,
            };
        }

        AudioOut out{};
        app_->audio_callback(in, out);

        for (int i = 0; i < 4; ++i) {
            hw_.dac()->write(i, out.cv[i]);
        }

        if constexpr (!WithDisplay) {
            // Non-display apps keep the original immediate DAC flush behavior.
            hw_.dac()->flush();
        }

        if (timing_pin_ != kNoTimingPin) {
            digitalWriteFast(timing_pin_, LOW);
        }
    }

    inline static Runtime* active_instance_ = nullptr;

    Platform       hw_{};
    core::PeriodicCore core_{};
    Application*   app_ = nullptr;
    uint8_t        timing_pin_ = kNoTimingPin;
    uint32_t       ui_interval_us_ = kDefaultUiIntervalUs;
    volatile uint32_t ui_scan_generation_ = 0;
    uint32_t       last_ui_generation_consumed_ = 0;
};

} // namespace oc