

#include "dsp/formantshifter.h"
#include "dsp/common.h"
#include "dsp/effect.h"
#include "mengumath.h"
#include <algorithm>
#include <array>
#include <complex>
#include <cstdint>

using namespace Mengu;
using namespace dsp;

// rescale an array in the freqency domain by the shape of an envelope if it were to be shifted up or down
template<typename T>
static void _shift_by_env(const T *input, 
                          T *output, 
                          const float *envelope, 
                          const uint32_t size, 
                          const float shift_factor) {
    for (uint32_t i = 0; i < size; i++) {
        uint32_t shifted_ind =  i / shift_factor;
        if (shifted_ind < size) {
            float correction = envelope[i] != 0.0f ? envelope[shifted_ind] / envelope[i] : 0.0f;
            output[i] = correction * input[i];
        }
        else {
            // output[i] = input[size - 1];
            output[i] = Complex(0.0f);
        }
    }
}

// shift the envelope of a lpc frequency spectrum then 
void LPCFormantShifter::_reconstruct_freq(const float *residuals, 
                                const float *envelope, 
                                const float *phases,
                                Complex *output, 
                                const float env_shift_factor) {

        for (uint32_t i = 0; i < ProcSize / 2; i++) {
            uint32_t shifted_ind =  i / env_shift_factor;
            float env_mag = shifted_ind < ProcSize / 2 ? envelope[shifted_ind] : envelope[i];
            float new_mag = residuals[i] * env_mag;
            output[i] = std::polar(new_mag, phases[i]);
        }
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
    uint32_t size_remaining = size;
    
    std::array<Complex, ProcSize> freq_shifted {0};
    std::array<Complex, ProcSize> samples {0};
    
    
    while (size_remaining > 0) {
        uint32_t size_this_loop = MIN(ProcSize, size_remaining);
        
        // Load sample segment. 0 unused frames
        _raw_buffer.pop_front_many(samples.data(), size_this_loop);
        for (uint32_t i = size_this_loop; i < ProcSize; i++) {
            samples[i] = 0.0;
        }

        // do the shifty
        _lpc.load_sample(samples.data());
        auto &freqs = _lpc.get_freq_spectrum();
        _shift_by_env(
            _lpc.get_freq_spectrum().data(), 
            freq_shifted.data(), 
            _lpc.get_envelope().data(),
            ProcSize / 2, 
            _shift_factor
        );
        float phases[ProcSize / 2];
        for (uint32_t i = 0; i < ProcSize / 2; i++) {
            phases[i] = std::arg(freqs[i]);
        }
        // _reconstruct_freq(
        //     _lpc.get_residuals().data(), 
        //     _lpc.get_envelope().data(), 
        //     phases, 
        //     freq_shifted.data(), 
        //     _shift_factor
        // );


        _lpc.get_fft().inverse_transform(freq_shifted.data(), samples.data());

        // copy to output
        for (uint32_t i = 0; i < size_this_loop; i++) {
            output[size - size_remaining + i] = (2.0f * samples[i]); // 4? sqrt?
        }

        size_remaining -= size_this_loop;

    }

    return size;
}

// number of samples that can be output given the current pushed signals of the Effect
uint32_t LPCFormantShifter::n_transformed_ready() const {
    return _raw_buffer.size();
};

// resets state of effect to make it reading to take in a new sample
void LPCFormantShifter::reset() {}

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


 
