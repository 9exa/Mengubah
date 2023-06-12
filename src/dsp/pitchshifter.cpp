#include "dsp/common.h"
#include "dsp/effect.h"
#include "dsp/timestretcher.h"
#include "dsp/sampling.h"
#include "fft.h"
#include "interpolation.h"
#include "templates/vecdeque.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstdint>
#include <dsp/pitchshifter.h>
#include <dsp/singletons.h>
#include <dsp/interpolation.h>
#include <iterator>
#include <mengumath.h>
#include <iostream>
#include <vector>

using namespace Mengu;
using namespace dsp;


std::vector<EffectPropDesc> PitchShifter::get_property_descs() const {
    return {
        EffectPropDesc {
            .type = EffectPropType::Slider,
            .name = "Pitch Shift",
            .desc = "Scales the pitch of pushed signals by this amount",
            .slider_data = {
                .min_value = 0.5,
                .max_value = 2,
                .scale = Exp,
            }
        }
    };
}

void PitchShifter::set_property(uint32_t id, EffectPropPayload data) {
    if (id == 0) {
        if (data.type == Slider) {
            set_shift_factor(data.value);
        }
    }
}

EffectPropPayload PitchShifter::get_property(uint32_t id) const {
    if (id == 0) {
        return EffectPropPayload {
            .type = Slider,
            .value = _shift_factor,
        };
    }

    return EffectPropPayload {
        .type = Slider,
        .value = 0.0f,
    };
}

PhaseVocoderPitchShifterV2::PhaseVocoderPitchShifterV2() {
    _transformed_buffer.resize(ProcSize);
}

PhaseVocoderPitchShifterV2::~PhaseVocoderPitchShifterV2() {}

void PhaseVocoderPitchShifterV2::push_signal(const Complex *input, const uint32_t &size) {
    _raw_buffer.extend_back(input, size);
}

