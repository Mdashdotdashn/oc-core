#include "timer.h"
#include <IntervalTimer.h>

namespace oc::platform {

namespace {
    IntervalTimer core_timer_;
    IntervalTimer ui_timer_;
}

void TimerImpl::start(uint32_t interval_us, ISRHandler handler) {
    core_timer_.begin(handler, interval_us);
    core_timer_.priority(128);  // Match existing O&C ISR priority
}

void TimerImpl::start_ui(uint32_t interval_us, ISRHandler handler) {
    ui_timer_.begin(handler, interval_us);
    ui_timer_.priority(160);  // Lower than core ISR; suitable for UI polling.
}

void TimerImpl::stop() {
    core_timer_.end();
}

void TimerImpl::stop_ui() {
    ui_timer_.end();
}

} // namespace oc::platform
