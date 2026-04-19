#include "platform/all.h"
#include "cpu_meter.h"

using Runtime = oc::Runtime<oc::platform::HardwarePlatform>;

Runtime runtime;
CpuMeter<Runtime> app(runtime);

int main() {
    runtime.init(app);
    runtime.start(100);

    while (true) {
        runtime.poll();
    }
}