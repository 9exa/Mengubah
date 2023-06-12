#include "dsp/fft.h"
#include "dsp/common.h"
#include "dsp/fastmath.h"
#include "mengumath.h"
#include <cstdint>
#include <vector>



using namespace Mengu;

// helpers

static void conjugate_arr(Complex *arr, const uint32_t a_size) {
    for (uint32_t i = 0; i < a_size; i++) {
        arr[i].imag(-arr[i].imag());
    }
}

std::vector<Complex> dsp::SFT::perform(const std::vector<Complex> &input) {
    const size_t outsize = input.size();
    const Complex mitau(0, -MATH_TAU);

    Complex *es = new Complex[outsize];
    for (size_t j = 0; j < outsize; j++) {
        float n = (float) j / outsize;
        es[j] = std::exp(mitau * n);
        // std::cout << es[j] << std::endl;
    }

    std::vector<Complex> outvec;
    outvec.resize(outsize);
    for (size_t k = 0; k < outsize; k++) {
        for (size_t n = 0; n < outsize; n++) {
            outvec[k] += input[n] * es[n * k % outsize];
        }
        outvec[k] /= outsize;
    }

    delete[] es;

    return outvec;
}

dsp::FFT::FFT(uint32_t size) {
    _fft_size = is_pow_2(size) ? size : next_pow_2(size);
    _size = size;
    _es = new complex<float>[_fft_size];
    for (uint32_t i = 0; i < _fft_size; i++) {
        _es[i] = std::polar(1.0f, -(float) MATH_TAU * i / _fft_size);
    }
    _inp_vec = new Complex[_fft_size];
    _out_vec = new Complex[_fft_size];
}

dsp::FFT::~FFT() {
    delete[] _es;
    delete[] _inp_vec;
    delete[] _out_vec;
}

void dsp::FFT::transform(const Complex *input, Complex *output) const {
    for (uint32_t i = 0; i < _size; i++) {
        _inp_vec[i] = input[i];
    }
    for (uint32_t i = _size; i < _fft_size; i++) {
        _inp_vec[i] = Complex(0.0f);
    }

    _transform_rec(_inp_vec, _out_vec, _fft_size, 1);

    for (uint32_t i = 0; i < _size; i++) {
        output[i] = _out_vec[i] / sqrtf((float) _fft_size);
    }
}

void dsp::FFT::transform(const CycleQueue<Complex>&input, Complex *output) const {
    // _transform_rec(input, 0, output, _size, 1);
    input.to_array(_inp_vec);

    // zero pad the  input
    for (uint32_t i = input.size(); i < _fft_size; i++) {
        _inp_vec[i] = 0.0f;
    }
    

    _transform_rec(_inp_vec, _out_vec, _fft_size, 1);
    for (uint32_t i = 0; i < _size / 2; i++) {
        output[i] = _out_vec[i] / sqrtf((float) _fft_size);
    }
}

void dsp::FFT::inverse_transform(const Complex *input, Complex *output) const {

    for (uint32_t i = 0; i < _size; i++) {
        _inp_vec[i] = input[i];
    }
    // zero pad input buffer
    for (uint32_t i = _size; i < _fft_size; i++) {
        _inp_vec[i] = 0.0f;
    }


    conjugate_arr(_inp_vec, _size);
    _transform_rec(_inp_vec, _out_vec, _fft_size, 1);
    conjugate_arr(_out_vec, _size);

    for (uint32_t i = 0; i < _size; i++) {
        // only inverse-transforming first half of the freq spectrum. so multiply by 2
        output[i] = _out_vec[i] / sqrtf((float) _fft_size);
    }
}

void dsp::FFT::_transform_rec(const Complex *input, Complex *output, const uint32_t N, const uint32_t stride) const {
    // recursive implementaion of radix-2 fft
    // Thanks wikipedia
    if (N == 1) {
        output[0] = input[0];
        return;
    }

    _transform_rec(input, output, N / 2, 2 * stride);
    _transform_rec(input + stride, output + N / 2, N / 2, 2 * stride);
    for (uint32_t k = 0; k < N / 2; k++) {
        const Complex e = _es[k * stride];
        const Complex p = output[k]; // kth even
        const Complex q = e * output[k + N / 2]; //kth odd
        output[k] = p + q;
        output[k + N / 2] = p - q;
    }
}

void dsp::FFT::_transform_rec(const CycleQueue<Complex> &input, uint32_t inp_ind, Complex *output, const uint32_t N, const uint32_t stride) const{
    // recursive implementaion of radix-2 fft
    // Thanks wikipedia
    if (N == 1) {
        output[0] = input[inp_ind];
        return;
    }

    _transform_rec(input, inp_ind, output, N / 2, 2 * stride);
    _transform_rec(input, inp_ind + stride, output + N / 2, N / 2, 2 * stride);

    for (uint32_t k = 0; k < N / 2; k++) {
        const Complex e = _es[k * stride];
        const Complex p = output[k]; // kth even
        const Complex q = e * output[k + N / 2]; //kth odd
        output[k] = p + q;
        output[k + N / 2] = p - q;
    }
}

const Complex *dsp::FFT::get_es() const {
    return _es;
}