#include "platforms/all.h"
#include "trigger_test.h"

using Runtime = oc::Runtime<oc::platform::HardwarePlatform>;

Runtime runtime;
TriggerTest app;

int main() {
    runtime.init(app);
    runtime.start(100);

    while (true) {
        runtime.poll();
    }
}