#include "oc/core/periodic_core.h"

namespace oc::core {

PeriodicCore::PeriodicCore() = default;

void PeriodicCore::init(hal::ADCInterface* adc,
                        hal::DACInterface* dac,
                        hal::GPIOInterface* gpio) {
    adc_  = adc;
    dac_  = dac;
    gpio_ = gpio;
    state_ = {};
}

void PeriodicCore::isr_cycle() {
    // 1. Advance hardware scans
    adc_->scan();
    gpio_->scan();

    // 2. Capture input snapshot
    for (int i = 0; i < 4; ++i) {
        state_.inputs.cv[i]     = adc_->get_calibrated(i);
        state_.inputs.cv_raw[i] = adc_->get_smoothed(i);
        state_.inputs.gate[i]   = gpio_->read_input(i);
    }
    state_.inputs.edges = gpio_->get_edge_mask();

    // 3. Increment tick counter
    ++state_.tick;
}

} // namespace oc::core
