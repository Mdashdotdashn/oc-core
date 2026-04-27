#pragma once

#include <array>
#include <cstdint>

#include "pages/page_app.h"
#include "pages/page_ui_utils.h"

namespace oc_test_pages {

class CVInputsPage final : public PageApp {
public:
    CVInputsPage() : PageApp("cv inputs") {}

    void init() override { reset(); }

    void reset() override {
        cv_ = {0, 0, 0, 0};
    }

    void audio_callback(const oc::Inputs& in, oc::Outputs& /*out*/) override {
        cv_ = in.cv;
    }

    void draw_body(weegfx::Graphics& gfx) override {
        draw_cv_input(gfx, 0, 0, 14, cv_[0]);
        draw_cv_input(gfx, 1, 64, 14, cv_[1]);
        draw_cv_input(gfx, 2, 0, 28, cv_[2]);
        draw_cv_input(gfx, 3, 64, 28, cv_[3]);
    }

private:
    static void draw_cv_input(weegfx::Graphics& gfx, uint8_t channel, int16_t x, int16_t y, int32_t millivolts) {
        gfx.setPrintPos(x, y);
        gfx.print("CV");
        gfx.print(static_cast<int>(channel + 1));
        gfx.print(":");
        print_voltage(gfx, millivolts);
    }

    std::array<int32_t, 4> cv_ = {0, 0, 0, 0};
};

} // namespace oc_test_pages
