/// oc-core LFO example — main.cpp (Teensy 3.2)
///
/// This is the entire boilerplate needed to run an oc-core algorithm
/// on Teensy 3.2 hardware. To write your own algorithm, replace
/// SimpleLFO with your own Application subclass.

#include "platform/all.h"
#include "oc/platform.h"
#include "oc/runtime.h"
#include "simple_lfo.h"
#include <Arduino.h>

// Debug / timing pin — toggle this around audio_callback() and measure
// the high pulse width on a scope or logic analyser.
// Pin 24 = OC_GPIO_DEBUG_PIN1 (back of board, spare GPIO, per OC_gpio.h).
static constexpr uint8_t kTimingPin = 24;

// ---------------------------------------------------------------------------
// Global instances — same pattern as Daisy examples
// ---------------------------------------------------------------------------

using Runtime = oc::Runtime<platform::HardwarePlatform>;

Runtime   runtime;
SimpleLFO app;

// ---------------------------------------------------------------------------
// main — initialization then background loop
// ---------------------------------------------------------------------------

int main() {
    runtime.set_timing_pin(kTimingPin);
    runtime.init(app);
    runtime.start(100);

    while (true) {
        runtime.poll();
    }
}
