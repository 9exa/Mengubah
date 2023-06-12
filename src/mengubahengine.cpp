#include "mengubahengine.h"
#include "dsp/formantshifter.h"

using namespace Mengu;
using namespace dsp;

MengubahEngine::MengubahEngine():
    _pitch_shifters({
        new TimeStretchPitchShifter(new WSOLATimeStretcher(), 1),
        new TimeStretchPitchShifter(new PhaseVocoderTimeStretcher(), 1),
        new TimeStretchPitchShifter(new PhaseVocoderDoneRightTimeStretcher(), 1),
    }),
    _formant_shifters({
        new LPCFormantShifter(),
        new TimeStretchPitchShifter(new PSOLATimeStretcher(), 1)
    }) {
    _pitch_shifter = _pitch_shifters[0];
    _formant_shifter = _formant_shifters[0];
}
MengubahEngine::~MengubahEngine() {
    for (Effect *pitch_shifter : _pitch_shifters) {
        delete pitch_shifter;
    }
    for (Effect *formant_shifter : _formant_shifters) {
        delete formant_shifter;
    }
}

void MengubahEngine::push_signal(const float *input, uint32_t size) {
    _cbuffer.resize(size);

    for (uint32_t i = 0; i < size; i++) {
        _cbuffer[i] = Complex(input[i]);
    }

    // output size assumed to immediately be the same as the output size
    _pitch_shifter->push_signal(_cbuffer.data(), size);
    _pitch_shifter->pop_transformed_signal(_cbuffer.data(), size);

    for (uint32_t i = 0; i < size; i++) {
        _transformed_buffer.push_back(_cbuffer[i].real());
    } 
}

uint32_t MengubahEngine::pop_transformed_signal(float *output, uint32_t size) {
    _transformed_buffer.pop_front_many(output, size);
    return size;
}

