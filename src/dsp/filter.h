/**
 * @file filter.h
 * @author your name (you@domain.com)
 * @brief Filter functions
 * @version 0.1
 * @date 2023-06-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef MENGU_FILTER
#define MENGU_FILTER

#include "dsp/common.h"
#include <cstdint>
namespace Mengu {
namespace dsp {
// the transfer function for a quadratic filter with denominator coefficients a1, a2 and numerator cofficients b0, b1, b2
// assumes z is on a unit circle
Complex quad_filter_trans(Complex z, float a1, float a2, float b0, float b1, float b2);


class BiquadFilter {
public:
    float a1;
    float a2;
    float b0;
    float b1;
    float b2;
    
    BiquadFilter(float pa1, float pa2, float pb0, float pb1, float pb2);

    void transform(const float *input, float *output, uint32_t size);
    void reset();
    
private:
    // store the last 2 raw inputs and intermediaries
    float _last_ms[2] {0};
    uint32_t _last_offset = 0;
};

}
}
#endif