#include "dsp/timestretcher.h"
#include "dsp/common.h"
#include "dsp/correlation.h"
#include "dsp/effect.h"
#include "dsp/interpolation.h"
#include "dsp/linalg.h"
#include "mengumath.h"
#include "templates/vecdeque.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <complex>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <vector>

using namespace Mengu;
using namespace dsp;

void TimeStretcher::set_stretch_factor(const float &scale) {
    _stretch_factor = scale;
}

Effect::InputDomain TimeStretcher::get_input_domain() {
    return InputDomain::Time;
}

std::vector<EffectPropDesc> TimeStretcher::get_property_descs() const {
    return {
        EffectPropDesc {
            .type = EffectPropType::Slider,
            .name = "stretch_factor",
            .desc = "Scales the length of pushed signals by this amount",
            .slider_data = {
                .min_value = 0.5,
                .max_value = 2,
                .scale = Exp,
            }
        }
    };
};

void TimeStretcher::set_property(uint32_t id, EffectPropPayload data) {
    switch (id) {
        case 0:
            if (data.type == Slider) {
                set_stretch_factor(data.value);
            }
            break;
    };
}

EffectPropPayload TimeStretcher::get_property(uint32_t id) const {
    switch (id) {
        case 0:
            return EffectPropPayload {
                .type = Slider,
                .value = _stretch_factor,
            };
            
    };
    return EffectPropPayload {};
}


static float _wrap_phase(float phase) {
    return fposmod(phase + MATH_PI, MATH_TAU) - MATH_PI;
}

// difference between 2 unwrapped phases. since, phases are preiodic, picks the closest one to the estimate
static float _phase_diff(float next, float prev, float est = 0.0f) {
    return _wrap_phase(next - prev - est) + est;
}

PhaseVocoderTimeStretcher::PhaseVocoderTimeStretcher(bool preserve_formants) {
    _preserve_formants = preserve_formants;
    reset();
}

void PhaseVocoderTimeStretcher::push_signal(const Complex *input, const uint32_t &size) {
    _raw_buffer.extend_back(input, size);
}

uint32_t PhaseVocoderTimeStretcher::pop_transformed_signal(Complex *output, const uint32_t &size) {
    while (n_transformed_ready() < size && _raw_buffer.size() >= WindowSize) {
        std::array<Complex, WindowSize> sample;
        _raw_buffer.to_array(sample.data(), WindowSize);
        _load_new_freq_window(sample);

        std::array<Complex, WindowSize / 2> curr_freqs;
        std::copy(_lpc.get_freq_spectrum().cbegin(), _lpc.get_freq_spectrum().cend(), curr_freqs.begin());

        float analysis_hop_sizef = SynthesisHopSize / _stretch_factor;
        _stretched_sample_truncated += std::modf(analysis_hop_sizef, &analysis_hop_sizef);
        uint32_t analysis_hop_size = (uint32_t) analysis_hop_sizef;
        if (_stretched_sample_truncated) {
            analysis_hop_size += 1;
            _stretched_sample_truncated -= 1;
        }
        
        std::array<float, WindowSize / 2> amplitudes = _calc_scaled_magnitudes();
        std::array<float, WindowSize / 2> new_phases = _calc_scaled_phases(curr_freqs, analysis_hop_size);

        // std::array<Complex, WindowSize> new_samples = _calc_new_samples(amplitudes.data(), curr_raw_phases.data()); 
        std::array<Complex, WindowSize> new_samples = _calc_new_samples(amplitudes.data(), new_phases.data()); 
        
        mix_and_extend(_transformed_buffer, new_samples, WindowSize - SynthesisHopSize, hann_window);
        // _transformed_buffer.extend_back(new_samples.data(), WindowSize);

        std::transform(new_phases.cbegin(), new_phases.cend(), _last_scaled_phases.begin(), 
            [] (float unwrapped) { return _wrap_phase(unwrapped); }
        );

        _raw_buffer.pop_front_many(nullptr, analysis_hop_size);

    }

    return _transformed_buffer.pop_front_many(output, MIN(n_transformed_ready(), size));
}

