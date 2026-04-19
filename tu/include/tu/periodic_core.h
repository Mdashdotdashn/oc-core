#pragma once
#include <cstdint>
#include <array>
#include "tu/calibration.h"
#include "tu/platform_traits.h"
#include "platform/cv_inputs.h"
#include "platform/trigger_inputs.h"

namespace tu::core {

/// Immutable snapshot of all Temps Utile hardware inputs for one ISR cycle.
struct CoreState {
    uint32_t tick;

    struct {
        std::array<int32_t,  4> cv;         ///< Calibrated ADC values
        std::array<uint32_t, 4> cv_raw;     ///< Smoothed raw 12-bit ADC values
        std::array<bool,     2> gate;        ///< Gate levels for TR1, TR2
        uint32_t                edges;       ///< Rising-edge bitmask
    } inputs;
};

/// Coordinates hardware scan and state capture for each ISR cycle.
/// Holds concrete platform pointers — no virtual dispatch.
class PeriodicCore {
public:
    PeriodicCore() = default;

    /// Must be called once from main() before starting the timer.
    void init(platform::CVInputs<tu::CVInputTraits>*          adc,
              platform::TriggerInputs<tu::TriggerInputTraits>* gpio) {
        adc_  = adc;
        gpio_ = gpio;
        state_ = {};
    }

    /// Advance one audio cycle: scan ADC + triggers, update CoreState, increment tick.
    void isr_cycle() {
        adc_->scan();
        gpio_->scan();

        for (int i = 0; i < 4; ++i) {
            const uint32_t smoothed    = adc_->get_smoothed(i);
            state_.inputs.cv_raw[i]    = smoothed;
            state_.inputs.cv[i]        =
                static_cast<int32_t>(calibration::data().adc_offset[i])
                - static_cast<int32_t>(smoothed);
        }
        for (int i = 0; i < 2; ++i) {
            state_.inputs.gate[i] = gpio_->read_input(i);
        }
        state_.inputs.edges = gpio_->get_edge_mask();

        ++state_.tick;
    }

    const CoreState& get_state() const { return state_; }
    uint32_t ticks() const { return state_.tick; }

private:
    platform::CVInputs<tu::CVInputTraits>*           adc_  = nullptr;
    platform::TriggerInputs<tu::TriggerInputTraits>* gpio_ = nullptr;
    CoreState state_{};
};

} // namespace tu::core
