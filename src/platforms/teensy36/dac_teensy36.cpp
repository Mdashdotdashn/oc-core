#include "dac_teensy36.h"
// TODO: include the appropriate SPI/DAC chip header once we wire in the
// low-level SPI routines from ArticCircle/src/drivers or a standalone copy.
// For now this is a stub to establish the interface.

namespace oc::platform::teensy36 {

DACImpl::DACImpl() {
    staged_.fill(0);
}

void DACImpl::init() {
    // TODO: SPI.begin(), set CS pin, send reset command to DAC chip
}

void DACImpl::write(uint8_t channel, uint16_t value) {
    staged_[channel] = value;
}

void DACImpl::write_all(const uint16_t values[4]) {
    for (int i = 0; i < 4; ++i) {
        staged_[i] = values[i];
    }
}

void DACImpl::flush() {
    // TODO: perform SPI burst transfer of all 4 staged_ values
    // to match the existing set8565_CH{A,B,C,D} + SPI_init protocol.
}

uint16_t DACImpl::get_last_value(uint8_t channel) const {
    return staged_[channel];
}

} // namespace oc::platform::teensy36