uint32_t PhaseVocoderTimeStretcher::n_transformed_ready() const {
    return MAX(SynthesisHopSize, _transformed_buffer.size()) - (SynthesisHopSize);
}

void PhaseVocoderTimeStretcher::reset() {
    _prev_raw_mag2s.fill(0.0f);
    _prev_raw_phases.fill(0.0f);

    _last_scaled_phases.fill(0.0);

    _raw_buffer.resize(0);
    _transformed_buffer.resize(SynthesisHopSize, 0);
    _stretched_sample_truncated = 0.0;
}

// void PhaseVocoderTimeStretcher::set_stretch_factor(const float &stretch_factor) {
//     _stretch_factor = stretch_factor;
//     // _last_scaled_phases.fill(0.0f);
// };

std::array<float, PhaseVocoderTimeStretcher::WindowSize / 2> PhaseVocoderTimeStretcher::_calc_scaled_magnitudes() {
    const std::array<Complex, WindowSize> &freqs = _lpc.get_freq_spectrum();
    std::array<float, WindowSize / 2> mags;
    
    if (_preserve_formants) {
        const std::array<float, WindowSize> &envelope = _lpc.get_envelope();
        std::array<float, WindowSize / 2> scaled_envelope;

        // make values of the the envelop not zero to prevent infinitys and nans
        const float envelope_max = std::reduce(envelope.cbegin(), envelope.cbegin() + WindowSize / 2, 0.0, 
        [] (float a, float b) {
                return MAX(a, b);
            }
        );
        std::transform(envelope.cbegin(), envelope.cbegin() + WindowSize / 2, scaled_envelope.begin(),
            [envelope_max] (float v) { return 0.05f * envelope_max + v; }
        );


        // Scale to the envelope
        for (uint32_t i = 0; i < WindowSize / 2; i++) {
            const uint32_t stretched_ind = i * _stretch_factor;
            const float correction = ((uint32_t) (stretched_ind) < WindowSize / 2) && std::isfinite(scaled_envelope[stretched_ind] / scaled_envelope[i]) ?
                scaled_envelope[stretched_ind] / scaled_envelope[i] : 
                0.0f; 
            // if (!std::isfinite(correction)) {
            //     std::cout << "nan ai: "<< i << " " << scaled_envelope[i] <<" emax" << envelope_max << std::endl;
            // }
            
            mags[i] = sqrt(std::norm(freqs[i])) * correction;
        }
    }
    else {
        for (uint32_t i = 0; i < WindowSize / 2; i++) {
            // mags[i] = residuals[i] * envelope[i];
            mags[i] = sqrt(std::norm(freqs[i]));
        }
    }
    
    return mags;
}

std::array<float, PhaseVocoderTimeStretcher::WindowSize / 2> PhaseVocoderTimeStretcher::_calc_scaled_phases(
        const std::array<Complex, WindowSize / 2> &curr_freqs,
        const uint32_t hopsize) {
    std::array<float, WindowSize / 2> curr_mag2s;
    std::array<float, WindowSize / 2> curr_phases;
    std::transform(curr_freqs.cbegin(), curr_freqs.cend(), curr_mag2s.begin(), 
        [] (Complex freq) { return std::norm(freq); }
    );
    std::transform(curr_freqs.cbegin(), curr_freqs.cend(), curr_phases.begin(), 
        [] (Complex freq) { return std::arg(freq); }
    );


    std::array<float, WindowSize / 2> phase_deltas;
    float stretch_factor = (float) SynthesisHopSize / hopsize;
    for (int i = 0; i < WindowSize / 2; i++) {
        const float est = i * MATH_TAU * ((float) hopsize / WindowSize);// / stretch_factor;
        phase_deltas[i] = _phase_diff(curr_phases[i], _prev_raw_phases[i], est);
    }
    std::array<float, WindowSize / 2> new_phases;
    for (int i = 0; i < WindowSize / 2; i++) {
        new_phases[i] = _last_scaled_phases[i] + (phase_deltas[i] * stretch_factor);
    }

    _replace_prev_freqs(curr_mag2s, curr_phases);

    return new_phases;
}

