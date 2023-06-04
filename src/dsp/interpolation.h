#ifndef MENGA_INTERPOLATION
#define MENGA_INTERPOLATION

#include "mengumath.h"
#include "dsp/fastmath.h"
#include <cmath>
#include <cstdint>
// Helper functions for various interpolation and windowing methods

namespace Mengu {
namespace dsp{

// The hann function
inline float hann(float a0, float x) {
    return a0 - (1.0 - a0) * std::cos(MATH_TAU * x);
    // equals
    // a - (1-a) (c2 -s2)
    // a (1 - c2 + s2) - (c2 - s2)
    // a (2s2) - 2s2 - 1
    // (a - 1) 2 s2
}

// root of the hann function designed to be used once each before and after processing
inline float hann_root(float a0, float x) {
    return std::sqrt(2.0 *(a0 - 1)) * std::sin(MATH_PI * x);
}

// the hann function, centered around 0
// inline float hann_centered(float a0, float x) {
//     return hann(a0, (x + 0.5f));
// }


// windows are 1 at 1, 0 at 0
inline float hann_window(float x) {
    return hann(0.5, 0.5f *  x);
}

inline float hamming_window(float x) {
    return hann((float)25 / 46, 0.5f * x);
}

// windows are 1 at 1, 0 at 0
inline float hann_window_root(float x) {
    return std::sin(0.5 * MATH_PI * x);
}

// does windowing to both ends of a sequence, in place
template<class T>
inline void window_ends(T* a, uint32_t a_size, uint32_t window_size, float window_f (float)) {
    window_size = MIN(a_size / 2, window_size);
    for (uint32_t i = 0; i < window_size; i++) {
        const float w = window_f((float) i / window_size);

        a[i] *= w;
        a[a_size - 1 - i] *= w;
    }
}

// added the tail and head of an array after applying a window function to bothe sides
template<class T>
inline void overlap_add(const T *prev, const T *next, T *output, uint32_t size, float window_f (float)) {
    for (uint32_t i = 0; i < size; i++) {
        const float w = window_f((float) i / size);

        output[i] = w * next[i] + (1- w) * prev[i];
    }
}

// overlap and extend after applying a window function first
// overlap_size may be larger than new_data.size(), in which case only new_data.size() is added. the offset is the same
template<class T, class NewT>
inline void mix_and_extend(T &array, const NewT &new_data, const uint32_t &overlap_size, float window_f (float)) {
    
    uint32_t i = 0;

    for(; i < MIN(overlap_size, new_data.size()); i++) {
        float w = (float) i / overlap_size;
        float w1 = window_f(w);
        float w2 = window_f(1.0-w);
        w2 = 1.0f - w1;
        uint32_t ind = array.size() - overlap_size + i;
        array[ind] = array[ind] * w2 + new_data[i] * w1;
    }

    for (; i < new_data.size(); i++) {
        array.push_back(new_data[i]);
    }

}

}
}


#endif