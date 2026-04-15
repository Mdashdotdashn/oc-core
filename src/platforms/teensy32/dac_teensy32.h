#pragma once
#include "oc/hal/dac.h"
#include <cstdint>
#include <array>

/// Teensy 3.6 DAC implementation.
/// Drives the MCP48x2/AD5668-style DAC via the hardware SPI bus,
/// matching the protocol used by the existing OC_DAC.cpp driver.

namespace oc::platform::teensy32 {

class DACImpl : public hal::DACInterface {
public:
    DACImpl();

    void init();

    void     write(uint8_t channel, uint16_t value)     override;
    void     write_all(const uint16_t values[4])        override;
    void     flush()                                    override;
    uint16_t get_last_value(uint8_t channel) const      override;

private:
    std::array<uint16_t, 4> staged_;
};

} // namespace oc::platform::teensy32
