#include "platform/cv_outputs.h"
#include "platform/drivers/util_SPIFIFO.h"
#include <Arduino.h>

// DAC8565 hardware pin assignments (match OC_gpio.h: DAC_CS=10, DAC_RST=9)
static constexpr uint8_t kDacCS  = 10;
static constexpr uint8_t kDacRST = 9;

// SPI clock: (36 MHz / 2) * ((1+1)/2) = 18 MHz  (F_BUS=36 MHz on Teensy 3.2 @72 MHz)
// DAC8565 max SPI clock is 30 MHz; 18 MHz is well within spec.
#define DAC_SPICLOCK (SPI_CTAR_PBR(0) | SPI_CTAR_BR(0) | SPI_CTAR_DBR)

// DAC8565 24-bit command bytes for single-channel update (non-power-down):
//   Bits [23:22] = 0b00  (fixed)
//   Bits [21:20] = 0b01  (write + update selected DAC)
//   Bit  [19]    = 0     (fixed)
//   Bits [18:17] = channel select  00=A  01=B  10=C  11=D
//   Bit  [16]    = 0     (normal, not power-down)
//   Bits [15: 0] = 16-bit DAC value  (sent separately via write16)
static constexpr uint8_t kChanCmd[4] = {
    0b00010000,  // Channel A
    0b00010010,  // Channel B
    0b00010100,  // Channel C
    0b00010110,  // Channel D
};

namespace platform {

CVOutputs::CVOutputs() {
    staged_.fill(0);
}

void CVOutputs::init() {
    pinMode(kDacCS,  OUTPUT);
    pinMode(kDacRST, OUTPUT);

    // DAC8565 reset: pulse RST low then high to clear all internal registers.
    digitalWrite(kDacRST, LOW);
    delayMicroseconds(1);
    digitalWrite(kDacRST, HIGH);
    delayMicroseconds(1);

    // spi0_init() has already set MOSI/SCK with DSE and configured CTAR0/CTAR1.
    // SPIFIFO.begin() sets the hardware CS pin and the pcs selector; it writes
    // the same CTAR values so it's safe to call after spi0_init().
    SPIFIFO.begin(kDacCS, DAC_SPICLOCK, SPI_MODE0);

    // Drive all outputs to minimum on startup.
    staged_.fill(0);
    flush();
}

void CVOutputs::write(uint8_t channel, uint16_t value) {
    staged_[channel] = value;
}

void CVOutputs::write_all(const uint16_t values[4]) {
    for (int i = 0; i < 4; ++i)
        staged_[i] = values[i];
}

void CVOutputs::flush() {
    // Direct 1:1 mapping: staged_[0] → DAC channel A, [1] → B, [2] → C, [3] → D.
    // No permutation — used for diagnosing jack-to-channel wiring.
    for (int ch = 0; ch < 4; ++ch) {
        const uint16_t raw = 0xFFFFu - staged_[ch];
        SPIFIFO.write(kChanCmd[ch], SPI_CONTINUE);
        SPIFIFO.write16(raw);
        SPIFIFO.read();
        SPIFIFO.read();
    }
}

uint16_t CVOutputs::get_last_value(uint8_t channel) const {
    return staged_[channel];
}

} // namespace platform
