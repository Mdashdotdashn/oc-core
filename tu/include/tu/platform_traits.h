#pragma once
#include <cstdint>

/// Hardware pin assignments for the Temps Utile (T_U) module, rev 1.
/// Pass these as template arguments to the corresponding platform:: classes.
///
/// Reference: TU_gpio.h (rev1 section, _TEMPS_UTILE_REV_0 NOT defined)

namespace tu {

struct DisplayTraits {
    static constexpr uint8_t kDC  = 9;   // OLED_DC
    static constexpr uint8_t kRST = 4;   // OLED_RST
    static constexpr uint8_t kCS  = 10;  // OLED_CS
};

struct ButtonTraits {
    static constexpr int kCount = 2;
    // but_top=3, but_bot=12 — active-low, INPUT_PULLUP
    static constexpr uint8_t kPins[kCount] = {3, 12};
};

struct EncoderTraits {
    static constexpr int kCount = 2;
    struct PinSet { uint8_t a, b, sw; };
    // LEFT:  encL1=22, encL2=21, butL=23
    // RIGHT: encR1=15, encR2=16, butR=13
    static constexpr PinSet kPins[kCount] = {{22, 21, 23}, {15, 16, 13}};
};

struct TriggerInputTraits {
    static constexpr int kCount = 2;
    // TR1=0, TR2=1 — active-low gate inputs
    static constexpr uint8_t kPins[kCount] = {0, 1};
};

struct CVInputTraits {
    static constexpr int kCount = 4;
    // CV1=A3(17), CV2=A6(20), CV3=A5(19), CV4=A4(18)
    static constexpr uint8_t kPins[kCount] = {17, 20, 19, 18};
};

struct TriggerOutputTraits {
    static constexpr int kCount = 6;
    // CLK1=5, CLK2=6, CLK3=7, CLK4=A14 (internal DAC), CLK5=8, CLK6=2
    // kPins[3] is unused (DAC channel handled specially in flush).
    static constexpr uint8_t kPins[kCount] = {5, 6, 7, 0, 8, 2};
    static constexpr int kDacChannel = 3;  // CLK4 = Teensy internal 12-bit DAC
};

} // namespace tu
