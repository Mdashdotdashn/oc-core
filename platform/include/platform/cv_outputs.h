#pragma once
#include <cstdint>
#include <array>
#include "platform/drivers/util_SPIFIFO.h"
#include <Arduino.h>

/// Teensy DAC output implementation (DAC8565 via SPI).
///
/// Traits must provide:
///   static constexpr int     kCount;   // number of output channels
///   static constexpr uint8_t kCSPin;   // SPI chip-select pin
///   static constexpr uint8_t kRSTPin;  // hardware reset pin

namespace platform {

template <typename Traits>
class CVOutputs final {
public:
    CVOutputs() { staged_.fill(0); }

    void init() {
        pinMode(Traits::kCSPin,  OUTPUT);
        pinMode(Traits::kRSTPin, OUTPUT);

        // DAC8565 reset: pulse RST low then high to clear all internal registers.
        digitalWrite(Traits::kRSTPin, LOW);
        delayMicroseconds(1);
        digitalWrite(Traits::kRSTPin, HIGH);
        delayMicroseconds(1);

        // spi0_init() has already configured CTAR0/CTAR1 at 18 MHz.
        SPIFIFO.begin(Traits::kCSPin, SPI_CTAR_PBR(0) | SPI_CTAR_BR(0) | SPI_CTAR_DBR, SPI_MODE0);

        staged_.fill(0);
        flush();
    }

    void write(uint8_t channel, uint16_t value) {
        staged_[channel] = value;
    }

    void write_all(const uint16_t* values) {
        for (int i = 0; i < Traits::kCount; ++i)
            staged_[i] = values[i];
    }

    void flush() {
        for (int ch = 0; ch < Traits::kCount; ++ch) {
            const uint16_t raw = 0xFFFFu - staged_[ch];
            SPIFIFO.write(kChanCmd[ch], SPI_CONTINUE);
            SPIFIFO.write16(raw);
            SPIFIFO.read();
            SPIFIFO.read();
        }
    }

    uint16_t get_last_value(uint8_t channel) const { return staged_[channel]; }

private:
    // DAC8565 24-bit command bytes: write + update selected DAC (non-power-down).
    // Bits [23:22]=00, [21:20]=01 (write+update), [19]=0, [18:17]=ch, [16]=0.
    static constexpr uint8_t kChanCmd[4] = {
        0b00010000,  // Channel A
        0b00010010,  // Channel B
        0b00010100,  // Channel C
        0b00010110,  // Channel D
    };

    std::array<uint16_t, Traits::kCount> staged_;
};

} // namespace platform
