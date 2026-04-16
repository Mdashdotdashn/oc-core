# Porting A Platform

## Goal

To add a new hardware target, implement the `oc::hal::*` interfaces for that platform and bundle them in a platform class that matches the shape expected by `oc::Runtime`.

The runtime currently assumes:

- one periodic core timer for the `10 kHz` control ISR
- one lower-rate UI timer for buttons and encoders
- one DAC path for four CV outputs
- one OLED display path that is always present
- coherent ADC and gate snapshots captured inside the core ISR

## Minimum Surface Area

Your platform must provide concrete implementations for these interfaces:

- `oc::hal::ADCInterface`
- `oc::hal::DACInterface`
- `oc::hal::GPIOInterface`
- `oc::hal::ButtonsInterface`
- `oc::hal::EncodersInterface`
- `oc::hal::DisplayInterface`
- `oc::hal::TimerInterface`
- `oc::hal::StorageInterface`

And it must expose them through a platform bundle with this effective shape:

```cpp
class HardwarePlatform {
public:
    void init_all();

    oc::hal::ADCInterface* adc();
    oc::hal::DACInterface* dac();
    oc::hal::GPIOInterface* gpio();
    oc::hal::ButtonsInterface* buttons();
    oc::hal::EncodersInterface* encoders();
    oc::hal::DisplayInterface* display();
    oc::hal::TimerInterface* timer();
    oc::hal::StorageInterface* storage();
};
```

`oc::Runtime<YourPlatform>` only relies on that contract. It does not care about the concrete device classes.

## Runtime Expectations

The runtime uses the platform in this order:

```text
init()
  platform.init_all()
  core.init(platform.adc(), platform.dac(), platform.gpio())

start()
  platform.timer()->start(core_interval_us, isr_handler)
  platform.timer()->start_ui(ui_interval_us, ui_handler)

poll()
  app.idle()
  app.draw(platform.display())
```

And inside the core ISR:

```text
display->flush()
dac->flush()
display->update()
adc/gpio scan via PeriodicCore
buttons/encoders state consumed from UI timer snapshot
app->audio_callback(in, out)
dac->write(...)
```

That order matters. In particular, the current architecture assumes the display and DAC may contend for the same bus, so `display->flush()` and `display->update()` bracket the DAC hardware write phase.

## Interface Notes

### ADC

Requirements:

- 4 input channels
- ISR-safe `scan()`
- raw and smoothed readings
- calibrated signed values for pitch CV use

Guidance:

- `scan()` should be cheap and deterministic
- if the ADC runs as a rolling pipeline, `scan()` should read the previous conversion and kick the next one
- calibration policy can remain platform-specific as long as `get_calibrated()` returns stable signed control values

### GPIO

Requirements:

- 4 gate or trigger inputs
- current logic level per input
- rising-edge mask since the previous scan

Guidance:

- edge detection should be performed in `scan()`
- the edge mask should represent the interval between the previous and current core scans

### Buttons And Encoders

Requirements:

- fixed-rate scanning from the separate UI timer
- debounced press state and one-shot edge events
- encoder rotation delta plus click state

Guidance:

- the UI timer should be stable enough that debounce behavior is predictable
- `1 kHz` is the current default and works well for the Teensy implementation

### DAC

Requirements:

- 4 output channels
- `write()` stages values only
- `flush()` commits the staged set to hardware

Guidance:

- do not make `write()` touch hardware directly
- the runtime depends on staging because values are computed late in tick `N` and committed at the start of tick `N+1`
- if your platform has a hardware DAC FIFO, you can still preserve the contract by treating `flush()` as the single commit point

### Display

Requirements:

- `flush()` finalizes the previous transfer unit
- `update()` starts the next transfer unit if work is pending
- `begin_frame()` grants a writable backbuffer
- `end_frame()` publishes the frame for transfer

Guidance:

- the transfer unit does not have to be a page, but the model assumes incremental transfer over multiple ISR ticks
- `draw()` runs in foreground code, not in the ISR
- if your display is memory-mapped or cheap to update, you can still satisfy the contract with trivial `flush()` and `update()` implementations

### Timer

Requirements:

- one periodic ISR source for the core loop
- one periodic ISR source for UI scanning
- plain function pointer callback support

Guidance:

- the callback type is `void (*)()`
- keep timer jitter low because ADC/gate latency is defined by the core timer
- do not do work in the timer layer beyond invoking the registered callback

### Storage

Requirements:

- byte-addressable persistent read and write
- enough capacity for calibration and settings blobs

Guidance:

- storage is not used in the hot ISR path
- EEPROM, flash emulation, or backed-up RAM are all acceptable if the semantics are stable

## Suggested Directory Layout

Follow the Teensy layout as closely as possible:

```text
src/platforms/<platform>/
├── all.h
├── platform.h
├── adc_<platform>.h/cpp
├── buttons_<platform>.h/cpp
├── dac_<platform>.h/cpp
├── display_<platform>.h
├── encoders_<platform>.h/cpp
├── gpio_<platform>.h/cpp
├── timer_<platform>.h/cpp
├── storage_<platform>.h/cpp
└── drivers/
```

That keeps example entrypoints simple and makes platform diffs easier to read.

## Bring-Up Order

This sequence is usually the least painful:

1. Get the timer firing and confirm `audio_callback()` cadence on a scope pin.
2. Bring up ADC and GPIO so input snapshots are coherent.
3. Bring up DAC staging and flush so CV outputs are correct without the display.
4. Add the display transfer path and verify it coexists with DAC writes.
5. Add buttons and encoders on the separate UI timer.
6. Validate the full ISR load with `examples/cpu_meter`.

## Common Failure Modes

- Writing DAC values directly in `write()` breaks the staged timing model and can corrupt bus sequencing.
- Updating the display too late in the ISR can leave the next `flush()` terminating an in-flight transfer.
- Polling buttons or encoders from the foreground loop makes debounce behavior dependent on render load.
- Moving gate scanning out of the fixed-rate core loop can make short triggers unreliable.

## Port Completion Checklist

- `examples/lfo` builds and runs
- `examples/display_test` shows stable OLED output and live controls
- `examples/cpu_meter` reports sensible ISR load
- DAC output polarity and channel order are verified on hardware
- gate edges are not missed at the chosen core ISR rate
- button and encoder events are stable at the chosen UI timer rate