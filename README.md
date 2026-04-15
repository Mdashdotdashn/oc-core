# oc-core

Hardware Abstraction Framework for Ornament & Crime (Teensy 3.6).

Inspired by the [Daisy Versio/Legio](https://github.com/electro-smith/DaisyExamples) developer experience: write your algorithm once, plug it into the platform via two methods.

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
    void main_loop() override { /* UI, parameters, display */ }
};
```

```cpp
// main.cpp — boilerplate, change only the algorithm type
#include "platforms/teensy36/all.h"
#include "my_algorithm.h"

oc::platform::teensy36::HardwarePlatform hw;
oc::core::PeriodicCore                   audio;
MyAlgorithm                              app;

void FASTRUN audio_callback() {
    audio.isr_cycle();
    const oc::core::CoreState& st = audio.get_state();

    oc::AudioIn in;
    for (int i = 0; i < 4; ++i) {
        in.cv[i] = st.inputs.cv[i];  in.gate[i] = st.inputs.gate[i];
    }
    in.gate_edges = st.inputs.edges;

    oc::AudioOut out;
    app.audio_callback(in, out);

    for (int i = 0; i < 4; ++i) hw.dac()->write(i, out.cv[i]);
    hw.dac()->flush();
}

int main() {
    hw.init_all();
    audio.init(hw.adc(), hw.dac(), hw.gpio());
    app.init();
    hw.timer()->start(100, audio_callback);   // 100µs = 10 kHz
    while (true) app.main_loop();
}
```

Build and flash:
```bash
cd examples/lfo
pio run -t upload
```

---

## Repository Layout

```
oc-core/
├── include/oc/
│   ├── app.h                      # User interface (Application, AudioIn, AudioOut)
│   ├── hal/
│   │   ├── adc.h                  # ADCInterface   — 4-channel CV input
│   │   ├── dac.h                  # DACInterface   — 4-channel CV output
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
│       └── teensy36/
│           ├── platform.h         # HardwarePlatform — owns all device instances
│           ├── all.h              # Convenience single-include for main.cpp
│           ├── adc_teensy36.h/cpp
│           ├── dac_teensy36.h/cpp  ⚠ SPI driver stub — see Phase 2
│           ├── gpio_teensy36.h/cpp ⚠ Pin numbers stub — see Phase 2
│           ├── timer_teensy36.h/cpp
│           └── storage_teensy36.h/cpp
│
├── examples/
│   ├── lfo/                       # Triangle LFO — complete end-to-end example
│   │   ├── simple_lfo.h           # Algorithm (100 lines)
│   │   ├── main.cpp               # Firmware entry point (~40 lines)
│   │   └── platformio.ini         # Isolated PlatformIO project
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
│      main_loop()                                        │  ← writes UI/params here
│  }                                                      │
└────────────────────────┬────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────┐
│  Framework  (oc::core::PeriodicCore)                    │
│  • isr_cycle()    — scans ADC + GPIO → CoreState        │
│  • get_state()    — exposes CoreState to audio ISR      │
│  ~300 lines, no business logic                          │
└───────┬──────────┬───────────┬──────────────────────────┘
        │          │           │
   ADCInterface  DACInterface  GPIOInterface   ← pure abstract HAL
        │          │           │
┌───────▼──────────▼───────────▼──────────────────────────┐
│  Teensy 3.6 Platform  (src/platforms/teensy36/)         │
│  ADCImpl / DACImpl / GPIOImpl / TimerImpl / StorageImpl │
│  ≈ 1200 lines extracted from ArticCircle               │
└─────────────────────────────────────────────────────────┘
```

The audio ISR fires at 10 kHz (100 µs). It calls `isr_cycle()` then `audio_callback()`. The `while(1)` main loop is preempted freely and carries no timing obligations.

---

## Implementation Plan

### Phase 1 — Scaffold ✅ (done)
- [x] HAL abstract interfaces: `adc.h`, `dac.h`, `gpio.h`, `timer.h`, `storage.h`
- [x] `oc::Application` base class with `audio_callback` + `main_loop`
- [x] `oc::core::PeriodicCore` ISR coordinator
- [x] Teensy 3.6 platform stubs for all 5 devices
- [x] `examples/lfo` — complete end-to-end template
- [x] Isolated PlatformIO build per example (no ArticCircle cross-contamination)

### Phase 2 — Teensy 3.6 HAL (current)
Wire the stubs to real hardware. All source material lives in `../ArticCircle/`.

| File | Status | Source to extract from |
|------|--------|------------------------|
| `adc_teensy36.cpp` | ✅ done | `OC_ADC.cpp` |
| `dac_teensy36.cpp` | ⚠ `flush()` stub | `OC_DAC.cpp` — `set8565_CH*` + `src/drivers/` |
| `gpio_teensy36.cpp` | ⚠ pin numbers | `OC_gpio.h` — `TR1`–`TR4` defines |
| `timer_teensy36.cpp` | ✅ done | `ArticCircle.ino` — `IntervalTimer` setup |
| `storage_teensy36.cpp` | ✅ done | standard EEPROM |

**Next actions:**
1. Copy pin defines from `../ArticCircle/OC_gpio.h` into `gpio_teensy36.h`
2. Wire `dac_teensy36.cpp::flush()` to the SPI routines in `../ArticCircle/src/drivers/`
   — either symlink the driver or copy it into `src/platforms/teensy36/drivers/`
3. Attempt first `pio run` on the LFO example

### Phase 3 — Integration Test
- Build `examples/lfo` without errors
- Flash to hardware; verify triangle wave appears on all 4 outputs
- Measure ISR execution time with a spare GPIO toggled around `audio_callback()`
  (target: < 50 µs out of the 100 µs budget)

### Phase 4 — Additional Examples
| Example | Demonstrates |
|---------|-------------|
| `examples/quantizer` | Multi-channel CV processing, scale lookup |
| `examples/turing_machine` | State across ISR cycles, gate triggering, `main_loop` parameter updates |

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

oc-core is a **sibling repository** (`../ArticCircle` / `../oc-core`).

- Separate git history — commits never cross
- All headers are under `include/oc/` — no name collision with `OC_*.h`
- Each example's `platformio.ini` uses `build_src_filter` to explicitly list sources — PlatformIO cannot accidentally pull in ArticCircle files
- `framework = arduino` can eventually be dropped once the SPI driver is moved here
