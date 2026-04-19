#pragma once
#include <cstdint>

struct EncoderEvent {
    int8_t  delta;
    bool    click_pressed;
    bool    click_just_pressed;
    bool    click_just_released;
};

/// Teensy 3.2 encoder implementation.
///
/// Rotation: 2-bit shift-register state machine, identical to ArticCircle
/// UI/ui_encoder.h. Rising edge detection on pin A with pin B state determines
/// direction. No acceleration (can be added later if needed).
///
/// Click switch: 8-bit shift-register debounce, same as buttons.
///
/// Pins (from OC_gpio.h, board rev >= 2c):
///   LEFT  encoder: A=22, B=21, SW=23
///   RIGHT encoder: A=16, B=15, SW=14
/// All pins: active-low, INPUT_PULLUP.

namespace oc::platform::teensy32 {

class EncodersImpl final {
public:
    static constexpr int kCount = 2;

    void init();

    void scan();
    EncoderEvent get(uint8_t idx) const;

private:
    struct EncoderState {
        uint8_t pin_a;
        uint8_t pin_b;
        uint8_t pin_sw;
        uint8_t state_a  = 0xFF;  // shift register for pin A
        uint8_t state_b  = 0xFF;  // shift register for pin B
        uint8_t state_sw = 0xFF;  // shift register for switch
        int8_t  delta    = 0;
    };

    // [0]=LEFT (A=22,B=21,SW=23), [1]=RIGHT (A=16,B=15,SW=14)
    // LEFT A/B are swapped to match physical wiring: CCW→+1, CW→-1 without swap.
    // Swapping pin_a/pin_b corrects direction to CW→+1.
    EncoderState enc_[kCount] = {
        {21, 22, 23},   // LEFT:  A<->B swapped to correct rotation direction
        {16, 15, 14},   // RIGHT: A=16, B=15, SW=14
    };
};

} // namespace oc::platform::teensy32
