#pragma once

#include <array>
#include <cstdint>

#include "oc/app.h"
#include "oc/calibration.h"
#include "platform/drivers/weegfx.h"

template <typename RuntimeT>
class TriggerToCV : public oc::Application {
private:
    static constexpr uint8_t kPageCount = 5;
    static constexpr uint8_t kProfileAverageShift = 4;

    struct PageStat {
        uint32_t avg_cycles = 0;
        uint32_t max_cycles = 0;
    };

    class PageApp : public oc::Application {
    public:
        explicit PageApp(const char* title) : title_(title) {}
        const char* title() const { return title_; }
        virtual void draw_body(weegfx::Graphics& gfx) = 0;

    private:
        const char* title_;
    };

    class EncoderPage final : public PageApp {
    public:
        EncoderPage() : PageApp("encoders") {}

        void init() override { reset(); }

        void reset() override {
            left_val_ = 0;
            right_val_ = 0;
            left_click_ = false;
            right_click_ = false;
        }

        void audio_callback(const oc::Inputs& in, oc::Outputs& /*out*/) override {
            left_val_ += in.encoders[0].delta;
            right_val_ += in.encoders[1].delta;
            left_click_ = in.encoders[0].click_pressed;
            right_click_ = in.encoders[1].click_pressed;
        }

        void draw_body(weegfx::Graphics& gfx) override {
            gfx.setPrintPos(0, 14);
            gfx.print("L:");
            gfx.print(left_val_);

            gfx.setPrintPos(64, 14);
            gfx.print("R:");
            gfx.print(right_val_);

            gfx.setPrintPos(0, 28);
            gfx.print("LC:");
            gfx.print(left_click_ ? "1" : "0");

            gfx.setPrintPos(64, 28);
            gfx.print("RC:");
            gfx.print(right_click_ ? "1" : "0");
        }

    private:
        int32_t left_val_ = 0;
        int32_t right_val_ = 0;
        bool left_click_ = false;
        bool right_click_ = false;
    };

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

    class OutputPage final : public PageApp {
    public:
        OutputPage() : PageApp("output") {}

        void init() override { reset(); }

        void reset() override {
            pattern_phase_ = 0;
            pattern_tick_counter_ = 0;
            output_volts_ = {-3, 0, 1, 2};
        }

        void audio_callback(const oc::Inputs& /*in*/, oc::Outputs& out) override {
            if (++pattern_tick_counter_ >= kPatternStepTicks) {
                pattern_tick_counter_ = 0;
                pattern_phase_ = static_cast<uint8_t>((pattern_phase_ + 1) & 0x3);
            }

            for (uint8_t i = 0; i < 4; ++i) {
                const uint8_t pattern_index = static_cast<uint8_t>((i + pattern_phase_) & 0x3);
                const int8_t volts = kPatternVolts[pattern_index];
                output_volts_[i] = volts;
                out.cv[i] = oc::calibration::volts_to_dac(i, static_cast<float>(volts));
            }
        }

        void draw_body(weegfx::Graphics& gfx) override {
            gfx.setPrintPos(0, 14);
            gfx.print("OUTA:");
            print_pattern_voltage(gfx, output_volts_[0]);

            gfx.setPrintPos(64, 14);
            gfx.print("OUTB:");
            print_pattern_voltage(gfx, output_volts_[1]);

            gfx.setPrintPos(0, 28);
            gfx.print("OUTC:");
            print_pattern_voltage(gfx, output_volts_[2]);

            gfx.setPrintPos(64, 28);
            gfx.print("OUTD:");
            print_pattern_voltage(gfx, output_volts_[3]);

            gfx.setPrintPos(0, 42);
            gfx.print("step:");
            gfx.print(static_cast<int>(pattern_phase_ + 1));
            gfx.print("/4");
        }

    private:
        static constexpr uint32_t kPatternStepTicks = 20000;
        static constexpr std::array<int8_t, 4> kPatternVolts = {-3, 0, 1, 2};

        uint8_t pattern_phase_ = 0;
        uint32_t pattern_tick_counter_ = 0;
        std::array<int8_t, 4> output_volts_ = {-3, 0, 1, 2};
    };

