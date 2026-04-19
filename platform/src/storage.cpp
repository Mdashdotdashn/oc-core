#include "platform/storage.h"
#include <EEPROM.h>

namespace platform {

void Storage::read(uint32_t address, void* data, uint32_t size) {
    uint8_t* p = static_cast<uint8_t*>(data);
    for (uint32_t i = 0; i < size; ++i) {
        p[i] = EEPROM.read(address + i);
    }
}

void Storage::write(uint32_t address, const void* data, uint32_t size) {
    const uint8_t* p = static_cast<const uint8_t*>(data);
    for (uint32_t i = 0; i < size; ++i) {
        EEPROM.update(address + i, p[i]);  // update() skips writes for unchanged bytes
    }
}

uint32_t Storage::capacity() const {
    return EEPROM.length();
}

} // namespace platform
