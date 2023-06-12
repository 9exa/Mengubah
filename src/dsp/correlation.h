/**
 * @file correlation.h
 * @author 9exa
 * @brief Correlation/Convolution functions between signals
 * @version 0.1
 * @date 2023-04-30
 * 
 * @copyright Copyright (c) 2023
 */

#ifndef MENGA_CORRELATION
#define MENGA_CORRELATION

#include "dsp/common.h"
#include "dsp/fft.h"
#include "dsp/linalg.h"
#include "mengumath.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstdint>
#include <vector>

namespace Mengu {
namespace dsp {

// The (real finite non-circular) cross-correlation of two signals of equal length, on the offset n
// basically just a dot product
float correlation(const Complex *s1, const Complex *s2, const int length, const int n);

// The (real finite non-circular) cross-correlation of of a signal on itself, on the offset n
float autocorrelation(const Complex *s, const int length, const int n);

// Find the offset/lag that corresponds to the max cross-correlation between s1 and s2 explored up to length
// s2 is assumed to be at least length + search_window_size long
int find_max_correlation(const Complex *s1, const Complex *s2, const int length, const int search_window_size);

// Max correlation where portions toward the center are weighted more
int find_max_correlation_quad(const Complex *s1, const Complex *s2, const int length, const int search_window_size);

// find the sr harmonics of (the positive half of) a frequency amplitude spectrum
std::vector<float> calc_srhs(const float *envelope,
                             const int &size,
                             const int &min_freq_ind,
                             const int &max_freq_ind,
                             const int &n_harm = 8,
                             const int &step = 1);

// the srh of only one frequency
float calc_srh(const float *envelope, const int &size, const int &freq_ind, const int &n_harm);

// performs and stores results of LinearPredictiveCoding. expects a fixed process size so it can be put on the stack
template<uint32_t SampleSize, uint32_t NParams>
class LPC {
public:
    LPC(): 
        _fft(SampleSize),
        // _b(NParams + 1),
        // _autocovariance_slice(NParams + 1) {
        _b{0},
        _autocovariance_slice{0} {
        _b[0] = 1;
    }

    // perform LPC on a sample and set up the intermediate variables
    void load_sample(const Complex *sample) {
        _fft.transform(sample, _freq_spectrum.data());

        // assumes that sample are all real numbers; In general-purpose dsp this will cause bugs

        // multiplication in the frequency domain is convolution (reversed correlation) in the real domain
        std::array<Complex, SampleSize> freq_squared;
        std::array<Complex, SampleSize> autocovariance_comp;
        std::transform(
            _freq_spectrum.cbegin(),
            _freq_spectrum.cend(),
            freq_squared.begin(),
            [] (Complex f) { return std::norm(f); }
        );
        _fft.inverse_transform(freq_squared.data(), autocovariance_comp.data());
        std::transform(
            autocovariance_comp.cbegin(),
            autocovariance_comp.cend(),
            _autocovariance.begin(),
            [] (Complex c) { return c.real(); }
        );

        std::copy(
            _autocovariance.cbegin(),
            _autocovariance.cbegin() + NParams + 1,
            _autocovariance_slice.begin()
        );

        std::array<float, NParams + 1> a = solve_sym_toeplitz(_autocovariance_slice, _b);
        std::array<Complex, SampleSize> a_comp{0};
        const float a0 = a[0];
        std::transform(a.cbegin(), a.cend(), a_comp.begin(),
            [a0] (float f) { return Complex(f / a0); }
        );
        
        std::array<Complex, SampleSize> A;
        _fft.transform(a_comp.data(), A.data());

        // calc envelope
        std::transform(A.cbegin(), A.cend(), _envelope.begin(),
            // try to prevent infs
            [] (Complex c) { return 1.0f / (sqrt(std::norm(c))); }
        );

        // calce residuals
        for (uint32_t i = 0; i < SampleSize; i++) {
            _residuals[i] = std::sqrt(std::norm(_freq_spectrum[i] * A[i]));
        }
        // std::transform(_freq_spectrum.cbegin(), _freq_spectrum.cend(), A.cbegin(), _residuals.begin(),
        //     [] (Complex x_val, Complex a_val) { return std::sqrt(std::norm(x_val * a_val)); }
        // );
    }
    
    // The dft of the loaded samples
    const std::array<Complex, SampleSize> &get_freq_spectrum() const {
        return _freq_spectrum;
    }
    
    // The correlation of the signal with itself
    const std::array<float, SampleSize> &get_autocovariance() const {
        return _autocovariance;
    }
    
    // Normalized copy of autocovariance
    std::array<float, SampleSize> get_autocorrelation() const {
        std::array<float, SampleSize> autocorrelation;
        float max_cov = *std::max_element(_autocovariance);
        std::transform(
            _autocovariance.cbegin(),
            _autocovariance.cend(),
            autocorrelation.begin(),
            [max_cov] (float cov) {return cov / max_cov;}
        );
    }

    // Envelope of the frequency spectrum
    const std::array<float, SampleSize> &get_envelope() const {
        return _envelope;
    }

    // LCP residuals of the frequency
    const std::array<float, SampleSize> &get_residuals() const {
        return _residuals;
    }

    // useful for inversion
    const FFT &get_fft() const {
        return _fft;
    }

private:
    // results to be getted
    std::array<Complex, SampleSize> _freq_spectrum;
    std::array<float, SampleSize> _autocovariance;
    std::array<float, SampleSize> _envelope;
    std::array<float, SampleSize> _residuals; 

    // intermediates
    FFT _fft;
    std::array<float, NParams + 1> _b;
    std::array<float, NParams + 1> _autocovariance_slice;
    // std::vector<float> _b;
    // std::vector<float> _autocovariance_slice;
    
};

}
}

#endif