std::array<Complex, PhaseVocoderTimeStretcher::WindowSize / 2> PhaseVocoderTimeStretcher::_load_new_freq_window(const std::array<Complex, WindowSize> &sample) {
    _lpc.load_sample(sample.data());
    
    std::array<Complex, WindowSize / 2> new_freqs;
    for (uint32_t i = 0; i < WindowSize / 2; i++) {
        new_freqs[i] = _lpc.get_freq_spectrum()[i];
    }

    return new_freqs;
}

void PhaseVocoderTimeStretcher::_replace_prev_freqs(const std::array<float, WindowSize / 2> &curr_mag2s, const std::array<float, WindowSize / 2> &curr_phases) {
    std::copy(curr_mag2s.cbegin(), curr_mag2s.cend(), _prev_raw_mag2s.begin());
    std::copy(curr_phases.cbegin(), curr_phases.cend(), _prev_raw_phases.begin());
}

std::array<Complex, PhaseVocoderTimeStretcher::WindowSize> PhaseVocoderTimeStretcher::_calc_new_samples(const float *amplitudes, const float *phases) {
    std::array<Complex, WindowSize> freqs = {0.0};
    for (uint32_t i = 0; i < WindowSize / 2; i++) {
        freqs[i] = 2.0f * std::polar(amplitudes[i], phases[i]);
    }

    std::array<Complex, WindowSize> samples;
    _lpc.get_fft().inverse_transform(freqs.data(), samples.data());
    return samples;
}

PhaseVocoderDoneRightTimeStretcher::PhaseVocoderDoneRightTimeStretcher(bool preserve_formants) : PhaseVocoderTimeStretcher(preserve_formants) {}

std::array<float, PhaseVocoderTimeStretcher::WindowSize / 2> PhaseVocoderDoneRightTimeStretcher::_calc_scaled_phases(
        const std::array<Complex, WindowSize / 2> &curr_freqs,
        const uint32_t hopsize) {
    std::array<float, WindowSize / 2> curr_mag2s;
    std::array<float, WindowSize / 2> curr_phases;
    std::transform(curr_freqs.cbegin(), curr_freqs.cend(), curr_mag2s.begin(), 
        [] (Complex freq) { return std::norm(freq); }
    );
    std::transform(curr_freqs.cbegin(), curr_freqs.cend(), curr_phases.begin(), 
        [] (Complex freq) { return std::arg(freq); }
    );

    float stretch_factor = (float) SynthesisHopSize / hopsize;
    std::array<float, WindowSize / 2> time_phase_deltas;
    for (int i = 0; i < WindowSize / 2; i++) {
        const float est = i * MATH_TAU * ((float) hopsize / WindowSize);
        time_phase_deltas[i] = _phase_diff(curr_phases[i], _prev_raw_phases[i], est);
    }

    std::array<float, WindowSize / 2> freq_phase_deltas;
    for (uint32_t i = 1; i < WindowSize / 2 - 1; i++) {
        const float up_delta = _wrap_phase(curr_phases[i + 1] - curr_phases[i]);
        const float down_delta = _wrap_phase(curr_phases[i] - curr_phases[i - 1]);
        freq_phase_deltas[i] = 0.5f * (up_delta + down_delta);
    }
    freq_phase_deltas[0] = _wrap_phase(curr_phases[1] - curr_phases[0]);
    freq_phase_deltas[WindowSize / 2 - 1] = _wrap_phase(curr_phases[WindowSize / 2 - 1] - curr_phases[WindowSize / 2 - 2]);

    std::array<float, WindowSize / 2> new_phases = _propagate_phase_gradients (
        time_phase_deltas,
        freq_phase_deltas,
        _last_scaled_phases,
        _prev_raw_mag2s,
        curr_mag2s,
        stretch_factor,
        1e-3f
    );

    // for (int i = 0; i < WindowSize / 2; i++) {
    //     new_phases[i] = _last_scaled_phases[i] + (time_phase_deltas[i] * stretch_factor);
    // }

    _replace_prev_freqs(curr_mag2s, curr_phases);
    return new_phases;
}

