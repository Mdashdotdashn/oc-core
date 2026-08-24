#include "platform/all.h"
#include "tu/platform.h"
#include "tu/runtime.h"
#include "tu_test.h"

using Runtime = tu::Runtime<platform::TUHardwarePlatform>;

Runtime runtime;
TuTestApp<Runtime> app(runtime);

int main() {
    runtime.init(app, 60);  // 60us = ~16.6 kHz (T_U standard)

    while (true) {
        runtime.poll();
    }
}
