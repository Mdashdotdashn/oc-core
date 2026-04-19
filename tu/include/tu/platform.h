#pragma once
#include "platform/cv_inputs.h"
#include "platform/trigger_inputs.h"
#include "platform/trigger_outputs.h"
#include "platform/buttons.h"
#include "platform/encoders.h"
#include "platform/display.h"
#include "platform/spi0_init.h"
#include "platform/timer.h"
#include "platform/storage.h"
#include "tu/calibration.h"
#include "tu/platform_traits.h"

/// Temps Utile hardware platform bundle.
///
/// Owns one concrete instance of each hardware driver.
/// Call init_all() once from main() before starting the timer.

namespace platform {

class TUHardwarePlatform {
public:
    void init_base() {
        // SPI0 bus (used only by SH1106 OLED on T_U; no SPI DAC).
        spi0_init();
        cv_inputs_.init();
        trigger_inputs_.init();
        trigger_outputs_.init();
        buttons_.init();
        encoders_.init();
    }

    void init_display() {
        display_.init(tu::DisplayTraits::kDC,
                      tu::DisplayTraits::kRST,
                      tu::DisplayTraits::kCS);
    }

    void init_all() {
        init_base();
        init_display();
    }

    void apply_calibration(const tu::calibration::CalibrationData& d) {
        display_.set_offset(d.display_offset);
    }

    auto& adc_impl()             { return cv_inputs_;       }
    auto& gpio_impl()            { return trigger_inputs_;  }
    auto& trigger_outputs_impl() { return trigger_outputs_; }
    auto& buttons_impl()         { return buttons_;         }
    auto& encoders_impl()        { return encoders_;        }
    auto& display_impl()         { return display_;         }
    auto& timer_impl()           { return timer_;           }
    auto& storage_impl()         { return storage_;         }

private:
    CVInputs<tu::CVInputTraits>              cv_inputs_;
    TriggerInputs<tu::TriggerInputTraits>    trigger_inputs_;
    TriggerOutputs<tu::TriggerOutputTraits>  trigger_outputs_;
    Buttons<tu::ButtonTraits>                buttons_;
    Encoders<tu::EncoderTraits>              encoders_;
    Display  display_;
    Timer    timer_;
    Storage  storage_;
};

} // namespace platform
