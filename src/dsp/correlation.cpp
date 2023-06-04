#include "dsp/correlation.h"
#include "dsp/common.h"
#include "iostream"
#include "mengumath.h"
#include <cstdint>

float Mengu::dsp::correlation(const Complex *s1, const Complex *s2, const int length, const int n) {
    float total = 0;
    for (uint32_t i = 0; i < length; i++) {
        total += s1[i].real() * s2[n + i].real();
    }

    return total;
}

float Mengu::dsp::autocorrelation(const Complex *s, const int length, const int n) {
    return correlation(s, s, length, n);
}

int Mengu::dsp::find_max_correlation(const Complex *s1, const Complex *s2, const int length, const int search_window_size) {
    float max_corr = correlation(s1, s2, length, 0);
    int max_lag = 0;

    for (int i = 1; i < search_window_size; i++) {
        float corr = correlation(s1, s2, length, i);

        if (corr > max_corr) {
            max_corr = corr;
            max_lag = i;
        }
    }

    return max_lag;
}

int Mengu::dsp::find_max_correlation_quad(const Complex *s1, const Complex *s2, const int length, const int search_window_size) {
    float max_corr = -1e10;
    int max_lag = 0;

    float *scaled_s1 = new float[length];

    for (int i = 0; i < length; i++) {
        scaled_s1[i] = s1[i].real() * i * (length - i);
    }

    // do max iteration
    for (int i = 0; i < search_window_size; i++) {
        float corr = 0.0f; 
        
        for (int j = 0; j < length; j++) {
            corr += scaled_s1[j] * s2[i + j].real();
        }

        if (corr > max_corr) {
            max_corr = corr;
            max_lag = i;
        }
    }

    delete[] scaled_s1;

    return max_lag;
}

std::vector<float> Mengu::dsp::calc_srhs(const float *envelope,
                                         const int &size,
                                         const int &min_freq_ind,
                                         const int &max_freq_ind,
                                         const int &n_harm,
                                         const int &step) {
    std::vector<float> output;
    for (int freq_ind = min_freq_ind; freq_ind < max_freq_ind; freq_ind += step) {
        output.push_back(calc_srh(envelope, size, freq_ind, n_harm));
    }
    return output;
}

float Mengu::dsp::calc_srh(const float *envelope, const int &size, const int &freq_ind, const int &n_harm) {
    const int pos_n_harm = MIN(n_harm, size / freq_ind);
    float pos_interference = 0;
    for (int k = 1; k < pos_n_harm; k++) {
        pos_interference += envelope[freq_ind * k];
    }

    const int neg_n_harm = MIN(n_harm, (int) ((float) size / freq_ind + 0.5));
    float neg_interference = 0;
    for (int k = 2; k < neg_n_harm; k++) {
        neg_interference += envelope[(int) (freq_ind * k - 0.5)];
    }

    return pos_interference - neg_interference;
}
