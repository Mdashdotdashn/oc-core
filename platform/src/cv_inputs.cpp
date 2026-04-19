#include "platform/cv_inputs.h"
#include <ADC.h>   // Teensy ADC library

namespace platform {

namespace {
    ::ADC adc_obj_;
}

CVInputs::CVInputs() {
    raw_.fill(0);
    smoothed_accumulator_.fill(0);
    smoothed_value_.fill(0);
}

void CVInputs::init() {
    adc_obj_.adc0->setAveraging(4);
    adc_obj_.adc0->setResolution(12);
    adc_obj_.adc0->setConversionSpeed(ADC_CONVERSION_SPEED::HIGH_SPEED);
    adc_obj_.adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::HIGH_SPEED);
    // Kick off the first conversion on channel 0
    adc_obj_.startSingleRead(kPins[0], ADC_0);
}

void CVInputs::scan() {
    // Read the result of the previous conversion
    if (adc_obj_.adc0->isComplete()) {
        const uint32_t raw = static_cast<uint32_t>(adc_obj_.readSingle(ADC_0));
        raw_[current_channel_] = raw;
        // Exponential moving average:  s = s - s>>k + raw
        smoothed_accumulator_[current_channel_] =
            smoothed_accumulator_[current_channel_] -
            (smoothed_accumulator_[current_channel_] >> kSmoothing) +
            raw;
        const uint32_t smoothed = smoothed_accumulator_[current_channel_] >> kSmoothing;
        smoothed_value_[current_channel_] = smoothed;
    }

    // Advance to the next channel and start a new conversion
    current_channel_ = (current_channel_ + 1) & 0x3;  // mod 4
    adc_obj_.startSingleRead(kPins[current_channel_], ADC_0);
}

uint32_t CVInputs::read_raw(uint8_t ch) const {
    return raw_[ch];
}

uint32_t CVInputs::get_smoothed(uint8_t ch) const {
    return smoothed_value_[ch];
}

} // namespace platform
