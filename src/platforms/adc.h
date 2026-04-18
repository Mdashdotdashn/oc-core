#pragma once
#include <cstdint>
#include <array>

/// Teensy 3.6 ADC implementation.
/// Wraps the Teensy ADC library with round-robin channel scanning,
/// simple exponential smoothing, and calibration offset support.
/// Reference implementation adapted from ArticCircle/OC_ADC.cpp.

namespace oc::platform {

class ADCImpl final {
public:
    ADCImpl();

    void init();

    void     scan();
    uint32_t read_raw(uint8_t ch)      const;
    uint32_t get_smoothed(uint8_t ch)  const;
    int32_t  get_calibrated(uint8_t ch) const;

    void set_calibration_offset(uint8_t channel, uint16_t offset);

private:
    static constexpr int   kSmoothing = 4;  ///< Exponential moving average factor (2^N)
    // CV input pins, verified against ArticCircle/OC_gpio.h (CV1=19, CV2=18, CV3=20, CV4=17)
    static constexpr uint8_t kPins[4] = {19, 18, 20, 17};

    uint8_t  current_channel_ = 0;
    std::array<uint32_t, 4> raw_;
    std::array<uint32_t, 4> smoothed_accumulator_;
    std::array<uint32_t, 4> smoothed_value_;
    std::array<int32_t, 4> calibrated_;
    std::array<uint16_t, 4> offsets_;
};

} // namespace oc::platform
