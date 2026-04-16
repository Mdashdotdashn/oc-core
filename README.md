# oc-core

Hardware Abstraction Framework for Ornament & Crime (Teensy 3.2).

Inspired by the Daisy Versio/Legio developer experience: write your algorithm once, plug it into the platform via two methods. The `audio_callback + idle` pattern is taken directly from the Daisy examples — see the reference implementations in `~/devtree/sdk/DaisyExamples/legio/FMOscillator/FMOscillator.cpp` and `~/devtree/sdk/DaisyExamples/versio/Decimator/Decimator.cpp`.

## Local Workspace Paths

| Role | Path |
|------|------|
| **This framework** | `~/devtree/marc-nostromo/oc-core/` |
| **ArticCircle firmware** (HAL source material) | `~/devtree/marc-nostromo/ArticCircle/` |
| **Daisy examples** (UX reference) | `~/devtree/sdk/DaisyExamples/` |
| **Daisy Legio example** | `~/devtree/sdk/DaisyExamples/legio/FMOscillator/FMOscillator.cpp` |
| **Daisy Versio example** | `~/devtree/sdk/DaisyExamples/versio/Decimator/Decimator.cpp` |

---

## User Experience

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

using Runtime = oc::Runtime<oc::platform::teensy32::HardwarePlatform, false>;

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

Display-capable apps use the display-enabled specialization and override `draw()`:

```cpp
using Runtime = oc::Runtime<oc::platform::teensy32::HardwarePlatform, true>;

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

Callback rate note:
- `audio_callback()` currently runs at `10 kHz` (`100 µs` period).
- In practice this is a control/CV update rate, not an audio sample rate.
- There is no audio buffer or audio codec in this framework right now; the name `audio_callback` is inherited from the original app shape, but the current implementation is driving CV, gates, encoders, buttons, and display updates.
- This is not the same as the original O&C firmware: the original core ISR is commonly configured at about `16.67 kHz` (`60 µs` period, see `OC_CORE_ISR_FREQ = 16666` in the upstream config), while this framework currently uses a simpler `10 kHz` rate.

---

## Repository Layout

```
oc-core/
├── include/oc/
│   ├── app.h                      # User interface (Application, AudioIn, AudioOut)
│   ├── hal/
│   │   ├── adc.h                  # ADCInterface   — 4-channel CV input
│   │   ├── buttons.h              # ButtonsInterface — 2 debounced front-panel buttons
│   │   ├── dac.h                  # DACInterface   — 4-channel CV output
│   │   ├── display.h              # DisplayInterface — SH1106 OLED framebuffer + DMA paging
│   │   ├── encoders.h             # EncodersInterface — 2 quadrature encoders + switches
│   │   ├── gpio.h                 # GPIOInterface  — 4 gate inputs + edge detection
│   │   ├── timer.h                # TimerInterface — periodic ISR registration
│   │   └── storage.h              # StorageInterface — EEPROM read/write
│   └── core/
│       └── periodic_core.h        # PeriodicCore   — ISR coordinator + CoreState
│
├── src/
│   ├── core/
│   │   └── periodic_core.cpp
│   └── platforms/
│       └── teensy32/
│           ├── platform.h         # HardwarePlatform — owns all device instances
│           ├── all.h              # Convenience single-include for main.cpp
│           ├── adc_teensy32.h/cpp
│           ├── buttons_teensy32.h/cpp
│           ├── dac_teensy32.h/cpp
│           ├── display_teensy32.h
│           ├── encoders_teensy32.h/cpp
│           ├── gpio_teensy32.h/cpp
│           ├── spi0_init.h        # Shared SPI0 setup for DAC + OLED
│           ├── timer_teensy32.h/cpp
│           ├── storage_teensy32.h/cpp
│           └── drivers/
│               ├── SH1106_128x64_driver.h/cpp
│               ├── framebuffer.h
│               ├── gfx_font_6x8.h
│               ├── page_display_driver.h
│               └── weegfx.h/cpp
│
├── examples/
│   ├── lfo/                       # Triangle LFO — complete end-to-end example
│   │   ├── simple_lfo.h           # Algorithm (100 lines)
│   │   ├── main.cpp               # Firmware entry point (~40 lines)
│   │   └── platformio.ini         # Isolated PlatformIO project
│   ├── display_test/              # Buttons + encoders + OLED integration test
│   ├── quantizer/                 # (placeholder)
│   └── turing_machine/            # (placeholder)
│
└── docs/
```

---

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│  User Code                                              │
│  class MyAlgo : public oc::Application {                │
│      audio_callback(AudioIn& in, AudioOut& out)         │  ← writes algorithm here
│      idle()                                             │  ← writes UI/params here
│  }                                                      │
└────────────────────────┬────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────┐
│  Framework  (oc::Runtime + oc::core::PeriodicCore)      │
│  • Runtime       — owns ISR ordering + app dispatch     │
│  • isr_cycle()   — scans ADC + GPIO → CoreState         │
│  • get_state()   — exposes CoreState to Runtime         │
└───────┬──────────┬───────────┬──────────────────────────┘
        │          │           │
   ADCInterface  DACInterface  GPIOInterface   ← pure abstract HAL
        │          │           │
┌───────▼──────────▼───────────▼──────────────────────────┐
│  Teensy 3.2 Platform  (src/platforms/teensy32/)         │
│  ADCImpl / DACImpl / GPIOImpl / TimerImpl / StorageImpl │
│  ≈ 1200 lines extracted from ArticCircle               │
└─────────────────────────────────────────────────────────┘
```

