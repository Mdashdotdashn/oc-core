#pragma once
#include <array>
#include <cstdint>
#include "oc/calibration.h"
#include "oc/platform_traits.h"
#include "platform/cv_inputs.h"
#include "platform/trigger_inputs.h"

namespace oc::core {

struct CoreState {
    uint32_t tick;

    struct {
        std::array<int32_t, 4> cv;
        std::array<uint32_t, 4> cv_raw;
        std::array<bool, 4> gate;
        uint32_t edges;
    } inputs;
};

class PeriodicCore {
public:
    PeriodicCore() = default;

    void init(platform::CVInputs<oc::CVInputTraits>* adc, platform::TriggerInputs<oc::TriggerInputTraits>* gpio) {
        adc_ = adc;
        gpio_ = gpio;
        state_ = {};
    }

    void isr_cycle() {
        adc_->scan();
        gpio_->scan();

        for (int i = 0; i < 4; ++i) {
            const uint32_t smoothed = adc_->get_smoothed(i);
            state_.inputs.cv_raw[i] = smoothed;
            state_.inputs.cv[i] = adc_->get_calibrated(i);
            state_.inputs.gate[i] = gpio_->read_input(i);
        }
        state_.inputs.edges = gpio_->get_edge_mask();

        ++state_.tick;
    }

    const CoreState& get_state() const { return state_; }
    uint32_t ticks() const { return state_.tick; }

private:
    platform::CVInputs<oc::CVInputTraits>* adc_ = nullptr;
    platform::TriggerInputs<oc::TriggerInputTraits>* gpio_ = nullptr;
    CoreState state_{};
};

} // namespace oc::core
