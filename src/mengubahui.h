
/**
 * @brief Engine and UI for the Mengubah plugin, format agnostic
 * 
 */
#ifndef MENGU_MENGUBAH_UI
#define MENGU_MENGUBAH_UI

#include "dsp/common.h"
#include "dsp/effect.h"
#include "dsp/pitchshifter.h"
#include "templates/vecdeque.h"
#include <cstdint>
#include <vector>

#include <nanogui/screen.h>

using namespace nanogui;

namespace Mengu {

class MenugubahUI : public Screen {
public:
    MenugubahUI();

    void push_signal(const float *input, uint32_t size);
    uint32_t pop_transformed_signal(float *output, uint32_t size);

private:
    dsp::PitchShifter *_pitch_shifter;
    VecDeque<float> _transformed_buffer;

    // intermediary buffer
    std::vector<Complex> _cbuffer;
};

}


#endif