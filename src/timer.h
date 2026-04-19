#pragma once
#include <cstdint>

namespace oc {

using ISRHandler = void (*)();

class TimerImpl final {
public:
    void start(uint32_t interval_us, ISRHandler handler);
    void start_ui(uint32_t interval_us, ISRHandler handler);
    void stop();
    void stop_ui();
};

} // namespace oc
