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

const VecDeque<Complex> &PhaseVocoderPitchShifter::get_raw_buffer() {
    return _raw_buffer;
}

PhaseVocoderPitchShifter::PhaseVocoderPitchShifter(uint32_t buffer_size) :
        _raw_buffer_size(last_pow_2(buffer_size)),
        _sfft_size(_raw_buffer_size / 2),
        _window_size(0),
        _fft(_sfft_size),
        _n_unprocessed(0) {
    // _raw_buffer.resize(_raw_buffer_size);
    _raw_buffer.resize(_raw_buffer_size);
    

    _last_sfft.resize(_sfft_size);

    _freq_.resize(_sfft_size);
    _shifted_.resize(_sfft_size);
    _result_.resize(_sfft_size);

    _window_size = _sfft_size / 5;

    _transformed_buffer.resize(_window_size+_sfft_size);   

    
}

PhaseVocoderPitchShifter::~PhaseVocoderPitchShifter() {}

void PhaseVocoderPitchShifter::set_shift_factor(const float &factor) {
    _shift_factor = factor;
    _shift_factor_reciprical = 1.0f / factor;
}


void PhaseVocoderPitchShifter::push_signal(const Complex *input, const uint32_t &size) {
    for (uint32_t i = 0; i < size; i++) {
        _raw_buffer.push_back(input[i]);
        _n_unprocessed++;
        // _transformed_buffer.push_back(input[i]);
        // _raw_buffer[i] = input[i];
    }

    // std::vector<Complex> new_raw(size);
    // _raw_buffer.pop_front_many(new_raw.data(), size);
    // _mix_into_transformed(new_raw.data(), size);
    // for (uint32_t i = 0; i < size; i++) {
    //     _transformed_buffer.push_back(_raw_buffer[i]);
    // }
    // std::cout << size << std::endl;
    _recalculate_transformed_buffer();
}

uint32_t PhaseVocoderPitchShifter::pop_transformed_signal(Complex *output, const uint32_t &size) {
    uint32_t n = _transformed_buffer.pop_front_many(output, size);

    for (uint32_t i = n; i < size; i++ ) {
        output[i] = 0;
    }

    return n;
}

uint32_t PhaseVocoderPitchShifter::n_transformed_ready() const {
    return _transformed_buffer.size();
}

void PhaseVocoderPitchShifter::reset() {
    _transformed_buffer.resize(0);
    for (uint32_t i = 0; i < _raw_buffer.size(); i++) {
        _raw_buffer.push_back(0);
    }
}

void PhaseVocoderPitchShifter::_recalculate_transformed_buffer() {
    // reset all intermediates
    std::fill(_freq_.begin(), _freq_.end(), 0);
    std::fill(_shifted_.begin(), _shifted_.end(), 0);
    std::fill(_result_.begin(), _result_.end(), 0);


    const uint32_t new_samples_per_process = _sfft_size - _window_size;
    std::vector<Complex> new_raw(_sfft_size, 0);

    // std::cout <<  _raw_buffer.size() << " " << (_raw_buffer.size() - _n_unprocessed) << std::endl;
    while (_n_unprocessed >= new_samples_per_process) {// new_samples_per_process) {
        // TODO make an actual fing ring buffer
        // _raw_buffer.to_array(new_raw.data(), _sfft_size);
        // std::cout << _raw_buffer.size();

        _raw_buffer.to_array(new_raw.data(), _sfft_size);

        // std::cout << _raw_buffer.size();

        // std::cout << "crash transform" << _sfft_size << _fft.size() << std::endl;
        _fft.transform(new_raw.data(), _freq_.data());

        // std::cout << "crash shift" << std::endl;
        _shift_freqs(_freq_.data(), _shifted_.data(), _sfft_size / 2, false);

        // std::cout << "phase correction" << std::endl;
        _determine_phase(_shifted_.data(), _sfft_size / 2);


        // std::cout << "crash inverse" << std::endl;
        // _fft.inverse_transform(_freq_.data(), _result_.data());
        _fft.inverse_transform(_shifted_.data(), _result_.data());

        _mix_into_transformed(_result_.data(), _sfft_size);
        // _mix_into_transformed(new_raw.data(), _sfft_size);

        // upons sucessfully adding, record this sequence as the last outputted samples for future transformations
        // std::copy(_shifted_.cbegin(), _shifted_.cend(), _last_sfft.begin());

        // _raw_buffer.rotate(new_samples_per_process);// Why did I put this line Here?

        _raw_buffer.pop_front_many(nullptr, new_samples_per_process);
        // for (uint32_t i = 0; i < new_samples_per_process; i++)
        // std::cout << _transformed_buffer.size() << std::endl;

        _n_unprocessed -= new_samples_per_process;

        // std::cout << _raw_buffer[0] << " " << ge << " " << (_raw_buffer[0] == ge) << std::endl;
        // std::cout << _raw_buffer[-1] << " " << he << " " << (_raw_buffer[-1] == he) << std::endl;

    }

    //_shift_freqs(_raw_buffer + (_raw_buffer_size/2), _transformed_buffer + (_raw_buffer_size / 2), _raw_buffer_size / 2, true);
}

