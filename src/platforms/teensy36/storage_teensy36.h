#pragma once
#include "oc/hal/storage.h"
#include <EEPROM.h>

/// Teensy 3.6 Storage implementation.
/// Wraps the Teensy EEPROM library for persistent calibration storage.

namespace oc::platform::teensy36 {

class StorageImpl : public hal::StorageInterface {
public:
    void     read(uint32_t address, void* data, uint32_t size)       override;
    void     write(uint32_t address, const void* data, uint32_t size) override;
    uint32_t capacity() const                                         override;
};

} // namespace oc::platform::teensy36
