#include "platform/all.h"
#include "oc/platform.h"
#include "oc/runtime.h"
#include "adc_calibration.h"

using Runtime = oc::Runtime<platform::HardwarePlatform>;

Runtime runtime;
CalibrationApp<Runtime> app(runtime);

int main() {
    runtime.init_hardware();
    runtime.begin(app);
    runtime.start(100);

    while (true) {
        runtime.poll();
    }
}