void PhaseVocoderPitchShifter::_determine_phase(Complex *new_samples, const uint32_t &size) {
    // uses phase derivative in the time domain to set the phase of the new_samples based on the phases of the previous samples

    // Ignore the phase of old samples if their amplitude is insignificant
    const float tol = 1.0f;
    vector<float> last_norms;
    float max_amp2 = 0.0f;
    // There HAS to be a more functional way of doing this
    for (uint32_t i = 0; i < size; i++) {
        float norm = std::norm(_last_sfft[i]);
        max_amp2 = MAX(norm, max_amp2);
        last_norms.push_back(norm);
    }

    vector<bool> use_phase;
    const float atol = tol * max_amp2;
    for (uint32_t i = 0; i < size; i++) {
        use_phase.push_back((bool) (last_norms[i] > atol));
    }

    // Assume that time between each sample is constant
    const uint32_t new_samples_per_process = _sfft_size - _window_size;
    for (uint32_t i = 0; i < size; i++) {
        // if (true) {
        if (use_phase[i]) {
            const float new_amp = sqrt(std::norm(new_samples[i]));
            // advance wave form by new_samples_per_process samples
            const Complex _phase_rotator = _fft.get_es()[(i * new_samples_per_process) % _fft.size()];
            const Complex new_val = with_amp(_last_sfft[i] * _phase_rotator, new_amp);
            new_samples[i] = new_val;
        }
        // else {use the calculated output signal}
    } 
    
}


// append to the transformed buffer where the overlapping segment in mixed in via the window function
void PhaseVocoderPitchShifter::_mix_into_transformed(const Complex *new_samples, const uint32_t &size, const int method) {
    // int i = 0;
    int i = 0;
    // mix in windows bits
    // std::cout << "w s " << _window_size << std::endl;

    // std::cout << _transformed_buffer[-_window_size - 1] << ",  " << _transformed_buffer[-_window_size] << std::endl;

    for(; i < _window_size; i++) {
        float w = (float) i / _window_size;
        w = hann_window(w);
        const Complex old_bit = _transformed_buffer[i - (int) _window_size] * (1.0f - w);
        const Complex new_bit = new_samples[i] * w;

        // std::cout << "w " << w << " " << i << "  " << i - (int) _window_size << " old bit: " << old << "new bit: " << new_bit << std::endl;

        _transformed_buffer[i - (int) _window_size] = old_bit + new_bit;

        // to_show.push_back(old_bit + new_bit);
        // std::cout << (old_bit + new_bit).real() << "' ";
    }
    for(; i < size; i++) {
        _transformed_buffer.push_back(new_samples[i]);
        // to_show.push_back(new_samples[i]);
        // std::cout << i << ", ";

    }

    // std::cout <<"end" << std::endl;
}

