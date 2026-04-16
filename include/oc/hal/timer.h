#pragma once
#include <cstdint>

/// Ornament & Crime Hardware Abstraction Layer
/// Timer Interface
///
/// Registers and starts the periodic ISR that drives the audio callback.
/// The callback is a plain function pointer (not std::function) to ensure
/// it is always ISR-safe and has zero overhead in the hot path.

namespace oc::hal {

using ISRHandler = void (*)();  // plain function pointer — safe in ISR context

class TimerInterface {
public:
    virtual ~TimerInterface() = default;

    /// Start the periodic timer and register the ISR handler.
    /// interval_us: period in microseconds (e.g. 100 for 10 kHz)
    /// handler: raw function pointer called from hardware interrupt context.
    virtual void start(uint32_t interval_us, ISRHandler handler) = 0;

    /// Start a second periodic timer for lower-rate UI/input housekeeping.
    /// Typical use: buttons and encoders at ~1 kHz.
    virtual void start_ui(uint32_t interval_us, ISRHandler handler) = 0;

    /// Stop the timer.
    virtual void stop() = 0;

    /// Stop the UI timer.
    virtual void stop_ui() = 0;
};

} // namespace oc::hal
