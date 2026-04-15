#pragma once
#include "oc/hal/timer.h"

/// Teensy 3.6 Timer implementation.
/// Wraps Teensy's IntervalTimer to register the audio ISR callback.

namespace oc::platform::teensy36 {

class TimerImpl : public hal::TimerInterface {
public:
    void start(uint32_t interval_us, hal::ISRHandler handler) override;
    void stop() override;
};

} // namespace oc::platform::teensy36
