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
| `adc_teensy32.cpp` | done | `ArticCircle/OC_ADC.cpp` |
| `dac_teensy32.cpp` | done | `ArticCircle/OC_DAC.cpp` |
| `gpio_teensy32.cpp` | done | `ArticCircle/OC_gpio.h` |
| `timer_teensy32.cpp` | done | `ArticCircle/ArticCircle.ino` |
| `storage_teensy32.cpp` | done | standard EEPROM |

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
- runtime facade introduced to centralize sequencing
- app-facing `main_loop()` renamed to `idle()`
- long-form project notes moved under `docs/`

## Current Gaps

- placeholder examples are still placeholders
- ISR budget should eventually be measured and recorded per example