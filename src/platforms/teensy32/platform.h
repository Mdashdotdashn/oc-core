#pragma once
#include "adc_teensy32.h"
#include "dac_teensy32.h"
#include "gpio_teensy32.h"
#include "timer_teensy32.h"
#include "storage_teensy32.h"
#include "oc/hal/adc.h"
#include "oc/hal/dac.h"
#include "oc/hal/gpio.h"
#include "oc/hal/timer.h"
#include "oc/hal/storage.h"

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
    void init_all() {
        adc_.init();
        dac_.init();
        gpio_.init();
    }

    hal::ADCInterface*     adc()     { return &adc_;     }
    hal::DACInterface*     dac()     { return &dac_;     }
    hal::GPIOInterface*    gpio()    { return &gpio_;    }
    hal::TimerInterface*   timer()   { return &timer_;   }
    hal::StorageInterface* storage() { return &storage_; }

private:
    ADCImpl     adc_;
    DACImpl     dac_;
    GPIOImpl    gpio_;
    TimerImpl   timer_;
    StorageImpl storage_;
};

} // namespace oc::platform::teensy32