The audio ISR fires at 10 kHz (100 µs). It calls `isr_cycle()` then `audio_callback()`. Input acquisition and output computation happen in that same ISR cycle.

DAC timing note:
- For non-display apps, DAC values are written and flushed in the same ISR.
- For display apps, DAC values are still computed in the same ISR, but the actual DAC SPI flush is intentionally deferred to the start of the next ISR so the OLED page DMA can start early and complete reliably.

The `while(1)` main loop is preempted freely and carries no timing obligations.

Display pipeline:
- `draw()` is called outside the ISR from `Runtime::poll()`.
- `draw()` only renders into a RAM backbuffer; it does not talk to the OLED directly.
- `begin_frame()` returns a writable framebuffer, and `end_frame()` marks that finished frame as ready for transfer.
- The actual OLED transfer happens inside the core ISR, one `128`-byte page at a time via DMA.
- ISR order for display apps is: `display->flush()` → `dac->flush()` → `display->update()` → control work.
- `flush()` finalizes the previous OLED page DMA and clears shared SPI state.
- `update()` starts the next OLED page DMA, or begins a new frame if a rendered backbuffer is ready.
- A full `128x64` framebuffer is `8` pages, so at `10 kHz` and one page per ISR a full screen refresh takes about `0.8 ms`.
- This split exists so the OLED and DAC can safely share `SPI0` without DMA/FIFO clashes.

Input acquisition policy:
- Today, CV and gate acquisition run in the same ISR as output generation so each control tick sees a coherent input snapshot and has a bounded input-to-output latency.
- This is a design choice, not a hard requirement for every input type.
- Buttons and encoders now run from a separate lower-rate UI timer path rather than the 10 kHz core ISR.
- The separate UI timer path for buttons and encoders has been verified on hardware.
- Gates are more timing-sensitive because short triggers and edge detection can be missed if polling becomes too irregular.
- CV acquisition could also be moved out of the main ISR, but then the snapshot age and latency would vary with foreground load unless it moved to a separate fixed-rate service.
- The current split is: keep CV/gate acquisition and output generation in the core ISR, and service buttons/encoders from a separate lower-rate timer.
- A possible future split is to move gate/CV scanning as well, but only if measurements show the remaining core ISR load matters.

---

## Implementation Plan

### Phase 1 — Scaffold ✅ (done)
- [x] HAL abstract interfaces: `adc.h`, `dac.h`, `gpio.h`, `timer.h`, `storage.h`
- [x] `oc::Application` base class with `audio_callback` + `idle`
- [x] `oc::core::PeriodicCore` ISR coordinator
- [x] Teensy 3.2 platform stubs for all 5 devices
- [x] `examples/lfo` — complete end-to-end template
- [x] Isolated PlatformIO build per example (no ArticCircle cross-contamination)

### Phase 2 — Teensy 3.2 HAL ✅ (done)
Wire the stubs to real hardware. All source material lives in `~/devtree/marc-nostromo/ArticCircle/`.

