#include "platform/all.h"
#include "platform/trigger_outputs.h"
#include "tu/platform.h"
#include "tu/runtime.h"
#include "clock_test.h"

using Runtime = tu::Runtime<platform::TUHardwarePlatform>;

Runtime runtime;
ClockTest<Runtime> app(runtime);

int main() {
    runtime.init(app, 60);  // 60us = ~16.6 kHz (T_U standard)
    runtime.start();

    while (true) {
        runtime.poll();
    }
}
