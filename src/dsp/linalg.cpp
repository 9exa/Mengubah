#include "dsp/linalg.h"
#include <algorithm>
#include <array>
#include <vector>



std::vector<float> Mengu::dsp::solve_sym_toeplitz(const std::vector<float> &cols, const std::vector<float> &y) {

    // good ol dynamic programming to repeatedly do each step of levinson_recursion consequtively
    std::vector<float> backward_vec = {1.0f / cols.at(0)}; // y.size  assumed small, so use vec
    backward_vec.reserve(y.size());

    std::vector<float> forward_vec(1);
    
    float error = 0.0f;

    std::vector<float> result = {y.at(0) / cols.at(0)};
    result.reserve(y.size());

    for (int n = 1; n < y.size(); n++) {
        // calculate the current error and backward vec
        std::copy(backward_vec.crbegin(), backward_vec.crend(), forward_vec.begin());
        error = dot(cols.data() + 1, backward_vec.data(), n);

        float denom = 1.0f / (1 - error * error);

        scalar_mul_inplace(denom, backward_vec.data(), backward_vec.size());
        backward_vec.emplace(backward_vec.begin(), 0);


        scalar_mul_inplace(-error * denom, forward_vec.data(), forward_vec.size());
        forward_vec.push_back(0);

        vec_add_inplace(forward_vec, backward_vec);


        // calculate this iterations result
        auto cols_iter = cols.crbegin() + (cols.size() - n - 1); // the elements n to 1
        float result_error = dot(cols_iter, cols.crend() - 1, result.cbegin());

        std::vector scaled_backward_vec = scalar_mul(y[n] - result_error, backward_vec);
        result.push_back(0);

        vec_add_inplace(scaled_backward_vec, result);
    }

    return result;
}

