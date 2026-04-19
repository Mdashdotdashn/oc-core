#pragma once
#include <cstdint>
#include <array>

/// Teensy 3.2 ADC implementation.
/// Wraps the Teensy ADC library with round-robin channel scanning,
/// simple exponential smoothing, and calibration offset support.
/// Reference implementation adapted from ArticCircle/OC_ADC.cpp.

namespace platform {

class CVInputs final {
public:
    CVInputs();

    void init();

    void     scan();
    uint32_t read_raw(uint8_t ch)      const;
    uint32_t get_smoothed(uint8_t ch)  const;

private:
    static constexpr int   kSmoothing = 4;  ///< Exponential moving average factor (2^N)
    // CV input pins, verified against ArticCircle/OC_gpio.h (CV1=19, CV2=18, CV3=20, CV4=17)
    static constexpr uint8_t kPins[4] = {19, 18, 20, 17};

    uint8_t  current_channel_ = 0;
    std::array<uint32_t, 4> raw_;
    std::array<uint32_t, 4> smoothed_accumulator_;
    std::array<uint32_t, 4> smoothed_value_;
};

} // namespace platform
