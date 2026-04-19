#pragma once
#include "adc.h"
#include "buttons.h"
#include "dac.h"
#include "display.h"
#include "encoders.h"
#include "gpio.h"
#include "spi0_init.h"
#include "timer.h"
#include "storage.h"
#include "oc/calibration.h"

/// Teensy 3.2 hardware bundle.
///
/// Owns one concrete instance of each hardware device.
/// Call init_all() once from main() before starting the timer.

namespace oc {

class Hardware {
public:
    void init_base() {
        spi0_init();
        adc_.init();
        dac_.init();
        gpio_.init();
        buttons_.init();
        encoders_.init();
    }

    void init_display() {
        display_.init();
    }

    void init_all() {
        init_base();
        init_display();
    }

    void apply_calibration(const calibration::CalibrationData& calibration_data) {
        for (size_t i = 0; i < calibration::kAdcChannelCount; ++i) {
            adc_.set_calibration_offset(i, calibration_data.adc.offset[i]);
        }
        display_.set_offset(calibration_data.display_offset);
    }

    /// Sample button state before the UI timer ISR has started.
    bool button_held_at_boot(uint8_t idx) {
        for (int i = 0; i < 8; ++i) buttons_.scan();
        return buttons_.get(idx).pressed;
    }

    ADCImpl&      adc_impl()      { return adc_;      }
    DACImpl&      dac_impl()      { return dac_;      }
    GPIOImpl&     gpio_impl()     { return gpio_;     }
    ButtonsImpl&  buttons_impl()  { return buttons_;  }
    EncodersImpl& encoders_impl() { return encoders_; }
    DisplayImpl&  display_impl()  { return display_;  }
    TimerImpl&    timer_impl()    { return timer_;    }
    StorageImpl&  storage_impl()  { return storage_;  }

private:
    ADCImpl      adc_;
    DACImpl      dac_;
    GPIOImpl     gpio_;
    ButtonsImpl  buttons_;
    EncodersImpl encoders_;
    DisplayImpl  display_;
    TimerImpl    timer_;
    StorageImpl  storage_;
};

} // namespace oc

