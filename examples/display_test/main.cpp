/// oc-core display_test example — main.cpp
///
/// Tests the SH1106 OLED display. Shows live encoder and button state.
/// The ISR handles DMA page transfers (flush + update).
/// idle() renders the frame via weegfx.

#include "platforms/all.h"
#include "display_test.h"
#include <Arduino.h>

using Runtime = oc::Runtime<oc::platform::HardwarePlatform>;

Runtime     runtime;
DisplayTest app;

int main() {
    runtime.init(app);
    runtime.start(100);

    while (true) {
        runtime.poll();
    }
}
