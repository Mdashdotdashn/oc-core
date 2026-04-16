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
template <typename Platform>
class Runtime {
public:
    static constexpr uint32_t kDefaultUiIntervalUs = 1000;
    static constexpr uint8_t kNoTimingPin = 0xFF;
    static constexpr uint8_t kIsrAverageShift = 4;

    void set_timing_pin(uint8_t pin) {
        timing_pin_ = pin;
    }

    void set_ui_interval_us(uint32_t interval_us) {
        ui_interval_us_ = interval_us;
    }

    uint32_t isr_average_cycles() const {
        return isr_avg_cycles_;
    }

    uint32_t isr_average_us() const {
        const uint32_t cycles_per_us = F_CPU / 1000000U;
        return cycles_per_us ? ((isr_avg_cycles_ + cycles_per_us / 2) / cycles_per_us) : 0;
    }

    uint8_t isr_load_percent() const {
        const uint32_t budget_cycles = isr_budget_cycles();
        if (!budget_cycles) {
            return 0;
        }
        const uint32_t percent = static_cast<uint32_t>(
            (static_cast<uint64_t>(isr_avg_cycles_) * 100U + budget_cycles / 2) / budget_cycles);
        return static_cast<uint8_t>(percent > 100U ? 100U : percent);
    }

    void init(Application& app) {
        app_ = &app;
        active_instance_ = this;

        if (timing_pin_ != kNoTimingPin) {
            pinMode(timing_pin_, OUTPUT);
            digitalWriteFast(timing_pin_, LOW);
        }

        enable_cycle_counter();

        hw_.init_all();
        core_.init(hw_.adc(), hw_.dac(), hw_.gpio());
        app_->init();
    }

    void start(uint32_t interval_us) {
        core_interval_us_ = interval_us;
        isr_avg_cycles_ = 0;
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
        app_->draw(hw_.display());
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
        const uint32_t start_cycles = current_cycle_count();

        if (timing_pin_ != kNoTimingPin) {
            digitalWriteFast(timing_pin_, HIGH);
        }

        // The OLED is always present and shares SPI0 with the DAC. Start the
        // next page transfer first, then flush the DAC values staged last tick.
        hw_.display()->flush();
        hw_.dac()->flush();
        hw_.display()->update();

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

        if (timing_pin_ != kNoTimingPin) {
            digitalWriteFast(timing_pin_, LOW);
        }

        const uint32_t elapsed_cycles = current_cycle_count() - start_cycles;
        int32_t avg_cycles = static_cast<int32_t>(isr_avg_cycles_);
        avg_cycles += (static_cast<int32_t>(elapsed_cycles) - avg_cycles) >> kIsrAverageShift;
        isr_avg_cycles_ = static_cast<uint32_t>(avg_cycles);
    }

    static void enable_cycle_counter() {
#if defined(ARM_DEMCR) && defined(ARM_DWT_CTRL) && defined(ARM_DWT_CYCCNT) && defined(ARM_DEMCR_TRCENA) && defined(ARM_DWT_CTRL_CYCCNTENA)
        ARM_DEMCR |= ARM_DEMCR_TRCENA;
        ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
        ARM_DWT_CYCCNT = 0;
#endif
    }

    static uint32_t current_cycle_count() {
#if defined(ARM_DWT_CYCCNT)
        return ARM_DWT_CYCCNT;
#else
        return 0;
#endif
    }

    uint32_t isr_budget_cycles() const {
        return static_cast<uint32_t>((static_cast<uint64_t>(F_CPU) * core_interval_us_) / 1000000ULL);
    }

    inline static Runtime* active_instance_ = nullptr;

    Platform       hw_{};
    core::PeriodicCore core_{};
    Application*   app_ = nullptr;
    uint8_t        timing_pin_ = kNoTimingPin;
    uint32_t       core_interval_us_ = 0;
    uint32_t       ui_interval_us_ = kDefaultUiIntervalUs;
    volatile uint32_t ui_scan_generation_ = 0;
    uint32_t       last_ui_generation_consumed_ = 0;
    volatile uint32_t isr_avg_cycles_ = 0;
};

} // namespace oc