| File | Status | ArticCircle source to extract from |
|------|--------|------------------------------------|
| `adc_teensy32.cpp` | ✅ done | `ArticCircle/OC_ADC.cpp` |
| `dac_teensy32.cpp` | ✅ done | `ArticCircle/OC_DAC.cpp` — `set8565_CH*` via `util_SPIFIFO.h` |
| `gpio_teensy32.cpp` | ✅ done | `ArticCircle/OC_gpio.h` — TR1–TR4 = pins 0–3, INPUT_PULLUP |
| `timer_teensy32.cpp` | ✅ done | `ArticCircle/ArticCircle.ino` — `IntervalTimer` setup |
| `storage_teensy32.cpp` | ✅ done | standard EEPROM |

**Next actions:**
1. Open `~/devtree/marc-nostromo/ArticCircle/OC_gpio.h` — copy `TR1`–`TR4` pin defines into `src/platforms/teensy32/gpio_teensy32.h` `kPins[]`
2. Open `~/devtree/marc-nostromo/ArticCircle/OC_DAC.cpp` and `ArticCircle/src/drivers/` — wire `set8565_CH*` calls into `dac_teensy32.cpp::flush()`; copy or reference the SPI driver into `src/platforms/teensy32/drivers/`
3. Run `cd examples/lfo && pio run` and fix any include/linker errors

**Daisy UX reference** for how the callback wires into main:
- `~/devtree/sdk/DaisyExamples/legio/FMOscillator/FMOscillator.cpp` — knob + gate + audio callback
- `~/devtree/sdk/DaisyExamples/versio/Decimator/Decimator.cpp` — minimal effect callback

### Phase 3 — UI + Display Integration ✅
- [x] Build `examples/lfo` — clean build, 13.6 kB Flash / 4.7 kB RAM (Teensy 3.2: 256 kB Flash, 64 kB RAM)
- [x] ISR timing pin added: `digitalWriteFast(24, HIGH/LOW)` around `audio_callback()`
- [x] Buttons HAL verified on hardware
- [x] Encoders HAL verified on hardware
- [x] SH1106 OLED HAL verified on hardware
- [x] `examples/display_test` verified with buttons, encoders, and display active together

Display bring-up result:
- The OLED and DAC can share `SPI0`, but the OLED page DMA must start early in the ISR.
- Working order is: `display->flush()` → `dac->flush()` of values staged on the previous cycle → `display->update()` → ADC/GPIO/buttons/encoders/app work → stage DAC values for the next ISR.
- If `display->update()` runs late, the next ISR's `flush()` can terminate the OLED page transfer early, which shows up as a vertical garbage band on the right edge of the display.
- `SH1106_128x64_Driver::Flush()` also halts SPI0 and clears TX/RX FIFOs before returning control to the DAC path so OLED DMA bytes cannot intermix with DAC `SPIFIFO` writes.

### Phase 4 — Additional Examples
| Example | Demonstrates |
|---------|-------------|
| `examples/quantizer` | Multi-channel CV processing, scale lookup |
| `examples/turing_machine` | State across ISR cycles, gate triggering, `idle` parameter updates |

### Phase 5 — Documentation & Polish
- `docs/GETTING_STARTED.md` — new user guide (< 5 pages)
- `docs/PORTING_PLATFORM.md` — how to add STM32 / Daisy support (implement 5 HAL classes)
- API doc comments on all public interfaces
- Measure and document ISR budget with each example

---

## Design Constraints

| Constraint | Rationale |
|-----------|-----------|
| No virtual calls in ISR hot path | HAL pointers are set at init and never change; `isr_cycle()` and all HAL calls are direct virtual dispatches through stable pointers — acceptable cost vs. flexibility |
| No heap allocation in ISR | Deterministic timing; embedded constraint |
| `ISRHandler` is a plain function pointer | `std::function` has overhead and may allocate; Teensy `IntervalTimer::begin()` takes `void(*)()` |
| One app at a time | Simplicity; no app-switching framework needed |
| Each example is a standalone PlatformIO project | Zero risk of ArticCircle sources being pulled into framework builds |

---

## Isolation from ArticCircle

oc-core is a **sibling repository** at `~/devtree/marc-nostromo/oc-core`, next to `~/devtree/marc-nostromo/ArticCircle`.

- Separate git history — commits never cross
- All headers are under `include/oc/` — no name collision with `OC_*.h`
- Each example's `platformio.ini` uses `build_src_filter` to explicitly list sources — PlatformIO cannot accidentally pull in ArticCircle files
- `framework = arduino` can eventually be dropped once the SPI driver is moved here
