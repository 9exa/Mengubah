


#include "mengubahui.h"
#include "dsp/effect.h"
#include "dsp/formantshifter.h"
#include "dsp/pitchshifter.h"
#include "dsp/timestretcher.h"
#include "gui/effectcontrol.h"
#include "gui/inputslider.h"
#include "gui/lineplotgpu.h"
#include "gui/vcombobox.h"
#include "mengubahengine.h"
#include "nanogui/layout.h"
#include "nanogui/screen.h"
#include "nanogui/widget.h"
#include "nanogui/label.h"
#include <cstdint>

using namespace nanogui;
using namespace Mengu;
using namespace dsp;

static const std::array<std::string, 2> FormantShifterNames {
    "LPC Formant Shifter",
    "PSOLA Formant Shifter",
};

static const std::array<std::string, 3> PitchShifterNames {
    "WSOLA Pitch Shifter",
    "Phase Vocoder Pitch Shifter",
    "Phase Vocoder 'Done Right' Pitch Shifter",
}; 

MengubahUI::MengubahUI(MengubahEngine *engine): 
    Screen(Vector2i(1280, 720), "Mengubah. A Real Time Pitch Shifter") {
    // the _engine can exist outsize the scope of this ui and must be cleaned up enlsewhere
    _engine = engine;    

    // setup gui
    _back_panel = new Widget(this);
    _back_panel->set_fixed_size(size());
    _back_panel->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 5, 5));

    for (LinePlotGPU **graph: {&_shifted_sample_graph, &_shifted_freqs_graph, &_shifted_envelope_graph}) {
        *graph = new LinePlotGPU(_back_panel);
        (*graph)->set_min_value(0.0f);
        (*graph)->set_max_value(1.0f);
        (*graph)->get_values().resize(NGraphSamples);
    }
    _shifted_sample_graph->set_min_value(-1.0f); // The only one that can do negative numbers

    Widget *formant_panel = new Widget(this);
    formant_panel->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Fill));
    formant_panel->set_position({0, height() * 1 / 2 });
    formant_panel->set_fixed_size({width() * 5 / 10, 500});

    EffectControl *formant_control = new EffectControl(formant_panel, FormantShifterNames[0]);
    formant_control->display_property_list(_engine->_formant_shifter->get_property_descs());
    formant_control->set_callback([this] (uint32_t ind, EffectPropPayload data) {
        _engine->_formant_shifter->set_property(ind, data);
    });
    VComboBox *formant_combo = new VComboBox(formant_panel, "Select Formant Shifter");
    formant_combo->set_items(std::vector(FormantShifterNames.begin(), FormantShifterNames.end()));
    formant_combo->set_selected_index(0);
    formant_combo->set_callback([this, formant_control, formant_combo] (uint32_t selected_ind) {
        _engine->_formant_shifter = _engine->_formant_shifters[selected_ind];
        _engine->_formant_shifter->reset();
        formant_control->display_property_list(_engine->_formant_shifter->get_property_descs());
        formant_combo->close();
        perform_layout();
    });

    Widget *pitch_panel = new Widget(this);
    pitch_panel->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Fill));
    pitch_panel->set_position({width() * 1 / 2, height() * 1 / 2 });
    pitch_panel->set_fixed_size({width() * 5 / 10, 500});

    EffectControl *pitch_control = new EffectControl(pitch_panel, PitchShifterNames[0]);
    pitch_control->display_property_list(_engine->_pitch_shifter->get_property_descs());
    pitch_control->set_callback([this] (uint32_t ind, EffectPropPayload data) {
        _engine->_pitch_shifter->set_property(ind, data);
    });
    VComboBox *pitch_combo = new VComboBox(pitch_panel, "Select Pitch Shifter");
    pitch_combo->set_items(std::vector(PitchShifterNames.begin(), PitchShifterNames.end()));
    pitch_combo->set_selected_index(0);
    pitch_combo->set_callback([this, pitch_control, pitch_combo] (uint32_t selected_ind) {
        _engine->_pitch_shifter = _engine->_pitch_shifters[selected_ind];
        _engine->_pitch_shifter->reset();
        pitch_control->display_property_list(_engine->_pitch_shifter->get_property_descs());
        pitch_combo->close();
        perform_layout();
    });


    perform_layout();
}

MengubahUI::~MengubahUI() {}
