
/**
 * @brief Engine and UI for the Mengubah plugin, format agnostic
 * 
 */
#ifndef MENGU_MENGUBAH_UI
#define MENGU_MENGUBAH_UI

#include "dsp/common.h"
#include "dsp/effect.h"
#include "dsp/pitchshifter.h"
#include "gui/inputslider.h"
#include "gui/lineplotgpu.h"
#include "templates/vecdeque.h"
#include <cstdint>
#include <vector>

#include <nanogui/screen.h>


namespace Mengu {

using namespace nanogui;
using namespace dsp;

class MengubahUI : public Screen {
public:
    static constexpr uint32_t FramesPerSecond = 30;

    MengubahUI();
    ~MengubahUI();

    void push_signal(const float *input, uint32_t size);
    uint32_t pop_transformed_signal(float *output, uint32_t size);

private:
    dsp::PitchShifter *_pitch_shifter;
    dsp::Effect *_formant_shifter;
    VecDeque<float> _transformed_buffer;

    const std::vector<PitchShifter *> _pitch_shifters;
    const std::vector<Effect *> _formant_shifters;


    // intermediary buffer
    std::vector<Complex> _cbuffer;

    static constexpr uint32_t NGraphSamples = 1 << 10;

    // gui handles
    Widget *_back_panel;
    LinePlotGPU *_shifted_sample_graph;
    LinePlotGPU *_shifted_freqs_graph;
    LinePlotGPU *_shifted_envelope_graph;
    InputSlider *_pitch_input_slider;
    InputSlider *_formant_input_slider;
};

}


#endif