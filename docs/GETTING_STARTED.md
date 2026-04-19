# Getting Started

## Overview

`oc-core` is a small framework for writing Ornament & Crime applications on Teensy 3.2.

The user-facing model is intentionally narrow:

- implement `audio_callback()` for real-time CV processing
- optionally implement `idle()` for non-real-time work
- optionally implement `draw()` for OLED rendering into the RAM backbuffer

## Basic App Shape

```cpp
#include "oc/app.h"

class MyAlgorithm : public oc::Application {
public:
    void init() override {
    }

    void audio_callback(const oc::AudioIn& in, oc::AudioOut& out) override {
        out.cv[0] = in.cv_raw[0] << 4;
    }

    void idle() override {
    }
};
```

## Basic Entrypoint

```cpp
#include "platform/all.h"
#include "my_algorithm.h"

using Runtime = oc::Runtime<oc::platform::HardwarePlatform>;

Runtime runtime;
MyAlgorithm app;

int main() {
    runtime.init(app);
    runtime.start(100);

    while (true) {
        runtime.poll();
    }
}
```

## Display Usage

The display is always part of the runtime. Apps that need OLED output override `draw()`.

```cpp
void draw(oc::Display* display) override {
    if (!display->begin_frame()) {
        return;
    }

    // render into display->frame_buffer()

    display->end_frame();
}
```

`draw()` does not talk to the OLED directly. It only fills the RAM backbuffer. The actual OLED transfer happens later inside the ISR.

## Prerequisites

Install PlatformIO CLI (verified on Linux, macOS, and Windows/Git Bash):

**macOS**
```bash
brew install platformio
```

**Linux**
```bash
pipx install platformio
# or
pip3 install --user platformio
```

**Windows (Git Bash)**
```bash
pip install platformio   # in a Python that Git Bash can see; add pio to PATH
# or use the PlatformIO VS Code extension
```

## Build And Flash

```bash
cd examples/lfo
pio run -t upload
```

Each example is its own PlatformIO project.

> **First flash:** if the Teensy does not have oc-core firmware already loaded,
> soft-reboot won't work. Press the physical reset button on the board when
> the uploader prompts for it.

## Timing Model

- `audio_callback()` runs at `10 kHz` with a `100 us` period.
- This is a control/CV scheduling loop, not an audio sample stream.
- There is no audio codec and no audio ring buffer in the current framework.
- The original O_C firmware commonly uses about `16.67 kHz` (`60 us`) for its core ISR, so this framework currently runs at a lower rate.

## Where To Read Next

- [Architecture](ARCHITECTURE.md)
- [Project Notes](PROJECT_NOTES.md)