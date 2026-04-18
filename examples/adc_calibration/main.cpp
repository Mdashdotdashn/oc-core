#include "all.h"
#include "adc_calibration.h"

using Runtime = oc::Runtime<oc::platform::HardwarePlatform>;

Runtime runtime;
CalibrationApp<Runtime> app(runtime);

int main() {
    runtime.init(app);
    runtime.start(100);

    while (true) {
        runtime.poll();
    }
}