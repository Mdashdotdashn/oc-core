#pragma once
#include <cstdint>

namespace oc {

struct EncoderEvent {
    int8_t  delta;
    bool    click_pressed;
    bool    click_just_pressed;
    bool    click_just_released;
};

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

} // namespace oc
