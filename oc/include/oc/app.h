#pragma once

#include "oc/application.h"
#include "oc/button_state.h"
#include "oc/display_fwd.h"
#include "oc/encoder_state.h"
#include "oc/inputs.h"
#include "oc/outputs.h"

/// oc-core: User Application Interface
///
/// This is the only header a user algorithm needs to include.
/// It re-exports the user-facing API types:
///   - ButtonState
///   - EncoderState
///   - Inputs
///   - Outputs
///   - Application
///
///   audio_callback(in, out)   — real-time, called every 100µs
///   idle()                    — background, called from while(1)
///   draw(display)             — background OLED rendering into a backbuffer
///
/// Usage:
///   #include "oc/app.h"
///
///   class MyAlgorithm : public oc::Application {
///   public:
///       void init() override { ... }
///       void audio_callback(const oc::Inputs& in, oc::Outputs& out) override { ... }
///       void idle() override { ... }
///   };
