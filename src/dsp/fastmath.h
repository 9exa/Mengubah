#ifndef MENGA_FAST_MATH
#define MENGA_FAST_MATH
#include "mengumath.h"
#include <cmath>

namespace Mengu {
namespace dsp {
// PADE approximations. Only accurate on certain ranges
// use between -pi to +pi
template<typename FloatType>
FloatType sin(FloatType x) {
    x = std::fmod(x + MATH_PI, MATH_TAU) - MATH_PI;
    FloatType x2 = x * x;
    FloatType numerator = x * ((11 * x2) * x2 + 2520);
    FloatType denominator = 60 * x2 + 2520;
    return numerator / denominator;
}

// use between -pi to +pi
template<typename FloatType>
FloatType cos(FloatType x) {
    x = std::fmod(x + MATH_PI, MATH_TAU) - MATH_PI;
    FloatType x2 = x * x;
    FloatType numerator = 131040 + x2 * (62160 + x2 * (3814 - x2 * 59));
    FloatType denominator = 131040 + x2 * (3360 + x2 * 34);
    return numerator / denominator;
}
template<typename FloatType>
FloatType tan(FloatType x) {
    x = std::fmod(x + MATH_PI, MATH_TAU) - MATH_PI;
    FloatType x2 = x * x;
    FloatType numerator = x * (-135135 + x2 * (17325 + x2 * (-378 + x2)));
    FloatType denominator = -135135 + x2 * (62370 + x2 * (-3150 + 28 * x2));
    return numerator / denominator;
}

} // namespace dsp
} // namespace Mengu


#endif