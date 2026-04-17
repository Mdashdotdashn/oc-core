#pragma once

#include <Arduino.h>
#include "oc/app.h"
#include "oc/calibration.h"
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

    struct IsrProfile {
        uint32_t total_cycles;
        uint32_t display_cycles;
        uint32_t dac_flush_cycles;
        uint32_t scan_cycles;
        uint32_t marshal_cycles;
        uint32_t app_cycles;
        uint32_t output_cycles;
    };

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
        return cycles_to_us(isr_avg_cycles_);
    }

    uint8_t isr_load_percent() const {
        return cycles_to_load_percent(isr_avg_cycles_);
    }

    IsrProfile isr_profile() const {
        return {
            isr_avg_cycles_,
            isr_display_avg_cycles_,
            isr_dac_flush_avg_cycles_,
            isr_scan_avg_cycles_,
            isr_marshal_avg_cycles_,
            isr_app_avg_cycles_,
            isr_output_avg_cycles_,
        };
    }

    uint32_t cycles_to_us(uint32_t cycles) const {
        const uint32_t cycles_per_us = F_CPU / 1000000U;
        return cycles_per_us ? ((cycles + cycles_per_us / 2) / cycles_per_us) : 0;
    }

    uint8_t cycles_to_load_percent(uint32_t cycles) const {
        const uint32_t budget_cycles = isr_budget_cycles();
        if (!budget_cycles) {
            return 0;
        }
        const uint32_t percent = static_cast<uint32_t>(
            (static_cast<uint64_t>(cycles) * 100U + budget_cycles / 2) / budget_cycles);
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
        calibration::initialize(hw_);
        core_.init(hw_.adc(), hw_.dac(), hw_.gpio());
        app_->init();
    }

    void start(uint32_t interval_us) {
        core_interval_us_ = interval_us;
        reset_profiling();
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
        const uint32_t display_start_cycles = current_cycle_count();
        hw_.display()->flush();
        const uint32_t display_flush_elapsed_cycles = current_cycle_count() - display_start_cycles;

        const uint32_t dac_flush_start_cycles = current_cycle_count();
        hw_.dac()->flush();
        const uint32_t dac_flush_elapsed_cycles = current_cycle_count() - dac_flush_start_cycles;

        const uint32_t display_update_start_cycles = current_cycle_count();
        hw_.display()->update();
        const uint32_t display_update_elapsed_cycles = current_cycle_count() - display_update_start_cycles;
        const uint32_t display_elapsed_cycles = display_flush_elapsed_cycles + display_update_elapsed_cycles;

        const uint32_t scan_start_cycles = current_cycle_count();
        core_.isr_cycle();
        const uint32_t scan_elapsed_cycles = current_cycle_count() - scan_start_cycles;

        const uint32_t marshal_start_cycles = current_cycle_count();
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
        const uint32_t marshal_elapsed_cycles = current_cycle_count() - marshal_start_cycles;

        AudioOut out{};
        const uint32_t app_start_cycles = current_cycle_count();
        app_->audio_callback(in, out);
        const uint32_t app_elapsed_cycles = current_cycle_count() - app_start_cycles;

        const uint32_t output_start_cycles = current_cycle_count();
        for (int i = 0; i < 4; ++i) {
            hw_.dac()->write(i, out.cv[i]);
        }
        const uint32_t output_elapsed_cycles = current_cycle_count() - output_start_cycles;

        if (timing_pin_ != kNoTimingPin) {
            digitalWriteFast(timing_pin_, LOW);
        }

        const uint32_t elapsed_cycles = current_cycle_count() - start_cycles;
        update_average(isr_avg_cycles_, elapsed_cycles);
        update_average(isr_display_avg_cycles_, display_elapsed_cycles);
        update_average(isr_dac_flush_avg_cycles_, dac_flush_elapsed_cycles);
        update_average(isr_scan_avg_cycles_, scan_elapsed_cycles);
        update_average(isr_marshal_avg_cycles_, marshal_elapsed_cycles);
        update_average(isr_app_avg_cycles_, app_elapsed_cycles);
        update_average(isr_output_avg_cycles_, output_elapsed_cycles);
    }

    static void update_average(volatile uint32_t& average_cycles, uint32_t sample_cycles) {
        int32_t avg_cycles = static_cast<int32_t>(average_cycles);
        avg_cycles += (static_cast<int32_t>(sample_cycles) - avg_cycles) >> kIsrAverageShift;
        average_cycles = static_cast<uint32_t>(avg_cycles);
    }

    void reset_profiling() {
        isr_avg_cycles_ = 0;
        isr_display_avg_cycles_ = 0;
        isr_dac_flush_avg_cycles_ = 0;
        isr_scan_avg_cycles_ = 0;
        isr_marshal_avg_cycles_ = 0;
        isr_app_avg_cycles_ = 0;
        isr_output_avg_cycles_ = 0;
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
    volatile uint32_t isr_display_avg_cycles_ = 0;
    volatile uint32_t isr_dac_flush_avg_cycles_ = 0;
    volatile uint32_t isr_scan_avg_cycles_ = 0;
    volatile uint32_t isr_marshal_avg_cycles_ = 0;
    volatile uint32_t isr_app_avg_cycles_ = 0;
    volatile uint32_t isr_output_avg_cycles_ = 0;
};

} // namespace oc