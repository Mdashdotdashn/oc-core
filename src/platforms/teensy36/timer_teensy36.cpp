#include "timer_teensy36.h"
#include <IntervalTimer.h>

namespace oc::platform::teensy36 {

namespace {
    IntervalTimer core_timer_;
}

void TimerImpl::start(uint32_t interval_us, hal::ISRHandler handler) {
    core_timer_.begin(handler, interval_us);
    core_timer_.priority(128);  // Match existing O&C ISR priority
}

void TimerImpl::stop() {
    core_timer_.end();
}

} // namespace oc::platform::teensy36
