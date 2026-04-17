#include "platforms/teensy32/all.h"
#include "adc_calibration.h"

using Runtime = oc::Runtime<oc::platform::teensy32::HardwarePlatform>;

Runtime runtime;
CalibrationApp<Runtime> app(runtime);

int main() {
    runtime.init(app);
    runtime.start(100);

    while (true) {
        runtime.poll();
    }
}