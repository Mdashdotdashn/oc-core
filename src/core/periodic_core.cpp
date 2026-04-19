#include "oc/core/periodic_core.h"
#include "adc.h"
#include "gpio.h"

namespace oc::core {

void PeriodicCore::init(ADCImpl* adc, GPIOImpl* gpio) {
    adc_   = adc;
    gpio_  = gpio;
    state_ = {};
}

void PeriodicCore::isr_cycle() {
    adc_->scan();
    gpio_->scan();

    for (int i = 0; i < 4; ++i) {
        state_.inputs.cv[i]     = adc_->get_calibrated(i);
        state_.inputs.cv_raw[i] = adc_->get_smoothed(i);
        state_.inputs.gate[i]   = gpio_->read_input(i);
    }
    state_.inputs.edges = gpio_->get_edge_mask();

    ++state_.tick;
}

} // namespace oc::core

