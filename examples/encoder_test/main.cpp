/// oc-core encoder_test example — main.cpp

#include "platform/all.h"
#include "encoder_test.h"
#include <Arduino.h>

using Runtime = oc::Runtime<platform::HardwarePlatform>;

Runtime     runtime;
EncoderTest app;

int main() {
    runtime.init(app);
    runtime.start(100);
    while (true) { runtime.poll(); }
}
