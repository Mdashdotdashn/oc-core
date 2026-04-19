#pragma once
#include <cstdint>

/// Teensy 3.6 GPIO / digital gate input implementation.
/// Reads 4 gate inputs and detects rising edges.
/// Pin assignments match the existing OC_digital_inputs.h (TR1–TR4).

namespace oc::platform::teensy32 {

class GPIOImpl final {
public:
    GPIOImpl();

    void init();

    void     scan();
    bool     read_input(uint8_t ch)  const;
    uint32_t get_edge_mask()         const;

private:
    // TR1-TR4 gate input pins (verified against ArticCircle/OC_gpio.h)
    static constexpr uint8_t kPins[4] = {0, 1, 2, 3};  // TR1=0, TR2=1, TR3=2, TR4=3

    uint8_t  last_state_mask_ = 0;
    uint8_t  current_state_mask_ = 0;
    uint32_t edge_mask_ = 0;
};

} // namespace oc::platform::teensy32
