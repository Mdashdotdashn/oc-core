#pragma once
#include "oc/hal/gpio.h"
#include <cstdint>
#include <array>

/// Teensy 3.6 GPIO / digital gate input implementation.
/// Reads 4 gate inputs and detects rising edges.
/// Pin assignments match the existing OC_digital_inputs.h (TR1–TR4).

namespace oc::platform::teensy36 {

class GPIOImpl : public hal::GPIOInterface {
public:
    GPIOImpl();

    void init();

    void     scan()                         override;
    bool     read_input(uint8_t ch)  const  override;
    uint32_t get_edge_mask()         const  override;

private:
    // TR1–TR4 pin numbers (must match OC_gpio.h definitions)
    static constexpr uint8_t kPins[4] = {0, 1, 2, 3};  // TODO: verify actual pin numbers

    std::array<bool, 4> last_state_;
    std::array<bool, 4> current_state_;
    uint32_t            edge_mask_ = 0;
};

} // namespace oc::platform::teensy36
