#pragma once
#include <cstdint>
#include <Arduino.h>
#include <kinetis.h>

/// Digital trigger output driver.
/// Writes N boolean gate channels via digitalWriteFast().
/// One channel may optionally be the Teensy's internal 12-bit DAC (A14).
///
/// Traits must provide:
///   static constexpr int     kCount;
///   static constexpr uint8_t kPins[kCount];   // GPIO output pins
///   static constexpr int     kDacChannel;      // index of internal DAC channel (-1 if none)

namespace platform {

template <typename Traits>
class TriggerOutputs final {
public:
    void init() {
        for (int i = 0; i < Traits::kCount; ++i) {
            if (i == Traits::kDacChannel) {
                // Enable and zero the Teensy internal DAC.
                SIM_SCGC2 |= SIM_SCGC2_DAC0;
                DAC0_C0 = DAC_C0_DACEN | DAC_C0_DACRFS;  // 3.3V reference
                *(int16_t *)&(DAC0_DAT0L) = 0;
            } else {
                pinMode(Traits::kPins[i], OUTPUT);
                digitalWriteFast(Traits::kPins[i], LOW);
            }
        }
    }

    /// Stage a boolean level for output channel ch.
    /// For the DAC channel this maps false→0V, true→full scale.
    void write(uint8_t ch, bool value) {
        staged_[ch] = value;
    }

    /// Stage a 12-bit analog value for the DAC channel (0–4095).
    /// Has no effect if kDacChannel < 0.
    void write_analog(uint8_t ch, uint16_t value) {
        if (ch == Traits::kDacChannel) {
            analog_staged_ = value & 0x0FFF;
            use_analog_     = true;
        }
    }

    /// Apply all staged values. Called once per ISR cycle.
    void flush() {
        for (int i = 0; i < Traits::kCount; ++i) {
            if (i == Traits::kDacChannel) {
                const uint16_t val = use_analog_ ? analog_staged_
                                                 : (staged_[i] ? 0x0FFF : 0x0000);
                *(int16_t *)&(DAC0_DAT0L) = static_cast<int16_t>(val);
            } else {
                digitalWriteFast(Traits::kPins[i], staged_[i] ? HIGH : LOW);
            }
        }
        use_analog_ = false;  // reset each cycle; app must set it every tick to keep it
    }

private:
    bool     staged_[Traits::kCount] = {};
    uint16_t analog_staged_ = 0;
    bool     use_analog_     = false;
};

} // namespace platform
