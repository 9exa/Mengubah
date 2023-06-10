#include "dsp/loudness.h"
#include "dsp/filter.h"
#include "dsp/common.h"
#include "mengumath.h"
#include <complex>
#include <cstdint>

// Coefficients for LUFS_freq filteters
// High shelf filter stage
#define S1_A1 -1.69065929318241
#define S1_A2 0.73248077421585
#define S1_B0 1.53512485958697
#define S1_B1 -2.69169618940638
#define S1_B2 1.19839281085285
// High pass Filter stage
#define S2_A1 -1.99004745483398
#define S2_A2 0.99007225036621
#define S2_B0 1.0
#define S2_B1 -2.0
#define S2_B2 1.0

#define LUFS_DEFAULT_SAMPLE_RATE 48000

float LUFS_freq(Complex *freqs, uint32_t size, uint32_t sample_rate) {
    // Adjust to the custum sample rate
    float sample_rate_correction = (float) sample_rate / LUFS_DEFAULT_SAMPLE_RATE;

    float total_amp2 = 0.0f; // total squared amplitude of each freq bin
    for (uint32_t i = 0; i < size; i++) {
        // argument for the transfer functions
        float omega = (float) MATH_PI * i / size * sample_rate_correction;
        Complex z = std::polar(1.0f, omega);

        // filtered frequency
        Complex y = freqs[i] * quad_filter_trans(z, S1_A1, S1_A2, S1_B0, S1_B1, S1_B2);
        y = y * quad_filter_trans(z, S2_A1, S2_A2, S2_B0, S2_B1, S2_B2);

        total_amp2 += std::norm(y);
    }

    return -0.691 + 10 * Mengu::log10(total_amp2 / size);

}
