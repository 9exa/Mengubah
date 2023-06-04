#include "dsp/sampling.h"
#include "dsp/common.h"
#include "mengumath.h"
#include "extras/miniaudio_split/miniaudio.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <math.h>
#include <vector>
#include <iostream>

using namespace Mengu;
using namespace dsp;

constexpr uint32_t DefaultSampleRate = 44100;

// void Mengu::dsp::linear_resample_no_filter(const float *input, float *output, uint32_t size, float shift_factor, float def_val)


LinearResampler::LinearResampler(uint32_t nchannels, float stretch_factor) :
    _stretch_factor(stretch_factor),
    _nchannels(nchannels) {
    
    ma_linear_resampler_config resampler_config = ma_linear_resampler_config_init(
        ma_format_f32,
        nchannels,
        DefaultSampleRate, 
        DefaultSampleRate * stretch_factor
    );

    ma_result result = ma_linear_resampler_init(&resampler_config, nullptr, &_resampler);

    if (result != MA_SUCCESS) {
        std::string err_msg ("Could not create resampler");
        err_msg += std::to_string(result);

        throw std::runtime_error(err_msg);
    }
    
}

LinearResampler::~LinearResampler() {
    ma_linear_resampler_uninit(&_resampler, nullptr);
}

void LinearResampler::set_stretch_factor(float stretch_factor) {
    _stretch_factor = stretch_factor;
    ma_linear_resampler_set_rate_ratio(&_resampler, stretch_factor);
}

std::vector<Complex> LinearResampler::resample(const std::vector<Complex> &samples) {
    std::vector<float> fsamples;
    std::transform(samples.cbegin(), samples.cend(), std::back_inserter(fsamples),
        [] (Complex f) { return f.real(); }
    );

    ma_uint64 input_size = fsamples.size();
    ma_uint64 output_size;
    ma_linear_resampler_get_expected_output_frame_count(&_resampler, samples.size(), &output_size);
    
    std::vector<float> foutput(output_size);
    ma_result result = ma_linear_resampler_process_pcm_frames(&_resampler, fsamples.data(), &input_size, foutput.data(), &output_size);

    if (result != MA_SUCCESS) {
        std::string err_msg ("Could not perform resample");
        err_msg += std::to_string(result);

        throw std::runtime_error(err_msg);
    }

    std::vector<Complex> output;
    std::transform(foutput.cbegin(), foutput.cend(), std::back_inserter(output),
        [] (float f) { return Complex(f); }
    );
    
    return output;
}