#ifndef MENGA_REAL_TIME_CHANGER
#define MENGA_REAL_TIME_CHANGER

#include <array>
#include <cstdint>
#include <nanogui/common.h>
#include <nanogui/vector.h>
#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/slider.h>
#include <nanogui/layout.h>
#include <nanogui/button.h>
#include <string>

#include "audioplayers/Microphoneaudiocapture.h"
#include "gui/effectcontrol.h"
#include "gui/lineplotgpu.h"
#include "gui/vcombobox.h"

namespace Mengu {
class RealTimeChanger : public nanogui::Screen {
public:
    RealTimeChanger();
    ~RealTimeChanger();

    static constexpr uint32_t FramesPerSecond = 60;

    virtual void draw_all() override;
private:
    MicrophoneAudioCapture capture;
    LinePlotGPU *_raw_graph;

    static inline const std::array<std::string, 4> EffectNames {
        "WSOLA Pitch Shifter",
        "PSOLA Pitch Shifter",
        "Phase Vocoder",
        "Phase Vocoder '''Done right'''"
    };

    enum EffectCode {
        WSOLA,
        PSOLA,
        PhaseVocoder,
        PhaseVocoderDoneRight,
    };

    // Used as a proxy for the whole main screen, so we can still have Child Windows moving around
    Widget *_root;
    
    Widget *_effect_control_container;

    std::vector<EffectControl *> _effect_controls;


    VComboBox *_effect_select_combobox;

    void _add_effect(EffectCode effect_code);
    void _remove_effect(uint32_t at);
};

}


#endif
