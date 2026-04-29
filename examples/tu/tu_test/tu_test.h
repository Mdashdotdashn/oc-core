#pragma once

#include <array>
#include <cstdint>

#include "tu/app.h"
#include "platform/drivers/weegfx.h"

#include "pages/button_page.h"
#include "pages/cpu_page.h"
#include "pages/cv_inputs_page.h"
#include "pages/encoder_page.h"
#include "pages/output_page.h"
#include "pages/page_app.h"
#include "pages/trigger_page.h"

template <typename RuntimeT>
class TuTestApp : public tu::Application {
private:
    static constexpr uint8_t kPageCount = 6;

public:
    explicit TuTestApp(RuntimeT& runtime)
        : cpu_page_(runtime) {
        pages_ = {
            &encoder_page_,
            &button_page_,
            &cv_inputs_page_,
            &output_page_,
            &trigger_page_,
            &cpu_page_,
        };
    }

    void init() override {
        current_page_ = kPageCount - 1;
        for (auto* page : pages_) {
            page->init();
        }
        pages_[current_page_]->reset();
    }

    void ui_callback(const std::array<tu::ButtonState, 2>& buttons,
                     const std::array<tu::EncoderState, 2>& encoders) override {
        if (buttons[0].just_pressed) {
            switch_page((current_page_ + kPageCount - 1) % kPageCount);
        }
        if (buttons[1].just_pressed) {
            switch_page((current_page_ + 1) % kPageCount);
        }
        pages_[current_page_]->ui_callback(buttons, encoders);
    }

    void audio_callback(const tu::Application::Input& in, tu::Outputs& out) override {
        pages_[current_page_]->audio_callback(in, out);
    }

    void idle() override {
        pages_[current_page_]->idle();
    }

    void draw(tu::Display* display) override {
        if (!display->begin_frame()) {
            return;
        }

        gfx_.Begin(display->frame_buffer(), true);
        pages_[current_page_]->draw_body(gfx_);
        draw_header();
        draw_footer();
        gfx_.End();
        display->end_frame();
    }

private:
    void switch_page(uint8_t next_page) {
        if (next_page == current_page_) {
            return;
        }
        current_page_ = next_page;
        pages_[current_page_]->reset();
    }

    void draw_header() {
        const char* title = pages_[current_page_]->title();
        const uint8_t title_len = string_length(title);
        const int16_t title_x = (128 - static_cast<int16_t>(title_len) * weegfx::Graphics::kFixedFontW) / 2;
        gfx_.setPrintPos(title_x < 0 ? 0 : title_x, 0);
        gfx_.print(title);
        gfx_.invertRect(0, 0, 128, 8);
    }

    void draw_footer() {
        static constexpr char kPrevLabel[] = " < ";
        static constexpr char kNextLabel[] = " > ";
        constexpr int16_t kLabelWidth = static_cast<int16_t>((sizeof(kPrevLabel) - 1) * weegfx::Graphics::kFixedFontW);
        constexpr int16_t next_x = 128 - kLabelWidth;

        gfx_.setPrintPos(0, 56);
        gfx_.print(kPrevLabel);
        gfx_.setPrintPos(next_x, 56);
        gfx_.print(kNextLabel);
        gfx_.invertRect(0, 56, kLabelWidth, 8);
        gfx_.invertRect(next_x, 56, kLabelWidth, 8);
    }

    static uint8_t string_length(const char* text) {
        uint8_t len = 0;
        while (text[len] != '\0') {
            ++len;
        }
        return len;
    }

    weegfx::Graphics gfx_;
    tu_test_pages::EncoderPage encoder_page_;
    tu_test_pages::ButtonPage button_page_;
    tu_test_pages::CVInputsPage cv_inputs_page_;
    tu_test_pages::OutputPage output_page_;
    tu_test_pages::TriggerPage trigger_page_;
    tu_test_pages::CpuPage<RuntimeT> cpu_page_;
    std::array<tu_test_pages::PageApp*, kPageCount> pages_{};

    uint8_t current_page_ = 0;
};
