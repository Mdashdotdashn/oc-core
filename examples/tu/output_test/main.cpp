/// Minimal T_U output test — no SPI, no display, no ADC.
///
/// What it does:
///   CLK1 = TR1 pass-through  (gate input → gate output)
///   CLK2 = TR2 pass-through
///   CLK3 = 1 Hz square wave (blinks ~every 8k ISR ticks at 16.6 kHz)
///   CLK4 = A14 internal DAC, ramps 0→4095 every 2048 ticks
///   CLK5 = 2 Hz square wave
///   CLK6 = 4 Hz square wave
///
/// If the Teensy LED (pin 13) blinks at ~0.5Hz, the ISR is running.

#include <Arduino.h>
#include <IntervalTimer.h>
#include <kinetis.h>
#include "tu/platform_traits.h"
#include "platform/trigger_inputs.h"
#include "platform/trigger_outputs.h"

using TrigIn  = platform::TriggerInputs<tu::TriggerInputTraits>;
using TrigOut = platform::TriggerOutputs<tu::TriggerOutputTraits>;

static TrigIn  trig_in;
static TrigOut trig_out;

static IntervalTimer isr_timer;
static volatile uint32_t tick = 0;

void FASTRUN isr() {
    trig_in.scan();

    const uint32_t t = tick;

    // CLK1/CLK2: gate pass-through
    trig_out.write(0, trig_in.read_input(0));
    trig_out.write(1, trig_in.read_input(1));

    // CLK3: toggle every 8192 ticks  (~0.5 Hz at 16.6 kHz ISR)
    trig_out.write(2, bool((t >> 13) & 1));

    // CLK4: ramp DAC 0→4095 over 4096 ticks, repeat
    trig_out.write_analog(3, static_cast<uint16_t>(t & 0x0FFF));

    // CLK5: toggle every 4096 ticks  (~1 Hz)
    trig_out.write(4, bool((t >> 12) & 1));

    // CLK6: toggle every 2048 ticks  (~2 Hz)
    trig_out.write(5, bool((t >> 11) & 1));

    trig_out.flush();
    ++tick;
}

int main() {
    // Blink the Teensy LED so we can tell the MCU is alive.
    pinMode(LED_BUILTIN, OUTPUT);

    // Init trigger I/O (no SPI, no display).
    trig_in.init();
    trig_out.init();

    isr_timer.begin(isr, 60);       // 60µs = 16.6 kHz
    isr_timer.priority(80);

    uint32_t last = 0;
    while (true) {
        const uint32_t t = tick;
        // Toggle LED roughly every 0.5s (8192 ticks at 16.6kHz)
        if ((t >> 13) != (last >> 13)) {
            digitalToggleFast(LED_BUILTIN);
            last = t;
        }
    }
}