uint32_t PhaseVocoderPitchShifterV2::pop_transformed_signal(Complex *output, const uint32_t &size) {
    while ((_transformed_buffer.size() < size + OverlapSize) && (_raw_buffer.size() >= ProcSize)) {
        std::array<Complex, ProcSize> samples;
        _raw_buffer.to_array(samples.data(), ProcSize);



        _lpc.load_sample(samples.data());
        const std::array<float, ProcSize> &envelope = _lpc.get_envelope();
        const std::array<float, ProcSize> &residuals = _lpc.get_residuals();
        const std::array<Complex, ProcSize> &frequencies = _lpc.get_freq_spectrum();

        
        // std::array<float, ProcSize / 2> log_envelope{};
        std::array<float, ProcSize / 2> log_residuals{};
        std::transform(residuals.cbegin(), residuals.cend(), log_residuals.begin(), [] (float f) {
            return log2(f);
        });

        std::array<float, ProcSize / 2> args{};
        std::transform(frequencies.cbegin(), frequencies.cend(), args.begin(), [] (Complex freq) {
            // shifted to be positive to make lerping easier
            return std::arg(freq);
        });
        /*
        // resample in the log-frequency domain
        std::array<Complex, ProcSize / 2> new_freq{};
        if (_shift_factor > 1.0f) {
            linear_resample_no_filter(log_residuals.data(), log_residuals.data(), ProcSize, _shift_factor, -2e5f);
            linear_resample_no_filter(args.data(), args.data(), ProcSize, _shift_factor, 0.0f);
        }
        else {
            linear_resample_no_filter(log_residuals.data(), log_residuals.data(), ProcSize, _shift_factor, -2e5f);
            linear_resample_no_filter(args.data(), args.data(), ProcSize, _shift_factor, 0.0f);
            // 0 the rest
            for (uint32_t i = ProcSize * _shift_factor; i < ProcSize; i++) {
                log_residuals[i] = -2e5f;
                args[i] = 0.0f;
            }
        }

        std::transform(log_residuals.cbegin(), log_residuals.cend(), new_freq.begin(), [] (float f) {
            return exp2(f);
        });

        std::transform(new_freq.cbegin(), new_freq.cend(), envelope.cbegin(), new_freq.begin(),
            [] (Complex resid, float env) {
                return resid * env;
            }
        );
        

        // rescale the frequencies
        float max_base_freq = 0.0f;
        float max_new_freq = 0.0f;
        for (uint32_t i = 0; i < ProcSize / 2; i++) {
            float freq_amp = std::norm(frequencies[i]);
            if (max_base_freq < freq_amp) { max_base_freq = freq_amp; }
            if (max_new_freq < new_freq[i].real()) { max_new_freq = new_freq[i].real(); }
        }
        max_base_freq = std::sqrt(max_base_freq); // rooting delayed until after the loop
        std::transform(new_freq.cbegin(), new_freq.cend(), new_freq.begin(),
            [max_base_freq, max_new_freq] (Complex freq) {
                return freq * max_base_freq / max_new_freq;
            }
        );

        // adjust phases
        for (uint32_t i = 0; i < ProcSize / 2; i++) {
            // new_freq[i] = std::polar(std::sqrt(std::norm(frequencies[i])), std::arg(frequencies[i]));
            new_freq[i] = std::polar(new_freq[i].real(), args[i]);
            // new_freq[i] = std::polar(new_freq[i].real(), (float) -MATH_TAU * (_samples_processed * i) / ProcSize);
        }
        */

        
        std::array<Complex, ProcSize / 2> new_freq{};
        std::array<float, ProcSize / 2> freq_mags{};
        std::transform(frequencies.cbegin(), frequencies.cend(), freq_mags.begin(), [] (Complex c) {
            return std::sqrt(std::norm(c));
        });
        if (_shift_factor > 1.0f) {
            linear_resample_no_filter(frequencies.data(), new_freq.data(), ProcSize / 2, _shift_factor);
            linear_resample_no_filter(freq_mags.data(), freq_mags.data(), ProcSize / 2, _shift_factor);
            // linear_resample_no_filter(args.data(), args.data(), ProcSize, _shift_factor);
        }
        else {
            linear_resample_no_filter(frequencies.data(), new_freq.data(), ProcSize / 2, _shift_factor);
            linear_resample_no_filter(freq_mags.data(), freq_mags.data(), ProcSize / 2, _shift_factor);
            // linear_resample_no_filter(args.data(), args.data(), ProcSize, _shift_factor);
            // 0 the rest
            for (uint32_t i = ProcSize * _shift_factor; i < ProcSize; i++) {
                freq_mags[i] = 0.0f;
                // args[i] = 0.0f;
            }
        }
        // std::transform(freq_mags.cbegin(), freq_mags.cend(), new_freq.cbegin(), new_freq.begin(),
        //     [] (float mag, Complex old_freq) { return old_freq / std::sqrt(std::norm(old_freq)) * mag; }
        // );
        
        // scale by formants
        // float envelope_max = -2e5;
        // float new_freq_max = -2e5;
        // float scaled_freq_max = -2e5;
        // for (uint32_t i = 0; i < ProcSize / 2; i++) {
        //     if (envelope_max < envelope[i]) { envelope_max = envelope[i]; }
        //     if (new_freq_max < std::norm(new_freq[i])) { new_freq_max = std::norm(new_freq[i]); }
        // }
        // new_freq_max = std::sqrt(new_freq_max);
        // for (uint32_t i = 0; i < ProcSize / 2; i++) {
        //     new_freq[i] = new_freq[i] * envelope[i] / envelope_max;
        //     if (scaled_freq_max < std::norm(new_freq[i])) { scaled_freq_max = std::norm(new_freq[i]); }
        // }
        // scaled_freq_max = std::sqrt(scaled_freq_max);
        // for (uint32_t i = 0; i < ProcSize / 2; i++) {
        //     new_freq[i] *= new_freq_max / scaled_freq_max;
        // }
        

        _lpc.get_fft().inverse_transform(new_freq.data(), samples.data());

        mix_and_extend(_transformed_buffer, samples, OverlapSize, hann_window);
        
        _samples_processed += ProcSize - OverlapSize;
        _raw_buffer.pop_front_many(nullptr, ProcSize - OverlapSize);
    }

    uint32_t n = _transformed_buffer.pop_front_many(output, size);
    return n;
}