    class TriggerPage final : public PageApp {
    public:
        TriggerPage() : PageApp("trigger") {}

        void init() override { reset(); }

        void reset() override {
            gates_ = {false, false, false, false};
        }

        void audio_callback(const oc::Inputs& in, oc::Outputs& /*out*/) override {
            gates_ = in.gate;
        }

        void draw_body(weegfx::Graphics& gfx) override {
            draw_trigger_value(gfx, "INA", gates_[0], 0, 14);
            draw_trigger_value(gfx, "INB", gates_[1], 64, 14);
            draw_trigger_value(gfx, "INC", gates_[2], 0, 28);
            draw_trigger_value(gfx, "IND", gates_[3], 64, 28);
        }

    private:
        static void draw_trigger_value(weegfx::Graphics& gfx, const char* label, bool value, int16_t x, int16_t y) {
            gfx.setPrintPos(x, y);
            gfx.print(label);
            gfx.print(":");
            gfx.print(value ? "1" : "0");
        }

        std::array<bool, 4> gates_ = {false, false, false, false};
    };

    class CpuPage final : public PageApp {
    public:
        CpuPage(RuntimeT& runtime,
                const std::array<PageStat, kPageCount>& page_stats,
                const uint8_t& active_page)
            : PageApp("cpu"),
              runtime_(runtime),
              page_stats_(page_stats),
              active_page_(active_page) {}

        void audio_callback(const oc::Inputs& /*in*/, oc::Outputs& /*out*/) override {}

        void draw_body(weegfx::Graphics& gfx) override {
            const auto profile = runtime_.isr_profile();

            gfx.setPrintPos(0, 14);
            gfx.print("TOT ");
            print_bucket(gfx, profile.total_cycles);

            gfx.setPrintPos(0, 22);
            gfx.print("APP ");
            print_bucket(gfx, profile.app_cycles);

            gfx.setPrintPos(0, 30);
            gfx.print("DSP ");
            print_bucket(gfx, profile.display_cycles);

            gfx.setPrintPos(0, 38);
            gfx.print("SCN ");
            print_bucket(gfx, profile.scan_cycles);

            gfx.setPrintPos(0, 46);
            gfx.print("P");
            gfx.print(static_cast<int>(active_page_));
            gfx.print(" ");
            print_avg_max_us(gfx, page_stats_[active_page_].avg_cycles, page_stats_[active_page_].max_cycles);
        }

    private:
        void print_bucket(weegfx::Graphics& gfx, uint32_t cycles) {
            gfx.print(static_cast<int>(runtime_.cycles_to_load_percent(cycles)));
            gfx.print("% ");
            gfx.print(static_cast<int>(runtime_.cycles_to_us(cycles)));
            gfx.print("u");
        }

        void print_avg_max_us(weegfx::Graphics& gfx, uint32_t avg_cycles, uint32_t max_cycles) {
            gfx.print(static_cast<int>(runtime_.cycles_to_us(avg_cycles)));
            gfx.print("/");
            gfx.print(static_cast<int>(runtime_.cycles_to_us(max_cycles)));
            gfx.print("u");
        }

        RuntimeT& runtime_;
        const std::array<PageStat, kPageCount>& page_stats_;
        const uint8_t& active_page_;
    };

public:
    explicit TriggerToCV(RuntimeT& runtime)
        : cpu_page_(runtime, page_stats_, current_page_) {
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
    static void print_voltage(weegfx::Graphics& gfx, int32_t millivolts) {
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

    static void print_pattern_voltage(weegfx::Graphics& gfx, int8_t volts) {
        if (volts >= 0) {
            gfx.print("+");
        }
        gfx.print(static_cast<int>(volts));
        gfx.print("V");
    }

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
    EncoderPage encoder_page_;
    CVInputsPage cv_inputs_page_;
    OutputPage output_page_;
    TriggerPage trigger_page_;
    CpuPage cpu_page_;
    std::array<PageApp*, kPageCount> pages_{};

    std::array<PageStat, kPageCount> page_stats_{};
    uint8_t current_page_ = 0;
};
