#pragma once
#include <cstdint>

/// Ornament & Crime Hardware Abstraction Layer
/// ADC (Analog-to-Digital Converter) Interface
///
/// Provides access to the 4 CV input channels sampled at the ISR rate (~10kHz).
/// Implementations must be ISR-safe; scan() is called from within the audio callback.

namespace oc::hal {

class ADCInterface {
public:
    virtual ~ADCInterface() = default;

    /// Advance the ADC scan by one step (called once per audio callback).
    /// Reads the result of the previous conversion and starts the next one.
    virtual void scan() = 0;

    /// Raw 12-bit ADC reading (0–4095), no smoothing or calibration.
    virtual uint32_t read_raw(uint8_t channel) const = 0;

    /// Smoothed raw reading (after internal averaging — still 0–4095 range).
    virtual uint32_t get_smoothed(uint8_t channel) const = 0;

    /// Calibrated value with offset applied (used for pitch CV inputs).
    /// Sign convention and scale matches the existing O&C calibration scheme.
    virtual int32_t get_calibrated(uint8_t channel) const = 0;

    static constexpr int kChannels = 4;
};

} // namespace oc::hal
