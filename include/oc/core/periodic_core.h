#pragma once
#include <cstdint>
#include <array>
#include "oc/hal/adc.h"
#include "oc/hal/dac.h"
#include "oc/hal/gpio.h"

/// oc-core: PeriodicCore
///
/// Coordinates the hardware scan and state capture for each ISR cycle.
/// Not user-facing — the user interacts with AudioIn/AudioOut in audio_callback().
///
/// Call isr_cycle() at the top of every audio ISR. It scans ADC and GPIO,
/// then populates the internal CoreState. The audio ISR reads the state
/// via get_state() to build the AudioIn buffer before calling audio_callback().

namespace oc::core {

/// Immutable snapshot of all hardware inputs for one ISR cycle.
/// Captured by isr_cycle() and read by the audio ISR to build AudioIn.
struct CoreState {
    uint32_t tick;                     ///< Monotonically increasing ISR tick counter

    struct {
        std::array<int32_t,  4> cv;    ///< Calibrated ADC values
        std::array<uint32_t, 4> cv_raw; ///< Smoothed raw 12-bit ADC values
        std::array<bool,     4> gate;  ///< Gate input logic levels
        uint32_t                edges; ///< Rising-edge bitmask (cleared each cycle)
    } inputs;
};

/// Lightweight coordinator between HAL devices and the audio ISR.
/// Holds pointers to the three input/output HAL devices registered at init().
class PeriodicCore {
public:
    PeriodicCore();

    /// Register HAL device implementations.
    /// Must be called from main() before starting the timer.
    void init(hal::ADCInterface* adc,
              hal::DACInterface* dac,
              hal::GPIOInterface* gpio);

    /// Advance one audio cycle: scan ADC + GPIO, update CoreState, increment tick.
    /// Call this at the very start of the audio ISR callback.
    void isr_cycle();

    /// Access the most recently captured hardware state.
    /// Safe to read from the audio ISR after isr_cycle() returns.
    const CoreState& get_state() const { return state_; }

    uint32_t ticks() const { return state_.tick; }

private:
    hal::ADCInterface* adc_  = nullptr;
    hal::DACInterface* dac_  = nullptr;
    hal::GPIOInterface* gpio_ = nullptr;

    CoreState state_{};
};

} // namespace oc::core
