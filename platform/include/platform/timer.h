#pragma once
#include <cstdint>

using ISRHandler = void (*)();

/// Teensy 3.6 Timer implementation.
/// Wraps Teensy's IntervalTimer to register the audio ISR callback.

namespace platform {

class Timer final {
public:
    void start(uint32_t interval_us, ISRHandler handler);
    void start_ui(uint32_t interval_us, ISRHandler handler);
    void stop();
    void stop_ui();
};

} // namespace platform
