#pragma once
#include <cstdint>
#include <EEPROM.h>

/// Teensy 3.6 Storage implementation.
/// Wraps the Teensy EEPROM library for persistent calibration storage.

namespace oc {

class StorageImpl final {
public:
    void     read(uint32_t address, void* data, uint32_t size);
    void     write(uint32_t address, const void* data, uint32_t size);
    uint32_t capacity() const;
};

} // namespace oc
