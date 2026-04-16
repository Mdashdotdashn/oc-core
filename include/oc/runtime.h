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
    static constexpr uint8_t kNoTimingPin = 0xFF;

    void set_timing_pin(uint8_t pin) {
        timing_pin_ = pin;
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
    }

    void stop() {
        hw_.timer()->stop();
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
        hw_.buttons()->scan();
        hw_.encoders()->scan();

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
            in.buttons[i] = {button.pressed, button.just_pressed, button.just_released};

            const auto encoder = hw_.encoders()->get(i);
            in.encoders[i] = {encoder.delta, encoder.click_pressed,
                              encoder.click_just_pressed, encoder.click_just_released};
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
};

} // namespace oc