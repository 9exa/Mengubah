


#include "mengubahui.h"
#include "dsp/effect.h"
#include "dsp/formantshifter.h"
#include "dsp/pitchshifter.h"
#include "dsp/timestretcher.h"
#include "gui/effectcontrol.h"
#include "gui/inputslider.h"
#include "gui/lineplotgpu.h"
#include "gui/vcombobox.h"
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

MengubahUI::MengubahUI(): 
    Screen(Vector2i(1280, 720), "Mengubah. A Real Time Pitch Shifter"), 
    _pitch_shifters({
        new TimeStretchPitchShifter(new WSOLATimeStretcher(), 1),
        new TimeStretchPitchShifter(new PhaseVocoderTimeStretcher(), 1),
        new TimeStretchPitchShifter(new PhaseVocoderDoneRightTimeStretcher(), 1),
    }),
    _formant_shifters({
        new LPCFormantShifter(),
        new TimeStretchPitchShifter(new PSOLATimeStretcher(), 1)
    }) {
    // setup audio components
    _pitch_shifter = _pitch_shifters[0];
    _formant_shifter = _formant_shifters[0];
    
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
    formant_control->display_property_list(_formant_shifters[0]->get_property_descs());
    formant_control->set_callback([this] (uint32_t ind, EffectPropPayload data) {
        _formant_shifter->set_property(ind, data);
    });
    VComboBox *formant_combo = new VComboBox(formant_panel, "Select Formant Shifter");
    formant_combo->set_items(std::vector(FormantShifterNames.begin(), FormantShifterNames.end()));
    formant_combo->set_selected_index(0);
    formant_combo->set_callback([this, formant_control, formant_combo] (uint32_t selected_ind) {
        _formant_shifter = _formant_shifters[selected_ind];
        _formant_shifter->reset();
        formant_control->display_property_list(_formant_shifter->get_property_descs());
        formant_combo->close();
        perform_layout();
    });

    Widget *pitch_panel = new Widget(this);
    pitch_panel->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Fill));
    pitch_panel->set_position({width() * 1 / 2, height() * 1 / 2 });
    pitch_panel->set_fixed_size({width() * 5 / 10, 500});

    EffectControl *pitch_control = new EffectControl(pitch_panel, PitchShifterNames[0]);
    pitch_control->display_property_list(_pitch_shifters[0]->get_property_descs());
    pitch_control->set_callback([this] (uint32_t ind, EffectPropPayload data) {
        _pitch_shifter->set_property(ind, data);
    });
    VComboBox *pitch_combo = new VComboBox(pitch_panel, "Select Pitch Shifter");
    pitch_combo->set_items(std::vector(PitchShifterNames.begin(), PitchShifterNames.end()));
    pitch_combo->set_selected_index(0);
    pitch_combo->set_callback([this, pitch_control, pitch_combo] (uint32_t selected_ind) {
        _pitch_shifter = _pitch_shifters[selected_ind];
        _pitch_shifter->reset();
        pitch_control->display_property_list(_formant_shifter->get_property_descs());
        pitch_combo->close();
        perform_layout();
    });


    perform_layout();
}

MengubahUI::~MengubahUI() {
    for (Effect *pitch_shifter : _pitch_shifters) {
        delete pitch_shifter;
    }
    for (Effect *formant_shifter : _formant_shifters) {
        delete formant_shifter;
    }
}

void MengubahUI::push_signal(const float *input, uint32_t size) {
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

uint32_t MengubahUI::pop_transformed_signal(float *output, uint32_t size) {
    _transformed_buffer.pop_front_many(output, size);
    return size;
}

