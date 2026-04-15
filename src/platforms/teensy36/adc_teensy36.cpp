#include "adc_teensy36.h"
#include <ADC.h>   // Teensy ADC library

namespace oc::platform::teensy36 {

namespace {
    ADC adc_obj_;
}

ADCImpl::ADCImpl() {
    raw_.fill(0);
    smoothed_.fill(0);
    offsets_.fill(0);
}

void ADCImpl::init() {
    adc_obj_.setAveraging(4);
    adc_obj_.setResolution(12);
    adc_obj_.setConversionSpeed(ADC_CONVERSION_SPEED::HIGH_SPEED);
    adc_obj_.setSamplingSpeed(ADC_SAMPLING_SPEED::HIGH_SPEED);
    // Kick off the first conversion on channel 0
    adc_obj_.startSingleRead(kPins[0], ADC_0);
}

void ADCImpl::scan() {
    // Read the result of the previous conversion
    if (adc_obj_.isComplete(ADC_0)) {
        const uint32_t raw = static_cast<uint32_t>(adc_obj_.readSingle(ADC_0));
        raw_[current_channel_] = raw;
        // Exponential moving average:  s = s - s>>k + raw
        smoothed_[current_channel_] =
            smoothed_[current_channel_] -
            (smoothed_[current_channel_] >> kSmoothing) +
            raw;
    }

    // Advance to the next channel and start a new conversion
    current_channel_ = (current_channel_ + 1) & 0x3;  // mod 4
    adc_obj_.startSingleRead(kPins[current_channel_], ADC_0);
}

uint32_t ADCImpl::read_raw(uint8_t ch) const {
    return raw_[ch];
}

uint32_t ADCImpl::get_smoothed(uint8_t ch) const {
    return smoothed_[ch] >> kSmoothing;  // Shift back to 0-4095 range
}

int32_t ADCImpl::get_calibrated(uint8_t ch) const {
    // Calibration convention from OC_ADC: offset - smoothed_value
    return static_cast<int32_t>(offsets_[ch]) -
           static_cast<int32_t>(get_smoothed(ch));
}

void ADCImpl::set_calibration_offset(uint8_t channel, uint16_t offset) {
    offsets_[channel] = offset;
}

} // namespace oc::platform::teensy36
