

#include "dsp/formantshifter.h"
#include "dsp/common.h"
#include "dsp/effect.h"
#include "dsp/interpolation.h"
#include "dsp/loudness.h"
#include "mengumath.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstdint>

using namespace Mengu;
using namespace dsp;



LPCFormantShifter::LPCFormantShifter() {
    _transformed_buffer.resize(OverlapSize);
}

Effect::InputDomain LPCFormantShifter::get_input_domain() {
    return InputDomain::Time;
};


// push new value of signal
void LPCFormantShifter::push_signal(const Complex *input, const uint32_t &size) {
    _raw_buffer.extend_back(input, size);
}

// Last value of transformed signal
uint32_t LPCFormantShifter::pop_transformed_signal(Complex *output, const uint32_t &size) {    
    std::array<Complex, ProcSize> freq_shifted {0};
    std::array<Complex, ProcSize> samples {0};
    std::array<Complex, ProcSize> shifted_samples {0};

    while (_raw_buffer.size() >= ProcSize && _transformed_buffer.size() < size + OverlapSize) {
        
        // Load sample segment. 0 unused frames
        _raw_buffer.to_array(samples.data(), ProcSize);

        // do the shifty
        _lpc.load_sample(samples.data());
        _shift_by_env(
            _lpc.get_freq_spectrum().data(), 
            freq_shifted.data(), 
            _lpc.get_envelope().data(),
            _shift_factor
        );
        _lpc.get_fft().inverse_transform(freq_shifted.data(), shifted_samples.data());

        // Make downward shifts not quieter and upward shifts not louder
        // Automatically adjusts for the fact that only half of the frequency spectrum is used
        _rescale_shifted_freqs(samples.data(), shifted_samples.data());

        // copy to output
        mix_and_extend(_transformed_buffer, shifted_samples, OverlapSize, hamming_window);

        _raw_buffer.pop_front_many(nullptr, HopSize);

    }

    if (_transformed_buffer.size() < size + OverlapSize) {
        // Not enough samples to output anything
        return 0;
    }
    else {
        return _transformed_buffer.pop_front_many(output, size);
    }
}

// number of samples that can be output given the current pushed signals of the Effect
uint32_t LPCFormantShifter::n_transformed_ready() const {
    return _raw_buffer.size();
};

// resets state of effect to make it reading to take in a new sample
void LPCFormantShifter::reset() {
    _raw_sample_filter.reset();
    _shifted_sample_filter.reset();
}

// The properties that this Effect exposes to be changed by GUI. 
// The index that they are put in is considered the props id
std::vector<EffectPropDesc> LPCFormantShifter::get_property_descs() const {
    return {
        EffectPropDesc {
            .type = EffectPropType::Slider,
            .name = "Formant Shift",
            .desc = "Scales the formant of pushed signals by this amount",
            .slider_data = {
                .min_value = 0.5,
                .max_value = 2,
                .scale = Exp,
            }
        }
    };
}

// Sets a property with the specified id the value declared in the payload
void LPCFormantShifter::set_property(uint32_t id, EffectPropPayload data) {
    switch (id) {
        case 0:
            if (data.type == Slider) {
                _shift_factor = data.value;
            }
            break;
        default:
            break;
    }
}


// Gets the value of a property with the specified id
EffectPropPayload LPCFormantShifter::get_property(uint32_t id) const {
    return EffectPropPayload {
        .type = Slider,
        .value = _shift_factor,
    };
};

// rescale an array in the freqency domain by the shape of an envelope if it were to be shifted up or down
void LPCFormantShifter::_shift_by_env(const Complex *input, 
                          Complex *output, 
                          const float *envelope, 
                          const float shift_factor) {
    // Calculate the Loudness (in LUFS) as we do the shift so we can correct it afterward

    for (uint32_t i = 0; i < ProcSize / 2; i++) {
        uint32_t shifted_ind =  i / shift_factor;
        if (shifted_ind < ProcSize / 2) {
            float correction = envelope[shifted_ind] / envelope[i];
            if (!std::isfinite(correction)) {
                output[i] = Complex(0.0f);
            }
            else {
                output[i] = correction * input[i];
            }
        }
        else {
            // output[i] = input[size - 1];
            output[i] = Complex(0.0f);
        }
    }
}

void LPCFormantShifter::_rescale_shifted_freqs(const Complex *raw_sample, Complex *shifted_sample) {
    float filtered_raw[ProcSize];
    float filtered_shifted[ProcSize];
    for (uint32_t i = 0; i < ProcSize; i++) {
        filtered_raw[i] = raw_sample[i].real();
        filtered_shifted[i] = shifted_sample[i].real();
    }

    // perform filter
    // _raw_sample_filter.transform(filtered_raw, filtered_raw, ProcSize);
    // _shifted_sample_filter.transform(filtered_shifted, filtered_shifted, ProcSize);

    // Get the (unormalized) power of each filtered sample
    float raw_power, shifted_power = 0.0f;
    for(uint32_t i = 0; i < ProcSize; i++) {
        raw_power += filtered_raw[i] * filtered_raw[i];
        shifted_power += filtered_shifted[i] * filtered_shifted[i];
    }
    float correction = sqrt(raw_power / shifted_power);
    if (!isfinite(correction)) {
        // abort if scaling cannot be done
        return;
    }
    else {
        // otherwise scale the shifted by the correction
        for (uint32_t i = 0; i < ProcSize; i++) {
            shifted_sample[i] *= correction;
        }
    }
    
}
 
