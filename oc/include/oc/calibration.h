#pragma once

#include <cstddef>
#include <cstdint>
#include <array>

// Forward-declare the concrete storage type used on Teensy 3.2.
namespace platform { class Storage; }

namespace oc::calibration {

static constexpr uint32_t kMagic = 0x43414C31u;  // CAL1
static constexpr uint16_t kVersion = 2;
static constexpr uint32_t kStorageAddress = 0;

static constexpr size_t kAdcChannelCount = 4;
static constexpr size_t kDacChannelCount = 4;
static constexpr size_t kDacVoltagePointCount = 11;
static constexpr size_t kAdcCalibrationPointCount = 8;

// Capture points for ADC calibration, spanning the usable O_C CV input range.
static constexpr int kAdcCalibrationVoltages[kAdcCalibrationPointCount] = {-3, -2, -1, 0, 1, 2, 3, 4};

static constexpr int kDacVoltageMin = -3;
static constexpr int kDacVoltageMax = 6;

struct DACCalibrationData {
    std::array<std::array<uint16_t, kDacVoltagePointCount>, kDacChannelCount> calibrated_octaves;
};

struct ADCCalibrationData {
    std::array<std::array<uint16_t, kAdcCalibrationPointCount>, kAdcChannelCount> points;
};

struct CalibrationData {
    uint32_t magic;
    uint16_t version;
    uint16_t size;

    DACCalibrationData dac;
    ADCCalibrationData adc;

    uint8_t  display_offset;
    uint8_t  reserved0[3];
    uint32_t reserved1;
};

CalibrationData make_default_data();
const CalibrationData& data();
CalibrationData& mutable_data();
bool is_valid(const CalibrationData& calibration_data);
void reset_to_defaults();
bool load(platform::Storage& storage);
bool save(platform::Storage& storage);
uint16_t volts_to_dac(uint8_t channel, float volts);
uint16_t dac_value_at(uint8_t channel, size_t point_index);

template <typename Platform>
void initialize(Platform& hw) {
    if (!load(hw.storage_impl())) {
        reset_to_defaults();
    }
    hw.apply_calibration(data());
}

} // namespace oc::calibration