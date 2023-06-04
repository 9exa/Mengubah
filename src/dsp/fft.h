#ifndef MENGA_FFT
#define MENGA_FFT

#include <valarray>
#include <vector>
#include "dsp/common.h"
#include <templates/cyclequeue.h>

typedef std::complex<float> Complex;
typedef std::valarray<Complex> CArray;

namespace Mengu {
namespace dsp {
using namespace std;

class SFT {
    // Slow O(N^2) Fourier Transform
public:
    // SFT();
    // out-of-place ft 
    vector<Complex> perform(const vector<Complex> &input);
private:
    vector<Complex> _es;
    // vector<Complex> _sines;
};

class FFTBuffer; // foward declaration

class FFT {
public:
    // Fast Fourier transform that uses lookup tables and performs onto an established array
    // Only works for arrays of a declared size. Designed to be cached and use many times
    // To use it for different sample rates/lengths, create a new FFT of a different size

    // The size needs to be a power of 2 in order for this to work properly
    FFT(uint32_t size);
    ~FFT();


    // Both arrays must be at least as long as _size
    void transform(const Complex *input, Complex *output) const;
    void transform(const CycleQueue<Complex> &input, Complex *output) const; // to avoid having to reallocate array
    
    //unlike 'transform' this edits *input to avoid unnecissary memory allocation
    void inverse_transform(const Complex *input, Complex *output) const;

    const Complex *get_es() const;



private:
    Complex *_es;
    uint32_t _size;
    uint32_t _fft_size; // size of arrays used in fft computations. must be a power of 2

    void _transform_rec(const Complex *input, Complex *output, const uint32_t N, const uint32_t stride) const;
    void _transform_rec(const CycleQueue<Complex> &input, uint32_t inp_ind, Complex *output, const uint32_t N, const uint32_t stride) const;

    friend class FFTBuffer;
public:
    uint32_t size() const { return _size; }

};

class FFTBuffer {
    // class designed to do update a n FFT of a signal in real time
public:
    void push_signal(const Complex *x, const uint32_t &size) {}

    // copies the last 'size'
    void pop_transformed_signal(const Complex *output, const uint32_t &size) {}
private:
    CycleQueue<Complex> _buffer;
};

};
};

#endif