std::array<float, PhaseVocoderTimeStretcher::WindowSize / 2> PhaseVocoderDoneRightTimeStretcher::_propagate_phase_gradients(
        const std::array<float, WindowSize / 2> &phase_time_deltas, 
        const std::array<float, WindowSize / 2> &phase_freq_deltas, 
        const std::array<float, WindowSize / 2> &last_stretched_phases, 
        const std::array<float, WindowSize / 2> &prev_freq_mags,
        const std::array<float, WindowSize / 2> &next_freq_mags,
        const float stretch_factor,
        const float tolerance) {
    // sort indexes to the next frequencies based on the magnitude of that bin, in descending order
    // also, bins with magnitude under the threshold do not propagate in the frequency domain
    float max_mag = 0.0f;
    for (uint32_t i = 0; i < WindowSize / 2; i++) {
        max_mag = MAX(MAX(max_mag, prev_freq_mags[i]), next_freq_mags[i]);
    }
    const float abs_tol = max_mag * (tolerance * tolerance);

    // Used to store indexs to each frequencing in the propagation_queue
    struct FreqBin {
        enum {
            Prev,
            Next,
        };
        uint8_t frame;
        uint32_t bin; 
    };
    const auto freq_bin_cmp = [&prev_freq_mags, &next_freq_mags] (FreqBin a, FreqBin b) { 
        float a_mag = (a.frame == FreqBin::Prev) ? prev_freq_mags[a.bin] : next_freq_mags[a.bin];
        float b_mag = (b.frame == FreqBin::Prev) ? prev_freq_mags[b.bin] : next_freq_mags[b.bin];
        return a_mag < b_mag;
    };
    
    // Max Heap of the frequency bins to propagate. (Listen I REALLY don't want allocate dynamically)
    std::array<FreqBin, WindowSize * 3> propagation_queue;
    uint32_t propagation_queue_size = WindowSize / 2;
    for (uint32_t i = 0; i < WindowSize / 2; i++) {
        propagation_queue[i] = FreqBin { .frame = FreqBin::Prev, .bin = i };
    }
    std::make_heap(propagation_queue.begin(), propagation_queue.begin() + propagation_queue_size, freq_bin_cmp);

    // Set of frequency bins to propagate to
    bool can_recieve_propagation[WindowSize / 2];
    uint32_t n_can_recieve_propagation = WindowSize / 2;
    for (uint32_t i = 0; i < WindowSize / 2; i++) {
        if ((next_freq_mags[i] < abs_tol)) {
            can_recieve_propagation[i] = false;
            n_can_recieve_propagation -= 1;
        }
        else {
            can_recieve_propagation[i] = true;
        }
    }

    // perform propagation in all dimension
    std::array<float, WindowSize / 2> new_phases {0};
    while (n_can_recieve_propagation > 0) {
        std::pop_heap(propagation_queue.begin(), propagation_queue.begin() + propagation_queue_size, freq_bin_cmp);
        FreqBin next_bin = propagation_queue[propagation_queue_size - 1];
        propagation_queue_size -= 1;

        const uint32_t freq_ind = next_bin.bin;
        if (next_bin.frame == FreqBin::Prev) {
            if (can_recieve_propagation[freq_ind]) {
                new_phases[freq_ind] = last_stretched_phases[freq_ind] + (phase_time_deltas[freq_ind] * stretch_factor);
                
                //remove from set
                can_recieve_propagation[freq_ind] = false;
                n_can_recieve_propagation -= 1;

                // push to the heap
                propagation_queue[propagation_queue_size] = FreqBin {.frame = FreqBin::Next, .bin = freq_ind};
                propagation_queue_size += 1;
                std::push_heap(propagation_queue.begin(), propagation_queue.begin() + propagation_queue_size, freq_bin_cmp);
            }
        }
        else {
            if (freq_ind > 0 && can_recieve_propagation[freq_ind - 1]) {
                const uint32_t freq_down = freq_ind - 1;

                new_phases[freq_down] = new_phases[freq_ind] - (0.5 * (phase_freq_deltas[freq_down] + phase_freq_deltas[freq_ind]) * stretch_factor);

                //remove from set
                can_recieve_propagation[freq_down] = false;
                n_can_recieve_propagation -= 1;

                // push to the heap
                propagation_queue[propagation_queue_size] = FreqBin {.frame = FreqBin::Next, .bin = freq_down};
                propagation_queue_size += 1;
                std::push_heap(propagation_queue.begin(), propagation_queue.begin() + propagation_queue_size, freq_bin_cmp);
            }
            if ((freq_ind < WindowSize / 2 - 1) && can_recieve_propagation[freq_ind + 1]) {
                const uint32_t freq_up = freq_ind + 1;
                new_phases[freq_up] = new_phases[freq_ind] + (0.5 * (phase_freq_deltas[freq_up] + phase_freq_deltas[freq_ind]) * stretch_factor);

                //remove from set
                can_recieve_propagation[freq_up] = false;
                n_can_recieve_propagation -= 1;

                // push to the heap
                propagation_queue[propagation_queue_size] = FreqBin {.frame = FreqBin::Next, .bin = freq_up};
                propagation_queue_size += 1;
                std::push_heap(propagation_queue.begin(), propagation_queue.begin() + propagation_queue_size, freq_bin_cmp);
            }
        }
    }


    // std::array<float, WindowSize / 2> new_phases {0};
    // auto freq_ind_cmp = [next_freq_mags] (uint32_t a, uint32_t b) { return next_freq_mags[a] > next_freq_mags[b]; };
    // std::array<uint32_t, WindowSize / 2> freq_inds;
    // for (uint32_t i = 0; i < WindowSize / 2; i++) { freq_inds[i] = i;}
    // std::sort(freq_inds.begin(), freq_inds.end(), freq_ind_cmp);

    // std::array<float, WindowSize / 2> prop_source_mag {0.0f};
    // for (uint32_t i = 0; i < WindowSize / 2; i++) {
    //     new_phases[i] = last_stretched_phases[i] + phase_time_deltas[i] * stretch_factor;
    //     prop_source_mag[i] = prev_freq_mags[i];
    // }
    // for (const uint32_t freq_ind: freq_inds) {
    //     if (freq_ind > 0 && prop_source_mag[freq_ind - 1] < next_freq_mags[freq_ind]) {
    //         const uint32_t freq_down = freq_ind - 1;
    //         new_phases[freq_down] = new_phases[freq_ind] - 0.5 * stretch_factor * (phase_freq_deltas[freq_down] + phase_freq_deltas[freq_ind]);
    //         prop_source_mag[freq_down] = next_freq_mags[freq_ind];
    //     }
    //     if (freq_ind < WindowSize / 2 && prop_source_mag[freq_ind + 1] < next_freq_mags[freq_ind]) {
    //         const uint32_t freq_up = freq_ind + 1;
    //         new_phases[freq_up] = new_phases[freq_ind] - 0.5 * stretch_factor * (phase_freq_deltas[freq_up] + phase_freq_deltas[freq_ind]);
    //         prop_source_mag[freq_up] = next_freq_mags[freq_ind];
    //     }


    //     prop_source_mag[freq_ind] = 2e10;
    // }

    return new_phases;   
}

