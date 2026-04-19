#pragma once
#include <cstdint>
#include <array>
#include <ADC.h>

/// Teensy ADC implementation.
/// Round-robin channel scanning with exponential smoothing.
///
/// Traits must provide:
///   static constexpr int     kCount;
///   static constexpr uint8_t kPins[kCount];  // analog input pins

namespace platform {

template <typename Traits>
class CVInputs final {
public:
    CVInputs() {
        raw_.fill(0);
        smoothed_accumulator_.fill(0);
        smoothed_value_.fill(0);
    }

    void init() {
        adc_.adc0->setAveraging(4);
        adc_.adc0->setResolution(12);
        adc_.adc0->setConversionSpeed(ADC_CONVERSION_SPEED::HIGH_SPEED);
        adc_.adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::HIGH_SPEED);
        adc_.startSingleRead(Traits::kPins[0], ADC_0);
    }

    void scan() {
        if (adc_.adc0->isComplete()) {
            const uint32_t raw = static_cast<uint32_t>(adc_.readSingle(ADC_0));
            raw_[current_channel_] = raw;
            smoothed_accumulator_[current_channel_] =
                smoothed_accumulator_[current_channel_] -
                (smoothed_accumulator_[current_channel_] >> kSmoothing) +
                raw;
            smoothed_value_[current_channel_] = smoothed_accumulator_[current_channel_] >> kSmoothing;
        }
        current_channel_ = (current_channel_ + 1) % Traits::kCount;
        adc_.startSingleRead(Traits::kPins[current_channel_], ADC_0);
    }

    uint32_t read_raw(uint8_t ch)     const { return raw_[ch]; }
    uint32_t get_smoothed(uint8_t ch) const { return smoothed_value_[ch]; }

private:
    static constexpr int kSmoothing = 4;  ///< Exponential moving average factor (2^N)

    ::ADC   adc_;
    uint8_t current_channel_ = 0;
    std::array<uint32_t, Traits::kCount> raw_{};
    std::array<uint32_t, Traits::kCount> smoothed_accumulator_{};
    std::array<uint32_t, Traits::kCount> smoothed_value_{};
};

} // namespace platform
