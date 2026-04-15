#pragma once
#include <cstdint>

/// Ornament & Crime Hardware Abstraction Layer
/// Storage Interface (EEPROM / Flash)
///
/// Used exclusively for calibration data load/save at startup and shutdown.
/// Not called from the audio ISR path.

namespace oc::hal {

class StorageInterface {
public:
    virtual ~StorageInterface() = default;

    /// Read `size` bytes from `address` into `data`.
    virtual void read(uint32_t address, void* data, uint32_t size) = 0;

    /// Write `size` bytes from `data` to `address`.
    virtual void write(uint32_t address, const void* data, uint32_t size) = 0;

    /// Total usable storage in bytes.
    virtual uint32_t capacity() const = 0;
};

} // namespace oc::hal