OLATimeStretcher::OLATimeStretcher(uint32_t w_size) {
    _window_size = w_size;
    _overlap = _window_size / 5;
    _selection_window = _window_size / 2;

    _transformed_buffer.resize(_window_size);
}

template<class T>
static void mix_into_extend_by_pointer(const Complex *new_data,
                                       T &output,
                                       const uint32_t &window_size,
                                       const uint32_t &overlap_size) {
    uint32_t i = 0;
    
    for (; i < overlap_size; i++) {
        const float w = hann_window((float) i / overlap_size);
        const uint32_t output_ind = output.size() - overlap_size + i;

        output[output_ind] = lerp(new_data[i], output[output_ind], w);
    }

    for (; i < window_size; i++) {
        output.push_back(new_data[i]);
    }
}

void OLATimeStretcher::push_signal(const Complex *input, const uint32_t &size) {
    _raw_buffer.extend_back(input, size);

}


uint32_t OLATimeStretcher::pop_transformed_signal(Complex *output, const uint32_t &size) {
    // Do stretchy
    // Theoretical interval between samples per process 
    const uint32_t sample_skip = (_window_size - _overlap) / _stretch_factor;

    // based on the example given by https://www.surina.net/article/time-and-pitch-scaling.html
    const uint32_t length_for_process = MAX(sample_skip, _window_size + _selection_window);

    while (_raw_buffer.size() > length_for_process) {
        std::vector<Complex> new_data(_window_size + _selection_window);
        _raw_buffer.to_array(new_data.data(), _window_size + _selection_window);

        std::vector<Complex> prev_tail(_overlap);
        _transformed_buffer.pop_back_many(prev_tail.data(), _overlap);

        // find best start for overlap
        uint32_t overlap_ind = find_max_correlation_quad(prev_tail.data(), new_data.data(), _overlap, _selection_window);

        mix_into_extend_by_pointer(new_data.data() + overlap_ind, prev_tail, _window_size, _overlap);

        for (auto v: prev_tail) {
            _transformed_buffer.push_back(v);
        }

        _raw_buffer.pop_front_many(nullptr, sample_skip);
    }


    uint32_t n = _transformed_buffer.pop_front_many(output, MIN(size, n_transformed_ready()));

    for (uint32_t i = n; i < size; i++ ) {
        output[i] = 0;
    }

    return n;
}

