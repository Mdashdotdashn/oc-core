#pragma once
#include <cstdint>
#include <array>

// Forward-declare concrete hardware types (defined in src/adc.h, src/gpio.h).
namespace oc { class ADCImpl; class GPIOImpl; }

/// oc-core: PeriodicCore
///
/// Coordinates the hardware scan and state capture for each ISR cycle.
/// Holds concrete pointers to ADCImpl and GPIOImpl — no virtual dispatch.

namespace oc::core {

/// Immutable snapshot of all hardware inputs for one ISR cycle.
struct CoreState {
    uint32_t tick;

    struct {
        std::array<int32_t,  4> cv;
        std::array<uint32_t, 4> cv_raw;
        std::array<bool,     4> gate;
        uint32_t                edges;
    } inputs;
};

class PeriodicCore {
public:
    PeriodicCore() = default;

    void init(ADCImpl* adc, GPIOImpl* gpio);

    /// Advance one audio cycle: scan ADC + GPIO, update CoreState, increment tick.
    void isr_cycle();

    const CoreState& get_state() const { return state_; }
    uint32_t ticks() const { return state_.tick; }

private:
    ADCImpl*  adc_  = nullptr;
    GPIOImpl* gpio_ = nullptr;
    CoreState state_{};
};

} // namespace oc::core


