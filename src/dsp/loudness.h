/**
 * @file loudness.h
 * @author your name (you@domain.com)
 * @brief Functions for calculating loudness in a signal
 * @version 0.1
 * @date 2023-06-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef MENGU_LOUDNESS
#define MENGU_LOUDNESS

#include "dsp/common.h"
#include "dsp/filter.h"
#include <cstdint>
#include <stdint.h>

namespace Mengu {
namespace dsp {

// Loudness Units relative to Full Scale of a sample in the frequency domain
// first (positive) half of the frequency spectrum only
// Only supports 1 channel
float LUFS_freq(Complex *freqs, uint32_t size, uint32_t sample_rate = 48000);

// The value of the transfer function associated with the described frequency bin
Complex LUFS_filter_transfer(float freq);

// Performs the filter step associated with LUFS
class LUFSFilter {
public:
    LUFSFilter();

    void transform(const float *input, float *output, uint32_t size);

    void reset();

private:
    BiquadFilter _high_shelf_filter; // Stage 1 filter
    BiquadFilter _high_pass_filter; // Stage 2 filter
};

// Scales a raw sample to have the same Loudness (in LUFS) as a reference sample
// The raw sample and reference sample are assumed to come from their own persistant samples
template<typename T, uint32_t N, int DefCorrection = 1>
class LoudnessNormalizer {
public:
    void normalize(const T *raw_sample, const T *reference_sample, T *output) {
        float filtered_raw[N];
        float filtered_reference[N];
        for (uint32_t i = 0; i < N; i++) {
            filtered_raw[i] = _as_float(raw_sample[i]);
            filtered_reference[i] = _as_float(reference_sample[i]); 
        }

        // perform filter
        _raw_sample_filter.transform(filtered_raw, filtered_raw, N);
        _reference_sample_filter.transform(filtered_reference, filtered_reference, N);

        // Get the (unormalized) power of each filtered sample
        float raw_power = 0.0f;
        float reference_power = 0.0f;
        for(uint32_t i = 0; i < N; i++) {
            raw_power += filtered_raw[i] * filtered_raw[i];
            reference_power += filtered_reference[i] * filtered_reference[i];
        }
        float correction = sqrt(reference_power / raw_power);
        // std::cout << correction << ", " << raw_power << ", " << shifted_power << std::endl;
        if (!std::isfinite(correction)) {
            // just correct for 1/2 freq spectrum sampling if correction is not real
            for (uint32_t i = 0; i < N; i++) {
                output[i] = raw_sample[i] * (float) DefCorrection;
            }
        }
        else {
            // otherwise scale the shifted by the correction
            for (uint32_t i = 0; i < N; i++) {
                output[i] = raw_sample[i] * correction;
            }
        }

    }
    // resets memory on previous raw and reference samples
    void reset() {
        _raw_sample_filter.reset();
        _reference_sample_filter.reset();
    }
private:
    LUFSFilter _raw_sample_filter;
    LUFSFilter _reference_sample_filter;


    static float _as_float(T v) {
        return Complex(v).real();
    }
};


}
}


#endif