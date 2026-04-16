# oc-core

Hardware Abstraction Framework for Ornament & Crime (Teensy 3.2).

Inspired by the Daisy Versio/Legio developer experience: write your algorithm once, plug it into the platform via a small application interface.

## Quick Start

```cpp
// my_algorithm.h — the only file you actually write
#include "oc/app.h"

class MyAlgorithm : public oc::Application {
public:
    void init() override { /* one-time setup */ }

    // Called every 100µs from hardware ISR (~10 kHz)
    void audio_callback(const oc::AudioIn& in, oc::AudioOut& out) override {
        out.cv[0] = process(in.cv[0]);   // read CV in, write CV out
    }

    // Called from while(1) — not timing-critical
    void idle() override { /* UI, parameters, display */ }
};
```

```cpp
// main.cpp — boilerplate, change only the algorithm type
#include "platforms/teensy32/all.h"
#include "my_algorithm.h"

using Runtime = oc::Runtime<oc::platform::teensy32::HardwarePlatform>;

Runtime     runtime;
MyAlgorithm app;

int main() {
    runtime.init(app);
    runtime.start(100);   // 100µs = 10 kHz

    while (true) {
        runtime.poll();
    }
}
```

The display is always present. Apps that need OLED output override `draw()`:

```cpp
using Runtime = oc::Runtime<oc::platform::teensy32::HardwarePlatform>;

Runtime   runtime;
MyDisplayApp app;

int main() {
    runtime.init(app);
    runtime.start(100);
    while (true) runtime.poll();
}
```

Build and flash:
```bash
cd examples/lfo
pio run -t upload
```

More detailed documentation now lives under `docs/`:

- [Getting Started](docs/GETTING_STARTED.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Porting A Platform](docs/PORTING_PLATFORM.md)
- [Project Notes](docs/PROJECT_NOTES.md)

## Repository Layout

```text
oc-core/
├── include/oc/              public app and HAL headers
├── src/                     Teensy 3.2 implementation
├── examples/                standalone PlatformIO example projects
└── docs/                    long-form architecture and project notes
```

## Current Behavior

- `audio_callback()` runs at `10 kHz` with a `100 us` period.
- This is a control/CV update loop, not an audio-sample buffer pipeline.
- The OLED is always present and shares `SPI0` with the DAC.
- DAC values are computed every ISR and committed to hardware at the start of the next ISR.
- Buttons and encoders are serviced by a separate lower-rate UI timer.
