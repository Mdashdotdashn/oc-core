/// Convenience single-include for Teensy 3.2 platform drivers.
/// This header must remain platform-only and not depend on oc/* headers.
///
///   #include "platform/all.h"

#pragma once

#include "platform/buttons.h"
#include "platform/cv_inputs.h"
#include "platform/cv_outputs.h"
#include "platform/display.h"
#include "platform/encoders.h"
#include "platform/spi0_init.h"
#include "platform/storage.h"
#include "platform/timer.h"
#include "platform/trigger_inputs.h"
