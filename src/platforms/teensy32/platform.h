#pragma once
#include "adc_teensy32.h"
#include "buttons_teensy32.h"
#include "dac_teensy32.h"
#include "display_teensy32.h"
#include "encoders_teensy32.h"
#include "gpio_teensy32.h"
#include "spi0_init.h"
#include "timer_teensy32.h"
#include "storage_teensy32.h"
#include "oc/calibration.h"

/// Teensy 3.6 hardware platform bundle.
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

namespace oc::platform::teensy32 {

class HardwarePlatform {
public:
    // Concrete type aliases for zero-overhead static dispatch in Runtime/PeriodicCore.
    using AType    = ADCImpl;
    using DType    = DACImpl;
    using GType    = GPIOImpl;
    using BType    = ButtonsImpl;
    using EType    = EncodersImpl;
    using DispType = DisplayImpl;

public:
    void init_base() {
        // SPI0 bus (shared by DAC and OLED) must be initialized before either device.
        // Sets MOSI/SCK drive strength, CTAR0 (8-bit) and CTAR1 (16-bit) at 18 MHz.
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

    void apply_calibration(const oc::calibration::CalibrationData& calibration_data) {
        for (size_t i = 0; i < oc::calibration::kAdcChannelCount; ++i) {
            adc_.set_calibration_offset(i, calibration_data.adc.offset[i]);
        }
        display_.set_offset(calibration_data.display_offset);
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

} // namespace oc::platform::teensy32
