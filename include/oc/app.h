#pragma once
#include <cstdint>
#include <array>
#include "oc/hal/display.h"

/// oc-core: User Application Interface
///
/// This is the only header a user algorithm needs to include.
/// It defines AudioIn (inputs from hardware each ISR cycle),
/// AudioOut (outputs the user writes to each cycle), and the
/// Application base class with the two-method contract:
///
///   audio_callback(in, out)   — real-time, called every 100µs
///   idle()                    — background, called from while(1)
///
/// Usage:
///   #include "oc/app.h"
///
///   class MyAlgorithm : public oc::Application {
///   public:
///       void init() override { ... }
///       void audio_callback(const oc::AudioIn& in, oc::AudioOut& out) override { ... }
///       void idle() override { ... }
///   };

namespace oc {

/// Debounced state of a single button for one ISR cycle.
/// just_pressed / just_released fire for exactly one cycle on edge transitions.
struct ButtonState {
    bool pressed;       ///< True while held (all recent samples confirm pressed)
    bool just_pressed;  ///< True for exactly one cycle on press edge
    bool just_released; ///< True for exactly one cycle on release edge
};

/// Hardware input snapshot for one audio ISR cycle.
struct AudioIn {
    /// Calibrated CV values from ADC (signed, scaled to pitch CV units).
    /// Use these for pitch/frequency calculations.
    std::array<int32_t, 4> cv;

    /// Raw 12-bit ADC readings (0–4095) before calibration.
    /// Use these if you need the raw voltage ratio.
    std::array<uint32_t, 4> cv_raw;

    /// Current logic level of each gate/trigger input (true = high).
    std::array<bool, 4> gate;

    /// Bitmask of gate channels that had a rising edge this cycle.
    /// Bit N is set when gate[N] just went high.
    /// Example: if (in.gate_edges & (1 << 2)) { /* gate 3 triggered */ }
    uint32_t gate_edges;

    /// Front-panel push-buttons. [0] = UP (pin 5), [1] = DOWN (pin 4).
    /// Poll from audio_callback(); cache values in member vars if needed in idle().
    std::array<ButtonState, 2> buttons;

    /// Rotary encoders. [0] = LEFT, [1] = RIGHT.
    /// delta: +1 CW, -1 CCW, 0 no movement this cycle.
    struct EncoderState {
        int8_t delta;
        bool   click_pressed;
        bool   click_just_pressed;
        bool   click_just_released;
    };
    std::array<EncoderState, 2> encoders;
};

/// Output values to drive from one audio ISR cycle.
struct AudioOut {
    /// DAC output values for each channel (0–65535, 16-bit).
    /// Maps to 0–10V on the O&C hardware (after calibration).
    std::array<uint16_t, 4> cv;

    /// Set a channel by voltage (-3V to +6V).
    ///
    /// kZeroVoltCode and kCodesPerVolt are derived from the default O&C
    /// calibration table. kZeroVoltCode may need trimming per-board —
    /// run the O&C calibration to get accurate values, or adjust empirically:
    ///   measured_error_volts * kCodesPerVolt → add to kZeroVoltCode.
    ///
    /// Default (theoretical):  kZeroVoltCode = 19661
    /// Empirical trim example: measured 0V→-0.75V, so +4915 → 24576
    ///
    /// Do NOT call from audio_callback() on Teensy 3.2 — software float
    /// is too slow for the 100µs ISR budget.
    void set_cv(uint8_t ch, float volts,
                float zero_code = 19661.0f,
                float codes_per_volt = 6553.5f) {
        const float raw = zero_code + volts * codes_per_volt;
        if (raw <= 0.0f)     { cv[ch] = 0;     return; }
        if (raw >= 65535.0f) { cv[ch] = 65535; return; }
        cv[ch] = static_cast<uint16_t>(raw);
    }
};

/// Base class for user algorithms.
///
/// Subclass this and implement audio_callback(). Optionally override init()
/// and idle(). Everything stays in your class — no global state needed.
class Application {
public:
    virtual ~Application() = default;

    /// Called once from main() before audio starts.
    /// Initialize oscillators, load presets, set defaults, etc.
    virtual void init() {}

    /// Called every 100µs from the hardware ISR (~10 kHz).
    ///
    /// Read inputs from `in`, write outputs to `out`.
    /// MUST be deterministic and complete well under 100µs.
    /// No heap allocation, no blocking I/O, no long loops.
    virtual void audio_callback(const AudioIn& in, AudioOut& out) = 0;

    /// Called from the main() while(1) loop as fast as possible.
    ///
    /// Safe for: parameter updates, UI, display, file I/O, Serial.
    /// Not timing-critical — will be preempted by the ISR timer.
    virtual void idle() {}

    /// Return true if this app uses the shared OLED display path.
    /// Display-enabled apps use a slightly different ISR ordering so the next
    /// OLED DMA page starts early and DAC writes are staged one control tick
    /// ahead to preserve SPI timing margin.
    virtual bool uses_display() const { return false; }

    /// Optional OLED drawing hook, called from the background loop.
    /// Default implementation does nothing.
    virtual void draw(hal::DisplayInterface* /*display*/) {}

    /// Called when the system is shutting down cleanly.
    virtual void shutdown() {}
};

} // namespace oc