uint32_t OLATimeStretcher::n_transformed_ready() const {
    return _transformed_buffer.size() - _overlap;
}

void OLATimeStretcher::reset() {
    _raw_buffer.resize(0);
    _transformed_buffer.resize(_window_size, 0);
}

WSOLATimeStretcher::WSOLATimeStretcher() {
    // _transformed_buffer.resize(MaxBackWindowOverlap);
}

void WSOLATimeStretcher::push_signal(const Complex *input, const uint32_t &size) {
    _raw_buffer.extend_back(input, size);

    // perform the stretchy
    // if (_raw_buffer.size() > SampleProcSize) {
    //     std::array<Complex, SampleProcSize> samples;

    //     _raw_buffer.to_array(samples.data(), SampleProcSize);

    //     // do the stretchy
    //     uint32_t frames_used = _stretch_sample_and_add(samples.data());

    //     // delete everything used
    //     _raw_buffer.pop_front_many(nullptr, frames_used);
    //     _last_overlap_start -= frames_used;
    //     _next_overlap_start -= frames_used;


    // }
}

uint32_t WSOLATimeStretcher::pop_transformed_signal(Complex *output, const uint32_t &size) {
    // lazily perform the stretchy
    while ((_raw_buffer.size() > SampleProcSize) && (n_transformed_ready() < size)) {
        std::array<Complex, SampleProcSize> samples;

        _raw_buffer.to_array(samples.data(), SampleProcSize);

        // do the stretchy
        uint32_t frames_used = _stretch_sample_and_add(samples.data());

        // delete everything used
        _raw_buffer.pop_front_many(nullptr, frames_used);
        _last_overlap_start -= frames_used;
        _next_overlap_start -= frames_used;
    }

    uint32_t n = _transformed_buffer.pop_front_many(output, MIN(size, n_transformed_ready()));
    for (uint32_t i = n; i < size; i++) {
        output[i] = 0;
    }
    return n;
}

uint32_t WSOLATimeStretcher::n_transformed_ready() const {
    return _transformed_buffer.size();
}

void WSOLATimeStretcher::reset() {
    _raw_buffer.resize(0);
    _transformed_buffer.resize(0);

}

