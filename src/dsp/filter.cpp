#include "dsp/filter.h"
#include "dsp/common.h"
#include <complex>

Complex quad_filter_trans(Complex z, float a1, float a2, float b0, float b1, float b2) {
    Complex z1 = std::conj(z); //z^-1
    Complex z2 = z1 * z1;
    return (b0 + b1 * z1 + b2 * z2) / (1.0f + a1 * z1 + a2 * z2);
}