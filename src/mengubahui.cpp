


#include "mengubahui.h"
#include "nanogui/screen.h"
#include <cstdint>

using namespace nanogui;
using namespace Mengu;
using namespace dsp;

MenugubahUI::MenugubahUI(): 
    Screen(Vector2i(1280, 720)), 
    _pitch_shifters({

    }) {}


void MenugubahUI::push_signal(const float *input, uint32_t size) {
    _cbuffer.resize(size);

    for (uint32_t i = 0; i < size; i++) {
        _cbuffer[i] = input[i];
    }

    _pitch_shifter->push_signal(_cbuffer.data(), size);
    _pitch_shifter->pop_transformed_signal(_cbuffer.data(), size);

    for (uint32_t i = 0; i < size; i++) {
        _transformed_buffer.push_back(_cbuffer[i].real());
    } 
}

uint32_t MenugubahUI::pop_transformed_signal(float *output, uint32_t size) {
    _transformed_buffer.pop_front_many(output, size);
    return size;
}

