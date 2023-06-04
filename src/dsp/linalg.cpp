#include "dsp/linalg.h"
#include <algorithm>
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

// https://hitonanode.github.io/cplib-cpp/linear_algebra_matrix/levinson.hpp
// std::vector<float> Mengu::dsp::solve_sym_toeplitz(const std::vector<float> &cols, const std::vector<float> &y) {
//     int N = y.size();
//     if (cols[N - 1] == 0) {
//         return {};
//     }

//     std::vector<float> x(N);
//     std::vector<float> forward_vec(1), backward_vec(1);
//     forward_vec[0] = backward_vec[0] = 1.0f / cols[N-1];
//     x[0] = forward_vec[0] * y[0];

//     for (int n = 1; n < N; n++) {
//         float error_forward = 0, error_backward = 0;
//         for (int i = 0; i < n; i++) {
//             error_forward += forward_vec[i] * cols[N - 1 + n - i];
//             error_backward += backward_vec[i] * cols[N - 2 - i];
//         }
//         if (error_forward * error_backward == 1) { return {}; }
//         auto next_forward = forward_vec, next_backward = backward_vec;
//         next_forward.emplace_back(0), next_backward.insert(next_backward.begin(), 0);
//         for (int i = 0; i < n; i++) {
//             next_forward[i + 1] -= error_forward * backward_vec[i];
//             next_backward[i] -= error_backward * forward_vec[i];
//         }
//         float c = 1.0f / (1 - error_forward * error_backward);
//         for (int i = 0; i < n + 1; i++) { 
//             next_forward[i] *= c, next_backward[i] *= c; 
//         }
//         forward_vec = next_forward, backward_vec = next_backward;

//         float z = y[n];
//         for (int i = 0; i < n; i++) {
//             z -= x[i] * cols[N - 1 + n - i];
//         }
//         for (int i = 0; i < n + 1; i++) {
//             x[i] += backward_vec[i] * z;
//         }
//     }
//     return x;

// }