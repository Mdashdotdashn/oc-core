#pragma once
#include <cstdint>

/// Ornament & Crime Hardware Abstraction Layer
/// DAC (Digital-to-Analog Converter) Interface
///
/// Controls the 4 CV output channels (16-bit, 0–65535).
/// write() stages values; flush() commits them via SPI in a single transfer.
/// Both are called from within the audio callback ISR.

namespace oc::hal {

class DACInterface {
public:
    virtual ~DACInterface() = default;

    /// Stage a 16-bit value on a single output channel (0–3).
    /// Does not immediately send to hardware — call flush() after updating all channels.
    virtual void write(uint8_t channel, uint16_t value) = 0;

    /// Stage all 4 channels at once.
    virtual void write_all(const uint16_t values[4]) = 0;

    /// Commit staged values to hardware (SPI transfer, DMA trigger, etc.)
    /// Called at the end of each audio callback after all out.cv[] are set.
    virtual void flush() = 0;

    /// Read back the last value staged for a channel (not the hardware readback).
    virtual uint16_t get_last_value(uint8_t channel) const = 0;

    static constexpr int kChannels    = 4;
    static constexpr uint16_t kMaxVal = 65535;
};

} // namespace oc::hal
