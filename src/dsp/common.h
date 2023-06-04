#ifndef MENGA_DSP_COMMON
#define MENGA_DSP_COMMON

#include <complex>


typedef std::complex<float> Complex;

namespace Mengu {

// returns a Complex number with the same norm as x but with the angle phase
template <typename T>
std::complex<T> with_phase(const std::complex<T> &x, T phase);

// A complex number with the same phase as x, but with distance from the origin amp
template <typename T>
std::complex<T> with_amp(const std::complex<T> &x, T amp) {
    return x / std::sqrt(std::norm(x)) * amp;
}

}

#endif