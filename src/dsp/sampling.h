/**
 * @file sampling.h
 * @author 9exa
 * @brief Objects that resamples signal frames
 * @version 0.1
 * @date 2023-05-08
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef MENGA_SAMPLING
#define MENGA_SAMPLING

#include "extras/miniaudio_split/miniaudio.h"
#include "miniaudio.h"
#include "mengumath.h"
#include "dsp/common.h"

#include <cstdint>
#include <vector>

namespace Mengu {
namespace dsp {

// Performs linear resampling without any filtering. Useful for resampling frequency domain directly
template<class T>
inline void linear_resample_no_filter(const T *input, 
                                      T *output, 
                                      uint32_t size, 
                                      float shift_factor, 
                                      T interp(T a, T b, float w) = &lerp,
                                      T def_val = T()) {
    float shift_rep = 1 / shift_factor;
    // interpelate the positiobs in the input
    if (shift_factor > 1.0f) {
        // iterate from the back of the array to make the resample work in-place
        for (uint32_t j = 0; j < size; j++) {
            float inp_index = j * shift_rep;
            uint32_t lower = uint32_t(inp_index);
            float remainder = inp_index - lower;

            output[j] = interp(input[lower], input[lower + 1], remainder);
        }
    }
    else {
        int j = size - 1;
        int resample_start = (int) (size * shift_factor);
        while (j >= resample_start) {
            output[j] = def_val;
            j -= 1;
        }
        while (j >= 0) {
            int inp_index = j * shift_rep;
            int lower = int(inp_index);
            float remainder = inp_index - lower;

            output[j] = interp(input[lower], input[lower + 1], remainder);
            j -= 1;
        }
    }
}

// Just a wrapper around miniaudios resampler API, which uses linear resampling 
// with a 4th order lowpass filter by default
class LinearResampler {
public:
    LinearResampler(uint32_t nchannels, float stretch_factor);
    ~LinearResampler();

    void set_stretch_factor(float stretch_factor);

    std::vector<Complex> resample(const std::vector<Complex> &samples);

private:
    ma_linear_resampler _resampler;

    float _stretch_factor;
    uint32_t _nchannels;


};

}
}

#endif