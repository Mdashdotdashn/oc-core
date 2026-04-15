/// oc-core encoder_test example — main.cpp

#include "platforms/teensy32/all.h"
#include "encoder_test.h"
#include <Arduino.h>

oc::platform::teensy32::HardwarePlatform hw;
oc::core::PeriodicCore                   audio;
EncoderTest                              app;

void FASTRUN audio_callback() {
    audio.isr_cycle();
    hw.buttons()->scan();
    hw.encoders()->scan();

    const oc::core::CoreState& st = audio.get_state();
    oc::AudioIn in;
    for (int i = 0; i < 4; ++i) {
        in.cv[i]     = st.inputs.cv[i];
        in.cv_raw[i] = st.inputs.cv_raw[i];
        in.gate[i]   = st.inputs.gate[i];
    }
    in.gate_edges = st.inputs.edges;

    for (int i = 0; i < 2; ++i) {
        const auto b      = hw.buttons()->get(i);
        in.buttons[i]     = {b.pressed, b.just_pressed, b.just_released};
        const auto e      = hw.encoders()->get(i);
        in.encoders[i]    = {e.delta, e.click_pressed,
                             e.click_just_pressed, e.click_just_released};
    }

    oc::AudioOut out;
    app.audio_callback(in, out);

    for (int i = 0; i < 4; ++i) hw.dac()->write(i, out.cv[i]);
    hw.dac()->flush();
}

int main() {
    hw.init_all();
    audio.init(hw.adc(), hw.dac(), hw.gpio());
    app.init();
    hw.timer()->start(100, audio_callback);
    while (true) { app.main_loop(); }
}
