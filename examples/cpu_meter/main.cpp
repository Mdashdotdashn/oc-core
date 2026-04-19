#include "platform/all.h"
#include "oc/platform.h"
#include "oc/runtime.h"
#include "cpu_meter.h"

using Runtime = oc::Runtime<platform::HardwarePlatform>;

Runtime runtime;
CpuMeter<Runtime> app(runtime);

int main() {
    runtime.init(app);
    runtime.start(100);

    while (true) {
        runtime.poll();
    }
}