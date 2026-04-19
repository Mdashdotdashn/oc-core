#pragma once
#include <cstdint>
#include <array>

// Forward-declare concrete storage type.
namespace platform { class Storage; }

namespace tu::calibration {

static constexpr uint32_t kMagic   = 0x54554331u;  // 'TUC1'
static constexpr uint16_t kVersion = 1;
static constexpr uint32_t kStorageAddress = 0;

static constexpr size_t kAdcChannelCount = 4;

/// Calibration blob for Temps Utile.
/// Simpler than OC: no SPI DAC, just ADC offsets + display offset.
struct CalibrationData {
    uint32_t magic   = kMagic;
    uint16_t version = kVersion;
    uint16_t size    = sizeof(CalibrationData);

    /// Per-channel ADC offset (raw 12-bit units at 0V input).
    /// Subtract raw value from offset to get a signed millivolt estimate.
    std::array<uint32_t, kAdcChannelCount> adc_offset = {2730, 2730, 2730, 2730};

    uint8_t display_offset = 2;
    uint8_t reserved[3]    = {};
};

// --- inline state & accessors (C++17 inline variables) ---

inline CalibrationData current_data;

inline CalibrationData make_default_data() {
    return CalibrationData{};
}

inline const CalibrationData& data() {
    return current_data;
}

inline CalibrationData& mutable_data() {
    return current_data;
}

inline bool is_valid(const CalibrationData& d) {
    return d.magic == kMagic
        && d.version == kVersion
        && d.size == sizeof(CalibrationData);
}

inline void reset_to_defaults() {
    current_data = make_default_data();
}

inline bool load(platform::Storage& storage) {
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

inline bool save(platform::Storage& storage) {
    if (storage.capacity() < sizeof(CalibrationData)) {
        return false;
    }
    current_data.magic   = kMagic;
    current_data.version = kVersion;
    current_data.size    = sizeof(CalibrationData);
    storage.write(kStorageAddress, &current_data, sizeof(current_data));
    return true;
}

template <typename Platform>
void initialize(Platform& hw) {
    if (!load(hw.storage_impl())) {
        reset_to_defaults();
    }
    hw.apply_calibration(data());
}

} // namespace tu::calibration
