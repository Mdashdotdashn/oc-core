#include "platform/all.h"
#include "trigger_test.h"

using Runtime = oc::Runtime<platform::HardwarePlatform>;

Runtime runtime;
TriggerTest app;

int main() {
    runtime.init(app);
    runtime.start(100);

    while (true) {
        runtime.poll();
    }
}