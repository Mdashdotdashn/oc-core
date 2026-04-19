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
    // Concrete type aliases for zero-overhead static dispatch in Runtime/PeriodicCore.
    using AType    = CVInputs;
    using DType    = CVOutputs;
    using GType    = TriggerInputs;
    using BType    = Buttons;
    using EType    = Encoders;
    using DispType = Display;

public:
    void init_base() {
        // SPI0 bus (shared by CVOutputs and OLED) must be initialized before either device.
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
        display_.set_offset(calibration_data.display_offset);
    }

    CVInputs&      adc_impl()      { return adc_;      }
    CVOutputs&      dac_impl()      { return dac_;      }
    TriggerInputs&     gpio_impl()     { return gpio_;     }
    Buttons&  buttons_impl()  { return buttons_;  }
    Encoders& encoders_impl() { return encoders_; }
    Display&  display_impl()  { return display_;  }
    Timer&    timer_impl()    { return timer_;    }
    Storage&  storage_impl()  { return storage_;  }


private:
    CVInputs      adc_;
    CVOutputs      dac_;
    TriggerInputs     gpio_;
    Buttons  buttons_;
    Encoders encoders_;
    Display  display_;
    Timer    timer_;
    Storage  storage_;
};

} // namespace platform
