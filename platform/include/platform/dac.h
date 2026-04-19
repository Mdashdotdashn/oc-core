#pragma once
#include <cstdint>
#include <array>

/// Teensy 3.6 DAC implementation.
/// Drives the MCP48x2/AD5668-style DAC via the hardware SPI bus,
/// matching the protocol used by the existing OC_DAC.cpp driver.

namespace platform {

class DAC final {
public:
    DAC();

    void init();

    void     write(uint8_t channel, uint16_t value);
    void     write_all(const uint16_t values[4]);
    void     flush();
    uint16_t get_last_value(uint8_t channel) const;

private:
    std::array<uint16_t, 4> staged_;
};

} // namespace platform