#define MENGA_PHASE_INJECTOR
void PhaseVocoderPitchShifter::_shift_freqs(const Complex *input, Complex *output, const uint32_t &size, const bool &reverse) {
    // resample the amplitude of the frequencies

    if (!use_interp) {
        for (uint32_t i = 0; i < size; i++) output[i] = 0;
        for (uint32_t i = 0; i < size; i++) {
            int scaled_index = std::round(i * _shift_factor);
            if (scaled_index >= size) {
                break;
            }

            int lower_index = scaled_index; // Mengu::CLAMP((int)std::round((i - 1) * _shift_factor) + 1, 0, scaled_index);

            // std::cout << lower_index << std::endl;
            for (int j = lower_index; j <= scaled_index; j++) {
                // apply to whole region
                uint32_t output_index, input_index;
                if (reverse) {
                    output_index = size - 1 - j;
                    input_index = size - 1 - i;
                }
                else {
                    output_index = j;
                    input_index = i;
                }
                // std::cout << scaled_index << " " << input_index << " " << output_index << std::endl;

                // float new_arg = std::arg(input[input_index]);//  * _shift_factor;
                // output[output_index] = std::polar(std::norm(std::sqrt(input[input_index])), new_arg);

                output[output_index] = input[input_index];

            }
        }
    }

    else {
        for (uint32_t i = 0; i < size; i++) {
            const float scaled_index = i * _shift_factor_reciprical;
            float scaled_index_whole;
            // float lerp_factor = Mengu::modf(scaled_index, &scaled_index_whole);
            
            if (scaled_index_whole >= size) {
                // resample out of range. can only be true if _shift_factor_reciprical > 1; so just zero out this and future indices
                for (uint32_t j = i;  j < size; j++) {
                    uint32_t output_index = reverse ? size - 1 - j : j;
                    output[output_index] = 0;
                }
                break;
            }

            int output_index, lower_scaled_index;//, upper_scaled_index;
            if (reverse) {
                output_index = size - 1 - i;
                lower_scaled_index = size - 1 - (int)scaled_index_whole;
                // upper_scaled_index = Mengu::MAX(lower_scaled_index - 1, 0);
            }
            else {
                output_index = i;
                lower_scaled_index = (int)scaled_index_whole;
                // upper_scaled_index = Mengu::MIN(lower_scaled_index + 1, size - 1);
            }
            // std::cout << i << " " << input[lower_scaled_index]  << " " << input[upper_scaled_index] << std::endl;

            //output[output_index] = Mengu::lerp(sqrtf(std::norm(input[lower_scaled_index])), sqrtf(std::norm(input[lower_scaled_index])), lerp_factor);
            output[output_index] = input[lower_scaled_index];
            // rescale the phase as well
            //output[output_index] = std::polar(sqrtf(std::norm(output[output_index])), _shift_factor * std::arg(output[output_index]));
        }
    }
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
        // const uint32_t n_raw = _raw_buffer.pop_front_many(output, size);
        // _stretcher->push_signal(output, n_raw);

        const uint32_t desired_stretched_size = size * _shift_factor;
        std::vector<Complex> stretched(desired_stretched_size);

        const uint32_t actually_stretched = _stretcher->pop_transformed_signal(stretched.data(), desired_stretched_size);
        stretched.resize(actually_stretched);
        
        // can_still_process = actually_stretched == desired_stretched_size;
        can_still_process = actually_stretched > 0;
        // std::cout << actually_stretched << std::endl;

        std::vector<Complex> unstretched = _resampler.resample(stretched);

        // _transformed_buffer.extend_back(stretched.data(), stretched.size());
        _transformed_buffer.extend_back(unstretched.data(), unstretched.size());
        // std::cout << "nready " << _transformed_buffer.size() << std::endl;
    }
    uint32_t n = _transformed_buffer.pop_front_many(output, size);
    // std::cout << "n " << n << std::endl; 

    // compensate for drift
    if (n_transformed_ready() > IncreaseResampleThreshold) {
        static constexpr float ResampleHalflife = IncreaseResampleThreshold * 1.5;
        float resample_factor = exp2(-(int)(n_transformed_ready() - IncreaseResampleThreshold) / ResampleHalflife);
        std::cout << "oversampling..." << -(n_transformed_ready() - IncreaseResampleThreshold) / ResampleHalflife<< " "<< resample_factor<< std::endl;
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

    // Complex zeros[MinResampleInputSize] = {Complex()};
    // _stretcher.push_signal(zeros, MinResampleInputSize);
    // _pitch_shifting_stretcher.push_signal(zeros, MinResampleInputSize);
}

void TimeStretchPitchShifter::set_shift_factor(const float &shift_factor) {
    _shift_factor = shift_factor;
    _stretcher->set_stretch_factor(shift_factor);

    _resampler.set_stretch_factor(shift_factor);
}