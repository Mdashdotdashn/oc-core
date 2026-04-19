#pragma once

#include <Arduino.h>
#include "tu/app.h"
#include "tu/calibration.h"
#include "tu/periodic_core.h"

namespace tu {

/// Runtime facade for Temps Utile.
///
/// Owns the hardware platform, PeriodicCore, and the validated ISR sequencing
/// for an Application instance. One instance may be active at a time.
template <typename Platform>
class Runtime {
public:
    static constexpr uint32_t kDefaultUiIntervalUs = 1000;
    static constexpr uint8_t  kNoTimingPin         = 0xFF;
    static constexpr uint8_t  kIsrAverageShift      = 4;

    void set_timing_pin(uint8_t pin) { timing_pin_ = pin; }
    void set_ui_interval_us(uint32_t us) { ui_interval_us_ = us; }

    uint32_t isr_average_us() const { return cycles_to_us(isr_avg_cycles_); }
    uint8_t  isr_load_percent() const { return cycles_to_load_percent(isr_avg_cycles_); }

    uint32_t cycles_to_us(uint32_t cycles) const {
        const uint32_t cpu = F_CPU / 1000000U;
        return cpu ? ((cycles + cpu / 2) / cpu) : 0;
    }

    uint8_t cycles_to_load_percent(uint32_t cycles) const {
        const uint32_t budget = isr_budget_cycles();
        if (!budget) return 0;
        const uint32_t pct = static_cast<uint32_t>(
            (static_cast<uint64_t>(cycles) * 100U + budget / 2) / budget);
        return static_cast<uint8_t>(pct > 100U ? 100U : pct);
    }

    /// Initialise hardware, load calibration, prepare PeriodicCore.
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
        core_.init(&hw_.adc_impl(), &hw_.gpio_impl());
    }

    /// Bind application and call its init(). Must be called after init().
    void begin(Application& app) {
        app_ = &app;
        app_->init();
    }

    void start(uint32_t interval_us) {
        core_interval_us_ = interval_us;
        isr_avg_cycles_   = 0;
        hw_.timer_impl().start(interval_us, isr_trampoline);
        if (ui_interval_us_ > 0) {
            hw_.timer_impl().start_ui(ui_interval_us_, ui_trampoline);
        }
    }

    void stop() {
        hw_.timer_impl().stop();
        hw_.timer_impl().stop_ui();
    }

    /// Call from main loop: run idle() and draw().
    void poll() {
        app_->idle();
        app_->draw(&hw_.display_impl());
    }

    auto& storage() { return hw_.storage_impl(); }

private:
    static void FASTRUN isr_trampoline() {
        if (active_instance_) active_instance_->isr();
    }

    static void FASTRUN ui_trampoline() {
        if (active_instance_) active_instance_->ui_service();
    }

    void FASTRUN ui_service() {
        hw_.buttons_impl().scan();
        hw_.encoders_impl().scan();
        ++ui_scan_generation_;
    }

    void FASTRUN isr() {
        const uint32_t start = current_cycle_count();

        if (timing_pin_ != kNoTimingPin) digitalWriteFast(timing_pin_, HIGH);

        // Apply outputs staged last tick, then start next OLED DMA page.
        hw_.display_impl().flush();
        hw_.trigger_outputs_impl().flush();
        hw_.display_impl().update();

        // Scan ADC + trigger inputs.
        core_.isr_cycle();

        // Marshal Inputs from CoreState + UI peripherals.
        const uint32_t ui_gen = ui_scan_generation_;
        const bool has_ui     = ui_gen != last_ui_gen_;
        last_ui_gen_          = ui_gen;

        const auto& st = core_.get_state();

        Inputs in;
        in.cv        = st.inputs.cv;
        in.cv_raw    = st.inputs.cv_raw;
        in.gate      = st.inputs.gate;
        in.gate_edges = st.inputs.edges;

        const uint8_t ui_mask = has_ui ? 0xFF : 0x00;

        const auto b0 = hw_.buttons_impl().get(0);
        in.buttons[0] = { b0.pressed, bool(b0.just_pressed & ui_mask), bool(b0.just_released & ui_mask) };
        const auto b1 = hw_.buttons_impl().get(1);
        in.buttons[1] = { b1.pressed, bool(b1.just_pressed & ui_mask), bool(b1.just_released & ui_mask) };

        const auto e0 = hw_.encoders_impl().get(0);
        in.encoders[0] = { static_cast<int8_t>(e0.delta & ui_mask), e0.click_pressed,
                           bool(e0.click_just_pressed & ui_mask), bool(e0.click_just_released & ui_mask) };
        const auto e1 = hw_.encoders_impl().get(1);
        in.encoders[1] = { static_cast<int8_t>(e1.delta & ui_mask), e1.click_pressed,
                           bool(e1.click_just_pressed & ui_mask), bool(e1.click_just_released & ui_mask) };

        // Run application.
        Outputs out{};
        app_->audio_callback(in, out);

        // Stage outputs for flush() next tick.
        for (int i = 0; i < 6; ++i) {
            hw_.trigger_outputs_impl().write(i, out.gates[i]);
        }
        if (out.analog) {
            hw_.trigger_outputs_impl().write_analog(3, out.analog);
        }

        if (timing_pin_ != kNoTimingPin) digitalWriteFast(timing_pin_, LOW);

        const uint32_t elapsed = current_cycle_count() - start;
        int32_t avg = static_cast<int32_t>(isr_avg_cycles_);
        avg += (static_cast<int32_t>(elapsed) - avg) >> kIsrAverageShift;
        isr_avg_cycles_ = static_cast<uint32_t>(avg);
    }

    static void enable_cycle_counter() {
#if defined(ARM_DEMCR) && defined(ARM_DWT_CTRL) && defined(ARM_DWT_CYCCNT)
        ARM_DEMCR     |= ARM_DEMCR_TRCENA;
        ARM_DWT_CTRL  |= ARM_DWT_CTRL_CYCCNTENA;
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
        return static_cast<uint32_t>(
            (static_cast<uint64_t>(F_CPU) * core_interval_us_) / 1000000ULL);
    }

    inline static Runtime* active_instance_ = nullptr;

    Platform      hw_{};
    core::PeriodicCore core_{};
    Application*  app_              = nullptr;
    uint8_t       timing_pin_       = kNoTimingPin;
    uint32_t      core_interval_us_ = 0;
    uint32_t      ui_interval_us_   = kDefaultUiIntervalUs;
    volatile uint32_t ui_scan_generation_ = 0;
    uint32_t      last_ui_gen_      = 0;
    volatile uint32_t isr_avg_cycles_ = 0;
};

} // namespace tu
