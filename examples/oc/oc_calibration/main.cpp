#include "platform/all.h"
#include "oc/platform.h"
#include "oc/runtime.h"
#include "oc_calibration.h"

using Runtime = oc::Runtime<platform::HardwarePlatform>;

Runtime runtime;
CalibrationApp<Runtime> app(runtime);

int main() {
    runtime.init(app, 100);
    runtime.start();

    while (true) {
        runtime.poll();
    }
}