/**
 * @file linalg.h
 * @author 9exa
 * @brief objects and algorithms to solve linear equations
 * @version 0.1
 * @date 2023-05-04
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef MENGA_LINALG
#define MENGA_LINALG


#include <array>
#include <cstddef>
#include <vector>
#include <iostream>

namespace Mengu {
namespace dsp {

//2-D matrix

// dot_product

// c-style dot product
template<typename T>
inline T dot(const T *a, const T *b, const int size) {
    T total = T();
    for (int i = 0; i < size; i++) {
        total += a[i] * b[i];
    }

    return total;
}

// do product on iterator
template<class InputIt1, class InputIt2>
inline float dot(InputIt1 afirst, InputIt1 alast, InputIt2 bfirst) {
    float total = 0.0f;
    while (afirst != alast) {
        total += *afirst * *bfirst;

        afirst++;
        bfirst++;
    }

    return total;
}

template<typename T>
std::vector<T> scalar_mul(const T lambda, const std::vector<T> &a) {
    std::vector<T> scaled;
    for (T x: a) {
        scaled.push_back(lambda * x);
    }
    return scaled;
}

template<typename T, size_t N>
std::array<T, N> scalar_mul(const T lambda, const T *a, size_t size) {
    std::array<T, N> scaled;
    for (size_t i = 0; i < size; i++) {
        scaled[i] = lambda * a[i];
    }
    return scaled;
}

template<typename T>
void scalar_mul_inplace(const T lambda, T *a, const int size) {
    for (int i = 0; i < size; i++) {
            a[i] = lambda * a[i];
        }
}

// add b to a inplace
template<typename T>
void vec_add_inplace(const std::vector<T> &b, std::vector<T> &a) {
    for (int i = 0; i < a.size(); i++) {
        a[i] += b.at(i);
    }
}
template<typename T>
void vec_add_inplace(const T *b, T *a, const int size) {
    for (int i = 0; i < size; i++) {
        a[i] += b[i];
    }
}

// Solves a system of equations where the matrix is a symmetric TOEplitz matrix,
// The matrix is represented as a list of its column values
std::vector<float> solve_sym_toeplitz(const std::vector<float> &cols, const std::vector<float> &y);


template <typename T, size_t N>
std::array<T, N> solve_sym_toeplitz(const std::array<T, N> &cols, const std::array<T, N> &y) {
    // good ol dynamic programming to repeatedly do each step of levinson_recursion consequtively
    std::array<T, N> backward_vec;
    std::array<T, N> forward_vec;
    backward_vec[backward_vec.size() - 1] = forward_vec[0] = 1.0 / cols.at(0);
    
    float error = 0.0f;

    std::array<T, N> result;
    result[0] = {y.at(0) / cols.at(0)};

    for (int n = 1; n < N; n++) {
        // calculate the current error and backward vec
        std::copy(backward_vec.crbegin(), backward_vec.crbegin() + n, forward_vec.begin());
        error = dot(cols.data() + 1, backward_vec.data() + (N - n), n);

        float denom = 1.0f / (1 - error * error);

        scalar_mul_inplace(denom, backward_vec.data() + (N - n), n);
        backward_vec[N - 1 - n] = T();


        scalar_mul_inplace(-error * denom, forward_vec.data(), n);
        forward_vec[n] = T();

        vec_add_inplace(forward_vec.data(), backward_vec.data() + (N - n - 1), n + 1);


        // calculate this iterations result
        auto cols_iter = cols.crbegin() + (cols.size() - n - 1); // the elements n to 0
        float result_error = dot(cols_iter, cols.crend() - 1, result.cbegin());

        std::array<T, N> scaled_backward_vec = scalar_mul<T, N>(
            y[n] - result_error, 
            backward_vec.data() + (N - n - 1), 
            n + 1);
        result[n] = T();

        vec_add_inplace(scaled_backward_vec.data(), result.data(), n + 1);
    }

    return result;
}


}
}

//


#endif