#pragma once
#include "platform/cv_inputs.h"
#include "platform/buttons.h"
#include "platform/cv_outputs.h"
#include "platform/display.h"
#include "platform/encoders.h"
#include "platform/trigger_inputs.h"
#include "platform/spi0_init.h"
#include "platform/timer.h"
#include "platform/storage.h"
#include "oc/calibration.h"
#include "oc/platform_traits.h"

/// Teensy 3.2 hardware platform bundle.
///
/// Owns one concrete instance of each HAL device.
/// Call init_all() once from main() before starting the timer.
/// Then pass device pointers to PeriodicCore::init().
///
///   HardwarePlatform hw;
///   oc::core::PeriodicCore audio;
///
///   hw.init_all();
///   audio.init(hw.adc(), hw.dac(), hw.gpio());
///   hw.timer().start(100, my_isr);

namespace platform {

class HardwarePlatform {
public:
    void init_base() {
        // SPI0 bus (shared by CVOutputs and OLED) must be initialized before either device.
        // Sets MOSI/SCK drive strength, CTAR0 (8-bit) and CTAR1 (16-bit) at 18 MHz.
        spi0_init<oc::SpiTraits>();
        cv_inputs_.init();
        cv_outputs_.init();
        trigger_inputs_.init();
        buttons_.init();
        encoders_.init();
    }

    void init_display() {
        display_.init(oc::DisplayTraits::kDC, oc::DisplayTraits::kRST, oc::DisplayTraits::kCS);
    }

    void init_all() {
        init_base();
        init_display();
    }

    void apply_calibration(const oc::calibration::CalibrationData& calibration_data) {
        display_.set_offset(calibration_data.display_offset);
    }

    auto& adc_impl()      { return cv_inputs_;      }
    auto& dac_impl()      { return cv_outputs_;      }
    auto& gpio_impl()     { return trigger_inputs_;  }
    auto& buttons_impl()  { return buttons_;         }
    auto& encoders_impl() { return encoders_;        }
    auto& display_impl()  { return display_;         }
    auto& timer_impl()    { return timer_;           }
    auto& storage_impl()  { return storage_;         }


private:
    CVInputs<oc::CVInputTraits>       cv_inputs_;
    CVOutputs<oc::CVOutputTraits>     cv_outputs_;
    TriggerInputs<oc::TriggerInputTraits> trigger_inputs_;
    Buttons<oc::ButtonTraits>         buttons_;
    Encoders<oc::EncoderTraits>       encoders_;
    Display  display_;
    Timer    timer_;
    Storage  storage_;
};

} // namespace platform
