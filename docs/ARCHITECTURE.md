# Architecture

## High-Level Structure

```text
┌─────────────────────────────────────────────────────────┐
│  User Code                                              │
│  class MyAlgo : public oc::Application {                │
│      audio_callback(AudioIn& in, AudioOut& out)         │
│      idle()                                             │
│      draw(DisplayInterface* display)                    │
│  }                                                      │
└────────────────────────┬────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────┐
│  oc::Runtime<Platform>  (core/include/oc/runtime.h)     │
│  • owns ISR ordering and app dispatch                   │
│  • calls platform concrete methods directly (no vtable) │
│  • holds PeriodicCore<ADCImpl, GPIOImpl>                │
└───────┬──────────┬───────────┬──────────────────────────┘
        │          │           │
   ADCImpl      DACImpl     GPIOImpl      (concrete, final)
        │          │           │
┌───────▼──────────▼───────────▼──────────────────────────┐
│  Teensy 3.2 Platform  (platform/)                      │
│  HardwarePlatform  owns concrete impl instances         │
│  adc_impl() / dac_impl() / gpio_impl() / …             │
│  — plus interface-pointer accessors for non-ISR code    │
└─────────────────────────────────────────────────────────┘
```

### Virtual dispatch policy

HAL interfaces (`ADCInterface`, `DACInterface`, etc.) still exist and are used:

- by calibration code, examples, and any non-ISR code that doesn't need to be overhead-free
- via the interface-pointer accessors on `HardwarePlatform` (`adc()`, `dac()`, …)

The ISR path uses **zero virtual dispatch**:

- All six concrete impl classes are marked `final`.
- `PeriodicCore<Adc, Gpio>` is a fully inline template; it holds concrete pointers and all calls resolve statically.
- `Runtime::isr()` and `ui_service()` use the `*_impl()` concrete-reference accessors on `HardwarePlatform`.
- The `Application::audio_callback()` virtual call is the only remaining vtable dispatch in the hot path (one call per ISR, unavoidable without also templating the app).

## Repository Layout

```text
oc-core/
├── core/
│   ├── include/oc/
│   │   ├── app.h
│   │   ├── calibration.h
│   │   ├── runtime.h
│   │   └── core/
│   │       └── periodic_core.h      ← fully inline template, no .cpp
│   └── src/
│       └── periodic_core.cpp        ← empty stub (template is header-only)
├── platform/
│   ├── include/
│   │   ├── platform.h       ← HardwarePlatform with *_impl() accessors
│   │   ├── all.h
│   │   ├── adc.h
│   │   ├── buttons.h
│   │   ├── dac.h
│   │   ├── display.h
│   │   ├── encoders.h
│   │   ├── gpio.h
│   │   ├── spi0_init.h
│   │   ├── timer.h
│   │   ├── storage.h
│   │   └── drivers/
│   │       ├── SH1106_128x64_driver.h
│   │       ├── framebuffer.h
│   │       ├── gfx_font_6x8.h
│   │       ├── page_display_driver.h
│   │       ├── util_SPIFIFO.h
│   │       └── weegfx.h
│   └── src/
│       ├── adc.cpp
│       ├── buttons.cpp
│       ├── dac.cpp
│       ├── encoders.cpp
│       ├── gpio.cpp
│       ├── storage.cpp
│       ├── timer.cpp
│       └── drivers/
│           ├── SH1106_128x64_driver.cpp
│           └── weegfx.cpp
├── examples/
│   ├── cpu_meter/
│   ├── lfo/
│   ├── display_test/
│   ├── quantizer/
│   └── turing_machine/
└── docs/
```

## Unified ISR Timing

The audio ISR fires at `10 kHz` with a `100 us` period. Because the OLED always shares `SPI0` with the DAC, every app follows the same schedule.

```text
ISR tick N begins
    1. display->flush()     complete the previous OLED page DMA and clean SPI0 state
    2. dac->flush()         send CV values staged during tick N-1 to the DAC8565
    3. display->update()    start the next OLED page DMA, or begin a new frame if ready
    4. core_.isr_cycle()    scan ADC and GPIO, capture a coherent CV/gate snapshot
    5. marshal UI state     fold in latest button/encoder scan from the separate UI timer
    6. audio_callback()     app computes the next four CV outputs from that snapshot
    7. dac->write()         stage those new CV outputs for tick N+1
ISR tick N ends

Foreground loop runs between ISR ticks
    A. idle()               non-real-time app work
    B. draw()               render into the OLED backbuffer in RAM
    C. end_frame()          publish the finished frame for later DMA transfer
```

That means output computation still happens every ISR, but the hardware DAC commit is intentionally one control tick behind the computation so the OLED DMA page transfer can start early and complete reliably.

## Display Pipeline

- `draw()` is called from `Runtime::poll()` outside the ISR.
- `draw()` only renders into a RAM backbuffer.
- `begin_frame()` returns the writable framebuffer.
- `end_frame()` publishes the finished frame for transfer.
- The OLED transfer happens inside the ISR, one `128`-byte page at a time via DMA.
- `flush()` finalizes the previous OLED page DMA and clears shared SPI state.
- `update()` starts the next OLED page DMA, or starts a new frame if a rendered backbuffer is ready.
- A full `128x64` framebuffer is `8` pages, so at `10 kHz` and one page per ISR a full refresh takes about `0.8 ms`.

## Input Acquisition Policy

- CV and gate acquisition run in the same ISR as output generation.
- This gives each control tick a coherent input snapshot and a bounded input-to-output latency.
- Buttons and encoders run from a separate lower-rate UI timer path rather than the `10 kHz` core ISR.
- The separate UI timer path for buttons and encoders has been verified on hardware.
- Gates are more timing-sensitive because short triggers and edge detection can be missed if polling becomes too irregular.
- CV acquisition could also move out of the main ISR, but snapshot age and latency would then depend on a separate schedule.

## DAC Behavior

- There is no audio-style sample buffer in the current system.
- The app computes one set of four CV outputs per ISR.
- `dac->write()` stages those values.
- `dac->flush()` sends them directly to the DAC8565 over SPI at the start of the next ISR.

## Runtime Profiling

- `Runtime` keeps moving averages for total ISR time and internal timing buckets.
- The current buckets are OLED work, DAC flush, hardware scan, input marshaling, app callback time, and residual overhead.
- `examples/cpu_meter` displays those averages on the OLED so runtime cost can be attributed before optimizing app code.

## Design Constraints

| Constraint | Rationale |
|-----------|-----------|
| No heap allocation in ISR | Deterministic timing |
| One active app at a time | Simpler scheduling and state ownership |
| Plain function pointer ISR registration | Teensy `IntervalTimer::begin()` takes `void(*)()` |
| Standalone PlatformIO examples | Prevent accidental source bleed from neighboring repos |

## Isolation From ArticCircle

`oc-core` is a sibling repository next to `ArticCircle`.

- separate git history
- all framework headers live under `core/include/oc/`
- each example uses explicit `build_src_filter` rules
- PlatformIO cannot accidentally pull unrelated ArticCircle sources into the build