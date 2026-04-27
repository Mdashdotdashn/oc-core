#pragma once

#include <cstdint>

#include "platform/drivers/weegfx.h"

namespace oc_test_pages {

inline void print_voltage(weegfx::Graphics& gfx, int32_t millivolts) {
    int32_t centivolts = millivolts;
    if (centivolts >= 0) {
        centivolts = (centivolts + 5) / 10;
    } else {
        centivolts = (centivolts - 5) / 10;
    }

    if (centivolts >= 0) {
        gfx.print("+");
    } else {
        gfx.print("-");
        centivolts = -centivolts;
    }

    gfx.print(static_cast<int>(centivolts / 100));
    gfx.print(".");
    const int32_t fractional = centivolts % 100;
    if (fractional < 10) {
        gfx.print("0");
    }
    gfx.print(static_cast<int>(fractional));
    gfx.print("V");
}

inline void print_pattern_voltage(weegfx::Graphics& gfx, int8_t volts) {
    if (volts >= 0) {
        gfx.print("+");
    }
    gfx.print(static_cast<int>(volts));
    gfx.print("V");
}

} // namespace oc_test_pages
