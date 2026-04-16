/// oc-core encoder_test example — main.cpp

#include "platforms/teensy32/all.h"
#include "encoder_test.h"
#include <Arduino.h>

using Runtime = oc::Runtime<oc::platform::teensy32::HardwarePlatform, false>;

Runtime     runtime;
EncoderTest app;

int main() {
    runtime.init(app);
    runtime.start(100);
    while (true) { runtime.poll(); }
}
