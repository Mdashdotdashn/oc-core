/// T_U display + ISR + ADC + full GPIO test.
/// Encoder counts accumulated in UI ISR to avoid the one-tick display race.

#include <Arduino.h>
#include <IntervalTimer.h>
#include <kinetis.h>
#include "tu/platform_traits.h"
#include "platform/spi0_init.h"
#include "platform/display.h"
#include "platform/cv_inputs.h"
#include "platform/trigger_inputs.h"
#include "platform/trigger_outputs.h"
#include "platform/buttons.h"
#include "platform/encoders.h"
#include "platform/drivers/weegfx.h"

static platform::Display                                   display;
static platform::CVInputs<tu::CVInputTraits>               cv_inputs;
static platform::TriggerInputs<tu::TriggerInputTraits>     trig_in;
static platform::TriggerOutputs<tu::TriggerOutputTraits>   trig_out;
static platform::Buttons<tu::ButtonTraits>                 buttons;
static platform::Encoders<tu::EncoderTraits>               encoders;
static weegfx::Graphics gfx;

static IntervalTimer core_timer;
static IntervalTimer ui_timer;

static volatile uint32_t tick    = 0;
static volatile int32_t  enc0_count = 0;
static volatile int32_t  enc1_count = 0;

void FASTRUN core_isr() {
    display.flush();
    trig_out.flush();
    display.update();
    cv_inputs.scan();
    trig_in.scan();
    trig_out.write(2, bool((tick >> 13) & 1));
    ++tick;
}

void ui_isr() {
    buttons.scan();
    encoders.scan();
    enc0_count += encoders.get(0).delta;
    enc1_count += encoders.get(1).delta;
}

int main() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWriteFast(LED_BUILTIN, HIGH);
    delay(100);

    cv_inputs.init();
    trig_in.init();
    trig_out.init();
    buttons.init();
    encoders.init();
    platform::spi0_init<tu::SpiTraits>();
    display.init(tu::DisplayTraits::kDC,
                 tu::DisplayTraits::kRST,
                 tu::DisplayTraits::kCS);

    digitalWriteFast(LED_BUILTIN, LOW);

    core_timer.begin(core_isr, 60);
    core_timer.priority(80);
    ui_timer.begin(ui_isr, 1000);
    ui_timer.priority(128);

    uint32_t last_frame = 0;

    while (true) {
        if (display.begin_frame()) {
            // snapshot atomically
            const int32_t e0 = enc0_count;
            const int32_t e1 = enc1_count;
            const auto b0 = buttons.get(0);
            const auto b1 = buttons.get(1);

            gfx.Begin(display.frame_buffer(), true);

            gfx.setPrintPos(0, 0);
            gfx.print("encoder test");

            gfx.setPrintPos(0, 12);
            gfx.print("enc0:");
            gfx.print(static_cast<int>(e0));
            gfx.print(" enc1:");
            gfx.print(static_cast<int>(e1));

            gfx.setPrintPos(0, 24);
            gfx.print("btn:");
            gfx.print(b0.pressed ? "1" : "0");
            gfx.print(b1.pressed ? "1" : "0");
            gfx.print(" cv:");
            gfx.print(static_cast<int>(cv_inputs.get_smoothed(0)));

            gfx.setPrintPos(0, 36);
            gfx.print("frame:");
            gfx.print(static_cast<int>(last_frame));

            gfx.End();
            display.end_frame();
            ++last_frame;
        }
    }
}
