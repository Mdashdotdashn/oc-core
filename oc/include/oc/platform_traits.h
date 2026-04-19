#pragma once
#include <cstdint>

/// Hardware pin assignments for the Ornament & Crime (O_C) module.
/// Pass these as template arguments to the corresponding platform:: classes.
///
/// To port to another module, create an analogous traits header and
/// substitute it when instantiating the platform bundle.

namespace oc {

struct ButtonTraits {
    static constexpr int kCount = 2;
    // UP=5 (but_top), DOWN=4 (but_bot) — active-low, INPUT_PULLUP
    static constexpr uint8_t kPins[kCount] = {5, 4};
};

struct EncoderTraits {
    static constexpr int kCount = 2;
    struct PinSet { uint8_t a, b, sw; };
    // LEFT A/B are swapped to correct physical rotation direction (CW → +1).
    // LEFT: A=21, B=22, SW=23  |  RIGHT: A=16, B=15, SW=14
    static constexpr PinSet kPins[kCount] = {{21, 22, 23}, {16, 15, 14}};
};

struct TriggerInputTraits {
    static constexpr int kCount = 4;
    // TR1=0, TR2=1, TR3=2, TR4=3 — active-low gate inputs
    static constexpr uint8_t kPins[kCount] = {0, 1, 2, 3};
};

struct CVInputTraits {
    static constexpr int kCount = 4;
    // CV1=19, CV2=18, CV3=20, CV4=17 (verified against OC_gpio.h)
    static constexpr uint8_t kPins[kCount] = {19, 18, 20, 17};
};

struct CVOutputTraits {
    static constexpr int kCount = 4;
    static constexpr uint8_t kCSPin  = 10;  // DAC_CS
    static constexpr uint8_t kRSTPin = 9;   // DAC_RST
};

} // namespace oc
