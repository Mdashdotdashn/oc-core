#include "platform/all.h"
#include "oc/platform.h"
#include "oc/runtime.h"
#include "trigger_to_cv.h"

using Runtime = oc::Runtime<platform::HardwarePlatform>;

Runtime runtime;
TriggerToCV app;

int main() {
    runtime.init(app);
    runtime.start(100);

    while (true) {
        runtime.poll();
    }
}
