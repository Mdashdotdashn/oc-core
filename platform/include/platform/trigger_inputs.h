#pragma once
#include <cstdint>
#include <Arduino.h>

/// Teensy digital gate input implementation.
/// Reads N gate inputs and detects rising edges.
///
/// Traits must provide:
///   static constexpr int     kCount;
///   static constexpr uint8_t kPins[kCount];  // active-low gate inputs

namespace platform {

template <typename Traits>
class TriggerInputs final {
public:
    void init() {
        for (int i = 0; i < Traits::kCount; ++i) {
            pinMode(Traits::kPins[i], INPUT_PULLUP);
        }
    }

    void scan() {
        uint8_t current = 0;
        for (int i = 0; i < Traits::kCount; ++i) {
            // Active-low: invert the raw pin state.
            current |= static_cast<uint8_t>(!digitalReadFast(Traits::kPins[i])) << i;
        }
        current_state_mask_ = current;
        edge_mask_ = static_cast<uint32_t>(current & ~last_state_mask_);
        last_state_mask_ = current;
    }

    bool     read_input(uint8_t ch)  const { return ((current_state_mask_ >> ch) & 0x1u) != 0; }
    uint32_t get_edge_mask()         const { return edge_mask_; }

private:
    uint8_t  last_state_mask_    = 0;
    uint8_t  current_state_mask_ = 0;
    uint32_t edge_mask_          = 0;
};

} // namespace platform
