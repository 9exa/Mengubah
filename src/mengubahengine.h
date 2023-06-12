/**
 * @file mengubahengine.h
 * @author 9exa
 * @brief The audio component of the Mengubah plugin. Just applies a formant shift before a pitch shift
 * @version 0.1
 * @date 2023-06-12
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef MENGUBAH_ENGINE
#define MENGUBAH_ENGINE

#include "dsp/effect.h"
#include "dsp/pitchshifter.h"

#include <cstdint>
#include <vector>

namespace Mengu {
using namespace dsp;

class MengubahEngine {
public:
    MengubahEngine();
    ~MengubahEngine();

    void push_signal(const float *input, uint32_t size);
    uint32_t pop_transformed_signal(float *output, uint32_t size);

    friend class MengubahUI;

private:
    PitchShifter *_pitch_shifter;
    Effect *_formant_shifter;
    VecDeque<float> _transformed_buffer;

    const std::vector<PitchShifter *> _pitch_shifters;
    const std::vector<Effect *> _formant_shifters;

    // intermediary buffer
    std::vector<Complex> _cbuffer;
};

}

#endif