# Project Notes

## Local Workspace Paths

| Role | Path |
|------|------|
| This framework | `~/devtree/marc-nostromo/oc-core/` |
| ArticCircle firmware source material | `~/devtree/marc-nostromo/ArticCircle/` |
| Daisy examples UX reference | `~/devtree/sdk/DaisyExamples/` |
| Daisy Legio example | `~/devtree/sdk/DaisyExamples/legio/FMOscillator/FMOscillator.cpp` |
| Daisy Versio example | `~/devtree/sdk/DaisyExamples/versio/Decimator/Decimator.cpp` |

## Completed Work

### Phase 1: Scaffold

- HAL abstract interfaces for ADC, DAC, GPIO, timer, storage
- `oc::Application` base class
- `oc::core::PeriodicCore`
- Teensy 3.2 platform skeleton
- `examples/lfo` template
- isolated PlatformIO builds per example

### Phase 2: Teensy 3.2 HAL

| File | Status | Source material |
|------|--------|-----------------|
| `adc.cpp` | done | `ArticCircle/OC_ADC.cpp` |
| `dac.cpp` | done | `ArticCircle/OC_DAC.cpp` |
| `gpio.cpp` | done | `ArticCircle/OC_gpio.h` |
| `timer.cpp` | done | `ArticCircle/ArticCircle.ino` |
| `storage.cpp` | done | standard EEPROM |

### Phase 3: UI And Display Integration

- `examples/lfo` clean build on Teensy 3.2
- ISR timing pin added on pin `24`
- buttons HAL verified on hardware
- encoders HAL verified on hardware
- SH1106 OLED HAL verified on hardware
- `examples/display_test` verified with buttons, encoders, and display active together

Display bring-up result:

- the OLED and DAC can share `SPI0`, but the OLED page DMA must start early in the ISR
- working order is `display->flush()` then `dac->flush()` then `display->update()` then control work
- if `display->update()` runs late, the next ISR can terminate the OLED transfer early and corrupt the right side of the display
- `SH1106_128x64_Driver::Flush()` halts SPI0 and clears TX and RX FIFOs before returning control to the DAC path

### Phase 4: Additional Examples

| Example | Demonstrates |
|---------|-------------|
| `examples/cpu_meter` | OLED display of averaged ISR runtime and load percentage |
| `examples/quantizer` | Multi-channel CV processing and scale lookup |
| `examples/turing_machine` | state across ISR cycles, gate triggering, `idle()` updates |

## Documentation And Polish

- public API doc comments added on the main interfaces

## Phase 5: ISR Devirtualization

Goal: eliminate virtual dispatch from the 10 kHz ISR hot path.

Changes made (branch `no-virtual`):

| Change | Effect |
|--------|--------|
| All 6 concrete impl classes marked `final` | Enables compiler devirtualization on any concrete-pointer call |
| `HardwarePlatform` gains `*_impl()` concrete-reference accessors and `AType`/`GType`/… type aliases | Exposes concrete types to `Runtime` and `PeriodicCore` |
| `PeriodicCore<Adc, Gpio>` converted to fully inline template | 15 vtable dispatches in `isr_cycle()` → 0; tiny getters inline to array loads |
| `Runtime::isr()` and `ui_service()` use `*_impl()` accessors throughout | All ISR HAL calls go through concrete types |
| `Inputs` marshal: removed `{}` zero-init + element loops → direct `std::array` assignment + single `ui_mask` bitwise gate | Eliminates ~80-byte memset and per-field conditional branches |

Measured ISR profile at 10 kHz on Teensy 3.2 (72 MHz), idle app:

| Bucket | Before | After |
|--------|--------|-------|
| TOT | 32 µs | 28 µs |
| SCN | 11 µs | 9 µs |
| MRS | 5 µs | 3 µs |
| DSP | 6 µs | 6 µs (SPI hardware-bound) |
| DAC | 8 µs | 8 µs (SPI hardware-bound) |

ISR load with idle app: **28%** of the 100 µs budget.
Remaining virtual call in the hot path: `Application::audio_callback()` (one per ISR, intentional).
- runtime facade introduced to centralize sequencing
- app-facing `main_loop()` renamed to `idle()`
- long-form project notes moved under `docs/`

## Current Gaps

- placeholder examples are still placeholders
- ISR budget should eventually be measured and recorded per example