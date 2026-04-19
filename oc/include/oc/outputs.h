#pragma once

#include <array>
#include <cstdint>

#include "oc/calibration.h"

namespace oc {

/// Output values to drive from one audio ISR cycle.
struct Outputs {
    /// DAC output values for each channel (0–65535, 16-bit).
    /// Maps to 0–10V on the O&C hardware (after calibration).
    std::array<uint16_t, 4> cv;

    /// Set a channel by calibrated voltage (-3V to +6V).
    /// Uses the persisted DAC calibration table loaded at startup.
    void set_cv(uint8_t ch, float volts) {
        cv[ch] = calibration::volts_to_dac(ch, volts);
    }

    /// Set a channel by voltage using explicit linear conversion constants.
    /// This bypasses the persisted calibration table and is kept for tests,
    /// experiments, and manual bring-up.
    void set_cv(uint8_t ch, float volts, float zero_code, float codes_per_volt) {
        const float raw = zero_code + volts * codes_per_volt;
        if (raw <= 0.0f)     { cv[ch] = 0;     return; }
        if (raw >= 65535.0f) { cv[ch] = 65535; return; }
        cv[ch] = static_cast<uint16_t>(raw);
    }
};

} // namespace oc