uint32_t PhaseVocoderPitchShifterV2::n_transformed_ready() const {
    return _transformed_buffer.size() - ProcSize;
}

void PhaseVocoderPitchShifterV2::reset() {
    _transformed_buffer.resize(ProcSize, Complex(0.0f));
}

TimeStretchPitchShifter::TimeStretchPitchShifter(TimeStretcher *stretcher, uint32_t nchannels):
    _stretcher(stretcher),
    _resampler(nchannels, 1.0f) {
    _stretcher->set_stretch_factor(1.0f);
    _shift_factor = 1.0f;

    // Complex zeros[MinResampleInputSize] = {Complex()};
    // _stretcher.push_signal(zeros, MinResampleInputSize);
    // _pitch_shifting_stretcher.push_signal(zeros, MinResampleInputSize);
    // _raw_buffer.resize(MinResampleInputSize * 2, 0);
}


TimeStretchPitchShifter::~TimeStretchPitchShifter() {
    delete _stretcher;
}

void TimeStretchPitchShifter::push_signal(const Complex *input, const uint32_t &size) {
    // _raw_buffer.extend_back(input, size);
    _stretcher->push_signal(input, size);
    // _pitch_shifting_stretcher.push_signal(input, size);

    
}

uint32_t TimeStretchPitchShifter::pop_transformed_signal(Complex *output, const uint32_t &size) {
    // do transform, eagerly
    // resample the time-stretch pitch shifted samples
    // while (_pitch_shifting_stretcher.n_transformed_ready() >= MinResampleInputSize) {
    bool can_still_process = true;
    while (can_still_process && n_transformed_ready() < size) {
        const uint32_t desired_stretched_size = size * _shift_factor;
        std::vector<Complex> stretched(desired_stretched_size);

        const uint32_t actually_stretched = _stretcher->pop_transformed_signal(stretched.data(), desired_stretched_size);
        stretched.resize(actually_stretched);
        
        can_still_process = actually_stretched > 0;

        std::vector<Complex> unstretched = _resampler.resample(stretched);

        _transformed_buffer.extend_back(unstretched.data(), unstretched.size());
    }
    uint32_t n = _transformed_buffer.pop_front_many(output, size);
    // std::cout << "n " << n << std::endl; 

    // compensate for drift
    if (n_transformed_ready() > IncreaseResampleThreshold) {
        static constexpr float ResampleHalflife = IncreaseResampleThreshold * 1.5;
        float resample_factor = exp2(-(int)(n_transformed_ready() - IncreaseResampleThreshold) / ResampleHalflife);
        _stretcher->set_stretch_factor(_shift_factor * resample_factor);
    }

    else if (n_transformed_ready() < StandardResampleThreshold) {
        _stretcher->set_stretch_factor(_shift_factor);
    }

    for (uint32_t i = n; i < size; i++) {
        output[i] = 0;
    }
    return n;
}

uint32_t TimeStretchPitchShifter::n_transformed_ready() const {
    return _transformed_buffer.size();
}

void TimeStretchPitchShifter::reset() {
    _raw_buffer.resize(0);
    _transformed_buffer.resize(0);

    _stretcher->reset();

}

void TimeStretchPitchShifter::set_shift_factor(const float &shift_factor) {
    _shift_factor = shift_factor;
    _stretcher->set_stretch_factor(shift_factor);

    _resampler.set_stretch_factor(shift_factor);
}