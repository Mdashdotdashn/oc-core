#include "platforms/teensy32/all.h"
#include "voltage_output.h"
#include <Arduino.h>

static constexpr uint8_t kTimingPin = 24;

oc::platform::teensy32::HardwarePlatform hw;
oc::core::PeriodicCore                   audio;
VoltageOutput                            app;

void FASTRUN audio_callback() {
    digitalWriteFast(kTimingPin, HIGH);

    audio.isr_cycle();

    const oc::core::CoreState& st = audio.get_state();
    oc::AudioIn in;
    for (int i = 0; i < 4; ++i) {
        in.cv[i]     = st.inputs.cv[i];
        in.cv_raw[i] = st.inputs.cv_raw[i];
        in.gate[i]   = st.inputs.gate[i];
    }
    in.gate_edges = st.inputs.edges;

    oc::AudioOut out;
    app.audio_callback(in, out);

    for (int i = 0; i < 4; ++i) hw.dac()->write(i, out.cv[i]);
    hw.dac()->flush();

    digitalWriteFast(kTimingPin, LOW);
}

int main() {
    pinMode(kTimingPin, OUTPUT);
    digitalWriteFast(kTimingPin, LOW);

    hw.init_all();
    audio.init(hw.adc(), hw.dac(), hw.gpio());
    app.init();

    hw.timer()->start(100, audio_callback);
    while (true) app.main_loop();
}
