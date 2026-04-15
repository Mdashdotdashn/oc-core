/// oc-core button_test example — main.cpp
///
/// Demonstrates reading the two front-panel push-buttons.
/// See button_test.h for the algorithm.

#include "platforms/teensy32/all.h"
#include "button_test.h"
#include <Arduino.h>

oc::platform::teensy32::HardwarePlatform hw;
oc::core::PeriodicCore                   audio;
ButtonTest                               app;

void FASTRUN audio_callback() {
    // Scan hardware: ADC + GPIO (gates)
    audio.isr_cycle();

    // Scan buttons — polled here in the audio ISR so just_pressed/just_released
    // are available in AudioIn every cycle.
    hw.buttons()->scan();

    // Build AudioIn from CoreState + button state
    const oc::core::CoreState& st = audio.get_state();
    oc::AudioIn in;
    for (int i = 0; i < 4; ++i) {
        in.cv[i]     = st.inputs.cv[i];
        in.cv_raw[i] = st.inputs.cv_raw[i];
        in.gate[i]   = st.inputs.gate[i];
    }
    in.gate_edges = st.inputs.edges;

    for (int i = 0; i < 2; ++i) {
        const auto ev     = hw.buttons()->get(i);
        in.buttons[i] = {ev.pressed, ev.just_pressed, ev.just_released};
    }

    // Process algorithm
    oc::AudioOut out;
    app.audio_callback(in, out);

    // Write outputs
    for (int i = 0; i < 4; ++i) {
        hw.dac()->write(i, out.cv[i]);
    }
    hw.dac()->flush();
}

int main() {
    hw.init_all();
    audio.init(hw.adc(), hw.dac(), hw.gpio());
    app.init();
    hw.timer()->start(100, audio_callback);

    while (true) {
        app.main_loop();
    }
}
