
/**
 * @brief UI for the Mengubah plugin, format agnostic
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
#include "mengubahengine.h"
#include <cstdint>
#include <vector>

#include <nanogui/screen.h>


namespace Mengu {

using namespace nanogui;
using namespace dsp;

class MengubahUI : public Screen {
public:
    static constexpr uint32_t FramesPerSecond = 30;

    MengubahUI(MengubahEngine *engine);
    ~MengubahUI();

private:
    MengubahEngine *_engine;
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