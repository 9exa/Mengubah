#include <dsp/effect.h>
#include <mengumath.h>


Mengu::dsp::EffectChain::EffectChain(uint32_t buffer_size) {
    _buffer_size = Mengu::last_pow_2(buffer_size);
    _input_buffer.resize(_buffer_size);
    _transformed_buffer.resize(_buffer_size);
}

Mengu::dsp::EffectChain::~EffectChain() {
    for (auto effect: _effects) {
        
    }
}

void Mengu::dsp::EffectChain::push_signal(const Complex *input, const uint32_t &size) {
    for (uint32_t i = 0; i < size; i++) {
        _input_buffer.push_back(input[i]);
    }
}

void Mengu::dsp::EffectChain::pop_transformed_signal(Complex *output, const uint32_t &size) {
    for (uint32_t i = _buffer_size - size; i < _buffer_size; i++) {
        output[i] = _transformed_buffer[i];
    }
}

void Mengu::dsp::EffectChain::append_effect(Effect *effect) {
    _effects.push_back(effect);
}