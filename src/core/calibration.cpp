#include "oc/calibration.h"
#include "platforms/storage.h"

namespace oc::calibration {

namespace {

constexpr uint16_t kDefaultPitchCvScale = 12 << 7;
constexpr uint16_t kDefaultAdcOffset = static_cast<uint16_t>(4096.0f * 0.6666667f);
constexpr uint8_t kDefaultDisplayOffset = 2;

constexpr std::array<uint16_t, kDacVoltagePointCount> kDefaultDacTable = {
    4890, 11443, 17997, 24551, 31104, 37658, 44211, 50765, 57318, 63871, 65535
};

CalibrationData current_data = make_default_data();

} // namespace

CalibrationData make_default_data() {
    CalibrationData calibration_data{};

    calibration_data.magic = kMagic;
    calibration_data.version = kVersion;
    calibration_data.size = sizeof(CalibrationData);

    for (auto& channel_table : calibration_data.dac.calibrated_octaves) {
        channel_table = kDefaultDacTable;
    }

    calibration_data.adc.offset.fill(kDefaultAdcOffset);
    calibration_data.adc.pitch_cv_scale = kDefaultPitchCvScale;
    calibration_data.adc.pitch_cv_offset = 0;

    calibration_data.display_offset = kDefaultDisplayOffset;
    calibration_data.reserved0[0] = 0;
    calibration_data.reserved0[1] = 0;
    calibration_data.reserved0[2] = 0;
    calibration_data.reserved1 = 0;

    return calibration_data;
}

const CalibrationData& data() {
    return current_data;
}

CalibrationData& mutable_data() {
    return current_data;
}

bool is_valid(const CalibrationData& calibration_data) {
    return calibration_data.magic == kMagic
        && calibration_data.version == kVersion
        && calibration_data.size == sizeof(CalibrationData);
}

void reset_to_defaults() {
    current_data = make_default_data();
}

bool load(platform::StorageImpl& storage) {
    if (storage.capacity() < sizeof(CalibrationData)) {
        return false;
    }

    CalibrationData persisted{};
    storage.read(kStorageAddress, &persisted, sizeof(persisted));
    if (!is_valid(persisted)) {
        return false;
    }

    current_data = persisted;
    return true;
}

bool save(platform::StorageImpl& storage) {
    if (storage.capacity() < sizeof(CalibrationData)) {
        return false;
    }

    current_data.magic = kMagic;
    current_data.version = kVersion;
    current_data.size = sizeof(CalibrationData);
    storage.write(kStorageAddress, &current_data, sizeof(current_data));
    return true;
}

uint16_t volts_to_dac(uint8_t channel, float volts) {
    if (channel >= kDacChannelCount) {
        return 0;
    }

    if (volts <= static_cast<float>(kDacVoltageMin)) {
        return current_data.dac.calibrated_octaves[channel][0];
    }
    if (volts >= static_cast<float>(kDacVoltageMax)) {
        return current_data.dac.calibrated_octaves[channel][kDacVoltageMax - kDacVoltageMin];
    }

    const float shifted = volts - static_cast<float>(kDacVoltageMin);
    const int lower_index = static_cast<int>(shifted);
    const float fractional = shifted - static_cast<float>(lower_index);

    const uint16_t lower = current_data.dac.calibrated_octaves[channel][lower_index];
    const uint16_t upper = current_data.dac.calibrated_octaves[channel][lower_index + 1];
    const float interpolated = lower + (upper - lower) * fractional;

    if (interpolated <= 0.0f) {
        return 0;
    }
    if (interpolated >= 65535.0f) {
        return 65535;
    }
    return static_cast<uint16_t>(interpolated + 0.5f);
}

uint16_t dac_value_at(uint8_t channel, size_t point_index) {
    if (channel >= kDacChannelCount || point_index >= kDacVoltagePointCount) {
        return 0;
    }
    return current_data.dac.calibrated_octaves[channel][point_index];
}

} // namespace oc::calibration