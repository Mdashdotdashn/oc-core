#pragma once

#include <array>
#include <cstdint>

#include "oc/app.h"
#include "platform/drivers/weegfx.h"

#include "pages/cpu_page.h"
#include "pages/cv_inputs_page.h"
#include "pages/encoder_page.h"
#include "pages/output_page.h"
#include "pages/page_app.h"
#include "pages/trigger_page.h"

template <typename RuntimeT>
class TriggerToCV : public oc::Application {
private:
    static constexpr uint8_t kPageCount = 5;
    static constexpr uint8_t kProfileAverageShift = 4;

    struct PageStat {
        uint32_t avg_cycles = 0;
        uint32_t max_cycles = 0;
    };

public:
    explicit TriggerToCV(RuntimeT& runtime)
        : cpu_page_(runtime) {
        pages_ = {
            &encoder_page_,
            &cv_inputs_page_,
            &output_page_,
            &trigger_page_,
            &cpu_page_,
        };
    }

    void init() override {
        current_page_ = kPageCount - 1;
        for (auto& stat : page_stats_) {
            stat = {};
        }
        for (auto* page : pages_) {
            page->init();
        }
        pages_[current_page_]->reset();
    }

    void audio_callback(const oc::Inputs& in, oc::Outputs& out) override {
        if (in.buttons[0].just_pressed) {
            switch_page((current_page_ + kPageCount - 1) % kPageCount);
        }
        if (in.buttons[1].just_pressed) {
            switch_page((current_page_ + 1) % kPageCount);
        }

        const uint8_t active_page = current_page_;
        const uint32_t page_start_cycles = current_cycle_count();
        pages_[active_page]->audio_callback(in, out);
        const uint32_t page_elapsed_cycles = current_cycle_count() - page_start_cycles;

        auto& page_stat = page_stats_[active_page];
        update_average(page_stat.avg_cycles, page_elapsed_cycles);
        if (page_elapsed_cycles > page_stat.max_cycles) {
            page_stat.max_cycles = page_elapsed_cycles;
        }
    }

    void idle() override {
        pages_[current_page_]->idle();
    }

    void draw(oc::Display* display) override {
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

    static uint32_t current_cycle_count() {
#if defined(ARM_DWT_CYCCNT)
        return ARM_DWT_CYCCNT;
#else
        return 0;
#endif
    }

    static void update_average(uint32_t& average_cycles, uint32_t sample_cycles) {
        int32_t avg_cycles = static_cast<int32_t>(average_cycles);
        avg_cycles += (static_cast<int32_t>(sample_cycles) - avg_cycles) >> kProfileAverageShift;
        average_cycles = static_cast<uint32_t>(avg_cycles);
    }

    weegfx::Graphics gfx_;
    oc_test_pages::EncoderPage encoder_page_;
    oc_test_pages::CVInputsPage cv_inputs_page_;
    oc_test_pages::OutputPage output_page_;
    oc_test_pages::TriggerPage trigger_page_;
    oc_test_pages::CpuPage<RuntimeT> cpu_page_;
    std::array<oc_test_pages::PageApp*, kPageCount> pages_{};

    std::array<PageStat, kPageCount> page_stats_{};
    uint8_t current_page_ = 0;
};
