#include "RealTimeChanger.h"
#include "dsp/effect.h"
#include "dsp/formantshifter.h"
#include "dsp/pitchshifter.h"
#include "dsp/timestretcher.h"
#include "gui/effectcontrol.h"
#include "gui/lineplotgpu.h"
#include "gui/vcombobox.h"
#include "mengumath.h"
#include "nanogui/common.h"
#include "nanogui/layout.h"
#include "nanogui/screen.h"
#include "nanogui/slider.h"
#include "nanogui/vector.h"
#include "nanogui/widget.h"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <math.h>
#include <vector>


Mengu::RealTimeChanger::RealTimeChanger() :
    nanogui::Screen(
        nanogui::Vector2i(1280, 720), 
        "Mengubah Real Time Changer",
        false
    ) {
    
    using namespace nanogui;

    _root = new Widget(this);
    _root->set_fixed_size(size());
    BoxLayout *fill_layout = new BoxLayout(Orientation::Vertical);
    fill_layout->set_alignment(Alignment::Fill);
    _root->set_layout(fill_layout);

    _raw_graph = new LinePlotGPU(_root, "Raw Mic");
    _raw_graph->get_values().resize(capture.raw_bufferf.size());

    _effect_control_container = new Widget(_root);
    _effect_control_container->set_layout(new BoxLayout(
        Orientation::Vertical, 
        Alignment::Fill,
        10, 4
    ));

    const std::vector<std::string> effect_names = {
        "WSOLA Pitch Shifter",
        "PSOLA Formant Shifter",
        "Phase Vocoder",
        "Phase Vocoder '''Done right'''",
    };
    _effect_select_combobox = new VComboBox(_root, "Add Effect");
    _effect_select_combobox->set_items(std::vector(EffectNames.begin(), EffectNames.end()));
    _effect_select_combobox->set_callback([this] (int ind) {
        _add_effect((EffectCode) ind);
        _effect_select_combobox->close();
    });    

    
    set_resize_callback([this] (const Vector2i &new_size) {
        _root->set_fixed_size(new_size);
        perform_layout();
    });
    perform_layout();
}

Mengu::RealTimeChanger::~RealTimeChanger() {}

void Mengu::RealTimeChanger::draw_all() {
    std::vector<float> &raw = _raw_graph->get_values();
    capture.raw_bufferf.to_array(raw.data(), raw.size());

    nanogui::Screen::draw_all();
}

void Mengu::RealTimeChanger::_add_effect(EffectCode e_code) {
    dsp::Effect *new_effect;
    switch (e_code) {
        case WSOLA:
            new_effect = new dsp::TimeStretchPitchShifter(new dsp::WSOLATimeStretcher(), 1);
            break;
        case PSOLA:
            new_effect = new dsp::TimeStretchPitchShifter(new dsp::PSOLATimeStretcher(), 1);
            break;
        case PhaseVocoder:
            new_effect = new dsp::TimeStretchPitchShifter(new dsp::PhaseVocoderTimeStretcher(true), 1);
            break;
        case PhaseVocoderDoneRight:
            new_effect = new dsp::TimeStretchPitchShifter(new dsp::PhaseVocoderDoneRightTimeStretcher(true), 1);
            break;
        case LPCFormant:
            new_effect = new dsp::LPCFormantShifter();
    };

    capture.add_effect(new_effect);

    // Adds an effect control to edit the effects parameters. 
    // Assumes the controls position in _effect_controls is equal to the corresponding effects index in capture._effects
    uint32_t e_ind = capture.get_effects().size() - 1;
    EffectControl *effect_control;
    if (_effect_controls.size() <= e_ind) {
        effect_control = new EffectControl(_effect_control_container);
        effect_control->set_effect_name(EffectNames[e_code]);
        effect_control->set_callback([this, e_ind] (uint32_t ind, dsp::EffectPropPayload data) {
            capture.get_effects()[e_ind]->set_property(ind, data);
        });
        effect_control->set_delete_button_enabled(true);
        effect_control->set_delete_button_callback([this, effect_control] () {
            const auto effect_iter = std::find(
                _effect_controls.cbegin(),
                _effect_controls.cend(),
                effect_control
            );
            if (effect_iter != _effect_controls.cend()) {
                const uint32_t ind = effect_iter - _effect_controls.cbegin();
                _remove_effect(ind);
            }
        });
        _effect_controls.push_back(effect_control);
    }
    else {
        effect_control = _effect_controls[e_ind];
        effect_control->set_visible(true);
    }
    effect_control->display_property_list(new_effect->get_property_descs());

    perform_layout();
}

void Mengu::RealTimeChanger::_remove_effect(uint32_t at) {
    capture.remove_effect(at);
    // update the effect controls
    const std::vector<Effect *> &effects = capture.get_effects();
    for (uint32_t i = 0; i < effects.size(); i++) {
        _effect_controls[i]->display_property_list(effects[i]->get_property_descs());
    }
    _effect_controls[effects.size()]->set_visible(false);
    perform_layout();
}

int main(int argc, char **argv) {
    nanogui::init();

    Mengu::RealTimeChanger *app = new Mengu::RealTimeChanger();
    app->set_visible(true);
    app->draw_all();

    try {
        nanogui::mainloop((1.0 / Mengu::RealTimeChanger::FramesPerSecond) * 1'000);

    }
    catch(std::exception e) {
        std::cout << e.what() << std::endl;
    }
    nanogui::shutdown();

    delete app;

    return 0;
}