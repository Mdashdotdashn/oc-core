/// oc-core LFO example — main.cpp (Teensy 3.6)
///
/// This is the entire boilerplate needed to run an oc-core algorithm
/// on Teensy 3.6 hardware. To write your own algorithm, replace
/// SimpleLFO with your own Application subclass.

#include "platforms/teensy36/all.h"
#include "simple_lfo.h"

// ---------------------------------------------------------------------------
// Global instances — same pattern as Daisy examples
// ---------------------------------------------------------------------------

oc::platform::teensy36::HardwarePlatform hw;
oc::core::PeriodicCore                   audio;
SimpleLFO                                app;

// ---------------------------------------------------------------------------
// Audio ISR — registered with the timer, called every 100µs
// ---------------------------------------------------------------------------

void FASTRUN audio_callback() {
    // Step 1: Scan hardware (ADC + GPIO) and build CoreState
    audio.isr_cycle();

    // Step 2: Convert CoreState → AudioIn
    const oc::core::CoreState& st = audio.get_state();
    oc::AudioIn in;
    for (int i = 0; i < 4; ++i) {
        in.cv[i]     = st.inputs.cv[i];
        in.cv_raw[i] = st.inputs.cv_raw[i];
        in.gate[i]   = st.inputs.gate[i];
    }
    in.gate_edges = st.inputs.edges;

    // Step 3: Process algorithm
    oc::AudioOut out;
    app.audio_callback(in, out);

    // Step 4: Write outputs to DAC
    for (int i = 0; i < 4; ++i) {
        hw.dac()->write(i, out.cv[i]);
    }
    hw.dac()->flush();
}

// ---------------------------------------------------------------------------
// main — initialization then background loop
// ---------------------------------------------------------------------------

int main() {
    // Initialize all hardware (ADC, DAC, GPIO)
    hw.init_all();

    // Wire HAL devices into the framework coordinator
    audio.init(hw.adc(), hw.dac(), hw.gpio());

    // Initialize user algorithm
    app.init();

    // Register audio ISR and start the timer (100µs = 10kHz)
    hw.timer()->start(100, audio_callback);

    // Background loop — NOT timing-critical
    // Preempted by audio_callback every 100µs; safe for anything slow.
    while (true) {
        app.main_loop();
    }
}
