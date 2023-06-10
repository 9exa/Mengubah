#include "dsp/filter.h"
#include "dsp/common.h"
#include <complex>
#include <cstdint>

using namespace Mengu;
using namespace dsp;

Complex Mengu::dsp::quad_filter_trans(Complex z, float a1, float a2, float b0, float b1, float b2) {
    Complex z1 = std::conj(z); //z^-1
    Complex z2 = z1 * z1;
    return (b0 + b1 * z1 + b2 * z2) / (1.0f + a1 * z1 + a2 * z2);
}

BiquadFilter::BiquadFilter(float pa1, float pa2, float pb0, float pb1, float pb2) :
    a1(pa1), a2(pa2), b0(pb0), b1(pb1), b2(pb2) {}

void BiquadFilter::transform(const float *input, float *output, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        float m = input[i] - a1 * _last_ms[_last_offset] - a2 * _last_ms[(_last_offset + 1) % 2];
        output[i] = b0 * m + b1 * _last_ms[_last_offset] + b2 * _last_ms[(_last_offset + 1) % 2];

        // push the last m
        _last_offset = (_last_offset + 1) % 2;
        _last_ms[_last_offset] = m;
    }
}