uint32_t WSOLATimeStretcher::_stretch_sample_and_add(const Complex *sample) {
    // base overlap
    const uint32_t overlap_size = WindowSize / 4;
    // search forward for better overlap point
    const uint32_t search_window = WindowSize / 5;
    const uint32_t flat_duration = WindowSize - 2 * overlap_size;

    // consider te amount of frames skiped through truncation
    double sample_skipd;
    double dropped_per_window = std::modf((WindowSize - overlap_size) / _stretch_factor, &sample_skipd);

    const uint32_t sample_skip = sample_skipd;
    
    while (_next_overlap_start + WindowSize < SampleProcSize) {
        // Find insertion that best fits the new sample
        uint32_t prev_not_overlapped = find_max_correlation(
            sample + _next_overlap_start, 
            sample + _last_overlap_start, 
            overlap_size, 
            search_window
        );

        uint32_t actual_last_overlap = _last_overlap_start + prev_not_overlapped;
        const uint32_t actually_overlapped = overlap_size - prev_not_overlapped;

        Complex overlap_buffer[SampleProcSize / 2];
        
        overlap_add(
            sample + actual_last_overlap, 
            sample + _next_overlap_start,
            overlap_buffer,
            actually_overlapped,
            hamming_window
        );

        // append new data
        _transformed_buffer.extend_back(sample + _last_overlap_start, prev_not_overlapped);
        _transformed_buffer.extend_back(overlap_buffer, actually_overlapped);
        // take 2 * prev_not_overlapped from both sides of the flat duration
        _transformed_buffer.extend_back(sample + _next_overlap_start + actually_overlapped, flat_duration);

        // set for next cycle
        _last_overlap_start = _next_overlap_start + actually_overlapped + flat_duration;
        _next_overlap_start = _next_overlap_start + sample_skip;

        _stretched_sample_truncated += dropped_per_window;

        if (_stretched_sample_truncated > 1.0) {
            _next_overlap_start += 1;
            _stretched_sample_truncated -= 1.0;
        }
    }

    return MIN(_last_overlap_start, _next_overlap_start);
}

PSOLATimeStretcher::PSOLATimeStretcher() {
    _transformed_buffer.resize(MaxBackWindowOverlap);
}


void PSOLATimeStretcher::push_signal(const Complex *input, const uint32_t &size) {
    _raw_buffer.extend_back(input, size);
}

uint32_t PSOLATimeStretcher::pop_transformed_signal(Complex *output, const uint32_t &size) {
    // lazily perform the stretchy
    while ((_raw_buffer.size() > SampleProcSize) && (size > n_transformed_ready())) {
        std::array<Complex, SampleProcSize> samples;
        std::array<Complex, SampleProcSize> windowed_samples;

        _raw_buffer.to_array(samples.data(), SampleProcSize);
        _raw_buffer.to_array(windowed_samples.data(), SampleProcSize);

        window_ends(windowed_samples.data(), SampleProcSize, SampleProcSize / 10, hann_window);

        int est_freq = _est_fund_frequency(windowed_samples.data());
        
        uint32_t est_period = (1.0 / est_freq) * SampleProcSize / 2;
        
        std::vector<uint32_t> est_peaks = _find_upcoming_peaks(samples.data(), est_period);

        _stretch_peaks_and_add(samples.data(), est_peaks);

        // delete everything used
        _raw_buffer.pop_front_many(nullptr, est_peaks.back() + 1); // +1 because est_peaks are the INDEX of the peaks, not the num of frames used
    }
    
    uint32_t n = _transformed_buffer.pop_front_many(output, MIN(size, n_transformed_ready()));

    for (uint32_t i = n; i < size; i++) {
        output[i] = 0;
    }

    return n;
}

uint32_t PSOLATimeStretcher::n_transformed_ready() const {
    return _transformed_buffer.size() - MaxBackWindowOverlap;
}

void PSOLATimeStretcher::reset() {
    _transformed_buffer.resize(MaxBackWindowOverlap, 0);
    _raw_buffer.resize(0);

}

int PSOLATimeStretcher::_est_fund_frequency(const Complex *samples) {
    // Todo: move all this to a PitchDetecter object or something
    
    _lpc.load_sample(samples);
    const std::array<float, SampleProcSize> residuals = _lpc.get_residuals();

    // find candidate from residual peaks (only use positive half of the spectrum)
    std::vector<float> srhs = calc_srhs(residuals.data(), residuals.size() / 2, MinFreqInd, MaxFreqInd, 10);

    float max_srhs = -10e32;
    int max_pitch_ind = MaxFreqInd;
    for (int i = 0; i < srhs.size(); i++) {
        if (srhs[i] > max_srhs) {
            max_pitch_ind = MinFreqInd + i;
            max_srhs = srhs[i];
        }
    }

    return max_pitch_ind;
}

