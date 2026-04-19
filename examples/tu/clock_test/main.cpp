#include "platform/all.h"
#include "platform/trigger_outputs.h"
#include "tu/platform.h"
#include "tu/runtime.h"
#include "clock_test.h"

using Runtime = tu::Runtime<platform::TUHardwarePlatform>;

Runtime runtime;
ClockTest<Runtime> app(runtime);

int main() {
    runtime.init(app);
    runtime.start(60);  // 60µs = ~16.6 kHz (T_U standard)

    while (true) {
        runtime.poll();
    }
}
