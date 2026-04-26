#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <ADC.h>

/// Teensy ADC implementation.
/// Round-robin channel scanning with exponential smoothing.
///
/// Traits must provide:
///   static constexpr int     kCount;
///   static constexpr uint8_t kPins[kCount];  // analog input pins

namespace platform {

namespace {
constexpr size_t kCalibrationPointCount = 8;
constexpr int kCalibrationVoltages[kCalibrationPointCount] = {-3, -2, -1, 0, 1, 2, 3, 4};
}

template <typename Traits>
class CVInputs final {
public:
    CVInputs() {
        raw_.fill(0);
        smoothed_accumulator_.fill(0);
        smoothed_value_.fill(0);
        calibrated_.fill(0);
        for (auto& channel_points : points_) {
            channel_points.fill(2048);
        }
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
            const uint32_t smoothed = smoothed_accumulator_[current_channel_] >> kSmoothing;
            smoothed_value_[current_channel_] = smoothed;
            calibrated_[current_channel_] = interpolate_mv(current_channel_, smoothed);
        }
        current_channel_ = (current_channel_ + 1) % Traits::kCount;
        adc_.startSingleRead(Traits::kPins[current_channel_], ADC_0);
    }

    uint32_t read_raw(uint8_t ch)     const { return raw_[ch]; }
    uint32_t get_smoothed(uint8_t ch) const { return smoothed_value_[ch]; }
    int32_t get_calibrated(uint8_t ch) const { return calibrated_[ch]; }

    void set_calibration_points(uint8_t channel, const uint16_t* points, size_t count) {
        if (channel >= Traits::kCount) {
            return;
        }
        const size_t point_count =
            count < kCalibrationPointCount ? count : kCalibrationPointCount;
        for (size_t i = 0; i < point_count; ++i) {
            points_[channel][i] = points[i];
        }
        calibrated_[channel] = interpolate_mv(channel, smoothed_value_[channel]);
    }

private:
    static constexpr int kSmoothing = 4;  ///< Exponential moving average factor (2^N)

    int32_t interpolate_mv(uint8_t channel, uint32_t raw) const {
        const auto& points = points_[channel];

        if (raw > static_cast<uint32_t>(points[0])) {
            const int32_t segment = static_cast<int32_t>(points[0]) - static_cast<int32_t>(points[1]);
            if (segment <= 0) {
                return kCalibrationVoltages[0] * 1000;
            }
            return kCalibrationVoltages[0] * 1000
                   - (static_cast<int32_t>(raw) - static_cast<int32_t>(points[0])) * 1000 / segment;
        }

        if (raw < static_cast<uint32_t>(points[kCalibrationPointCount - 1])) {
            const size_t last = kCalibrationPointCount - 1;
            const int32_t segment = static_cast<int32_t>(points[last - 1]) - static_cast<int32_t>(points[last]);
            if (segment <= 0) {
                return kCalibrationVoltages[last] * 1000;
            }
            const int32_t delta = static_cast<int32_t>(points[last]) - static_cast<int32_t>(raw);
            return kCalibrationVoltages[last] * 1000 + delta * 1000 / segment;
        }

        for (size_t i = 0; i + 1 < kCalibrationPointCount; ++i) {
            if (raw <= static_cast<uint32_t>(points[i]) && raw >= static_cast<uint32_t>(points[i + 1])) {
                const int32_t segment = static_cast<int32_t>(points[i]) - static_cast<int32_t>(points[i + 1]);
                if (segment == 0) {
                    return kCalibrationVoltages[i] * 1000;
                }
                const int32_t delta = static_cast<int32_t>(points[i]) - static_cast<int32_t>(raw);
                const int32_t delta_mv =
                    (kCalibrationVoltages[i + 1] - kCalibrationVoltages[i]) * 1000;
                return kCalibrationVoltages[i] * 1000 + delta * delta_mv / segment;
            }
        }

        // If point ordering is imperfect, fall back to nearest captured point
        // instead of collapsing to 0V.
        size_t nearest = 0;
        uint32_t best_diff =
            raw > static_cast<uint32_t>(points[0])
                ? raw - static_cast<uint32_t>(points[0])
                : static_cast<uint32_t>(points[0]) - raw;

        for (size_t i = 1; i < kCalibrationPointCount; ++i) {
            const uint32_t point = static_cast<uint32_t>(points[i]);
            const uint32_t diff = raw > point ? (raw - point) : (point - raw);
            if (diff < best_diff) {
                best_diff = diff;
                nearest = i;
            }
        }

        return kCalibrationVoltages[nearest] * 1000;
    }

    ::ADC   adc_;
    uint8_t current_channel_ = 0;
    std::array<uint32_t, Traits::kCount> raw_{};
    std::array<uint32_t, Traits::kCount> smoothed_accumulator_{};
    std::array<uint32_t, Traits::kCount> smoothed_value_{};
    std::array<int32_t, Traits::kCount> calibrated_{};
    std::array<std::array<uint16_t, kCalibrationPointCount>, Traits::kCount> points_{};
};

} // namespace platform
