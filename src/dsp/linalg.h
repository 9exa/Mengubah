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

/* results in infinite loop ????
template<typename T>
std::vector<T> vec_add_inplace(const T &b, T &a) {
    std::cout << b.size() << ", " << a.size() << std::endl;
    size_t size = a.size();
    for (size_t i = 0; i < size; i++) {
        std::cout << i << size << (i < size) << "\n";
        a[i] += b.at(i);
    }
}*/

// Solves a system of equations where the matrix is a symmetric TOEplitz matrix,
// The matrix is represented as a list of its column values
std::vector<float> solve_sym_toeplitz(const std::vector<float> &cols, const std::vector<float> &y);

}
}

//


#endif