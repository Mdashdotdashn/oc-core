/// oc-core encoder_test example — main.cpp

#include "all.h"
#include "encoder_test.h"
#include <Arduino.h>

using Runtime = oc::Runtime<oc::platform::HardwarePlatform>;

Runtime     runtime;
EncoderTest app;

int main() {
    runtime.init(app);
    runtime.start(100);
    while (true) { runtime.poll(); }
}