std::vector<uint32_t> PSOLATimeStretcher::_find_upcoming_peaks(const Complex *samples, const uint32_t est_period) {
    // assume that peaks are around est_period apart, but give some sllack as pitches change slightly
    const uint32_t search_start = 0.8 * est_period;
    const uint32_t search_end = 1.2 * est_period;

    std::vector<uint32_t> peaks; 
    uint32_t last_peak = 0;

    while ((last_peak + est_period) < SampleProcSize) {
        uint32_t peak = last_peak + search_start;
        float peak_size = 0.0f;

        for (uint32_t i = last_peak + search_start; i < MIN(SampleProcSize, last_peak + search_end); i++) {
            if (samples[i].real() > peak_size) {
                peak_size = samples[i].real();
                peak = i;
            }
        }

        peaks.push_back(peak);
        last_peak = peak;
    }
    
    return peaks;
}

// // overlap and extend without applying a window function first
// // overlap_size may be larger than new_data.size(), in which case only new_data.size() is added. the offset is the same
template<class T, class NewT>
static void _mix_and_extend_no_window(T &array, const NewT new_data, const uint32_t &overlap_size) {
    
    uint32_t i = 0;

    for(; i < MIN(overlap_size, new_data.size()); i++) {
        uint32_t ind = array.size() - overlap_size + i;
        array[ind] = array[ind] + new_data[i];
    }

    for (; i < new_data.size(); i++) {
        array.push_back(new_data[i]);
    }

}

void PSOLATimeStretcher::_stretch_peaks_and_add(const Complex *samples, const std::vector<uint32_t> &est_peaks) {

    // accumulate by collecting the left halves and right halves if each peak sepeartly
    std::vector<std::vector<Complex>> right_windows;
    std::vector<std::vector<Complex>> left_windows;

    uint32_t last_peak = 0;
    for (uint32_t next_peak: est_peaks) {
        std::vector<Complex> left_window;
        std::vector<Complex> right_window;

        for (uint32_t i = last_peak; i < next_peak; i++) {
            float w = (float) (i - last_peak) / (next_peak - last_peak);
            w = hann_window(w);

            left_window.emplace_back(w * samples[i]);
            right_window.emplace_back((1.0f - w) * samples[i]);
        }

        left_windows.push_back(std::move(left_window));
        right_windows.push_back(std::move(right_window));

        last_peak = next_peak;
    }

    // arrange the peaks together
    last_peak = 0;

    
    for (uint32_t i = 0; i < left_windows.size(); i++) {
        const std::vector<Complex> left_window = std::move(left_windows[i]);
        const std::vector<Complex> right_window = std::move(right_windows[i]);

        // use overlapsize of previous period to make the right window continuous with the previous left window
        _mix_and_extend_no_window(_transformed_buffer, right_window, _next_right_window_overlap);


        // calculate the frames overlap taking the truncated part of previous processes into account
        int curr_period = est_peaks[i] - last_peak;
        double overlap_sized;
        _stretched_sample_truncated += std::abs(std::modf((2.0 - _stretch_factor) * curr_period, &overlap_sized));
        
        uint32_t overlap_size = std::abs(overlap_sized);
        if (_stretched_sample_truncated > 1.0f) {
            overlap_size += 1;
            _stretched_sample_truncated -= 1.0f;
        }


        if (_stretch_factor >= 1.0f) {
            if (_stretch_factor > 2.0f) {

                // pad the gap between each side with 0s
                uint32_t padding_size = std::move(overlap_size);
            
                for (uint32_t j = 0; j < padding_size; j++) {
                    _transformed_buffer.push_back(0);
                }

                _transformed_buffer.extend_back(left_window.data(), left_window.size());
            }
            else {
                // some parts are overlapped
                // mix it with the left window of the next
                _mix_and_extend_no_window(_transformed_buffer, left_window, overlap_size);

            }

            // the end of the _transform buffer is the end of the left window, which is equivalenting to setting both of these values to 0
            _next_right_window_overlap = 0;
        }
        else {
            // since the left peak ends before the right peak ends, we have to overlap both the right and left peaks into the transformed data
            _mix_and_extend_no_window(_transformed_buffer, left_window, overlap_size);
            
            _next_right_window_overlap = overlap_size - curr_period;
        }



        last_peak = est_peaks[i];
    }
}