#pragma once
#include <cstdint>
#include <array>
#include "oc/calibration.h"
#include "platform/cv_inputs.h"
#include "platform/trigger_inputs.h"

/// oc-core: PeriodicCore
///
/// Coordinates the hardware scan and state capture for each ISR cycle.
/// Not user-facing — the user interacts with Inputs/Outputs in audio_callback().
///
/// Templated on concrete ADC and GPIO types to eliminate virtual dispatch in the ISR.
/// Call isr_cycle() at the top of every audio ISR. It scans ADC and GPIO,
/// then populates the internal CoreState. The audio ISR reads the state
/// via get_state() to build the Inputs buffer before calling audio_callback().

namespace oc::core {

/// Immutable snapshot of all hardware inputs for one ISR cycle.
/// Captured by isr_cycle() and read by the audio ISR to build Inputs.
struct CoreState {
    uint32_t tick;                     ///< Monotonically increasing ISR tick counter

    struct {
        std::array<int32_t,  4> cv;    ///< Calibrated ADC values
        std::array<uint32_t, 4> cv_raw; ///< Smoothed raw 12-bit ADC values
        std::array<bool,     4> gate;  ///< Gate input logic levels
        uint32_t                edges; ///< Rising-edge bitmask (cleared each cycle)
    } inputs;
};

/// Lightweight coordinator between hardware scan and the audio ISR.
/// Uses concrete platform types — no virtual dispatch.
class PeriodicCore {
public:
    PeriodicCore() = default;

    /// Register concrete HAL device implementations.
    /// Must be called from main() before starting the timer.
    void init(platform::CVInputs* adc, platform::TriggerInputs* gpio) {
        adc_  = adc;
        gpio_ = gpio;
        state_ = {};
    }

    /// Advance one audio cycle: scan ADC + GPIO, update CoreState, increment tick.
    /// All calls here resolve statically — no vtable lookup.
    void isr_cycle() {
        adc_->scan();
        gpio_->scan();

        for (int i = 0; i < 4; ++i) {
            const uint32_t smoothed = adc_->get_smoothed(i);
            state_.inputs.cv_raw[i] = smoothed;
            state_.inputs.cv[i] =
                static_cast<int32_t>(oc::calibration::data().adc.offset[i]) - static_cast<int32_t>(smoothed);
            state_.inputs.gate[i]   = gpio_->read_input(i);
        }
        state_.inputs.edges = gpio_->get_edge_mask();

        ++state_.tick;
    }

    /// Access the most recently captured hardware state.
    /// Safe to read from the audio ISR after isr_cycle() returns.
    const CoreState& get_state() const { return state_; }

    uint32_t ticks() const { return state_.tick; }

private:
    platform::CVInputs*      adc_  = nullptr;
    platform::TriggerInputs* gpio_ = nullptr;

    CoreState state_{};
};

} // namespace oc::core

