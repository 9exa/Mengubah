#include "mengupitchy.h"
#include "audioplayers/Audioplayer.h"
#include "dsp/effect.h"
#include "dsp/fft.h"
#include "gui/effectcontrol.h"
#include "gui/lineplot.h"
#include "gui/lineplotgpu.h"
#include "mengumath.h"
#include "nanogui/combobox.h"
#include "nanogui/common.h"
#include "nanogui/label.h"
#include "nanogui/layout.h"
#include "nanogui/popup.h"
#include "nanogui/vector.h"
#include "nanogui/widget.h"
#include "nanogui/window.h"
#include "templates/cyclequeue.h"
#include <complex>
#include <cstddef>
#include <cstdint>
#include <nanogui/button.h>
#include <filesystem>
#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>


#define FILEDIALOG_OPEN_COMMAND "zenity --file-selection --file-filter=\'Music files (ogg,wav,mp3) | *.ogg *.wav *.mp3\'"

MenguPitchy::MenguPitchy(): 
    nanogui::Screen(
        nanogui::Vector2i(1280, 720), 
        "Menga Replayer", 
        false),
    _fft(NSamples) {
    using namespace nanogui;
    using namespace Mengu;
    
    _root = new Widget(this);
    _root->set_fixed_size(size());
    _root->set_size(size());

    //audio
    _audio_player.sample_buffer.resize(NSamples);

    //gui
    BoxLayout *fill_layout = new BoxLayout(Orientation::Vertical);
    fill_layout->set_alignment(Alignment::Fill);
    _root->set_layout(fill_layout);
    
    _sample_graph = new LinePlotGPU(_root, "G");
    _sample_graph->set_min_value(-1.0f);
    _sample_graph->set_max_value(1.0f);
    _sample_graph->get_values().resize(NSamples);

    // display the (phase invariant) amplitudes of each frequency
    // half the number of sample because of symmytre and the nyquil freq
    _freq_graph = new LinePlotGPU(_root, "Freqs");
    _freq_graph->get_values().resize(NSamples);   

    // File loading 
    Widget *file_label_parent = new Widget(_root);
    file_label_parent->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Middle));
    file_label_parent->set_fixed_width(_root->width());
    
    Label *file_label = new Label(file_label_parent, "No File loaded");
    file_label->set_fixed_height(40);
    file_label->set_font_size(30);

    Button *file_button = new Button(_root, "Load File");
    file_button->set_callback([this, file_label] () {
        // load a new audio file
        char filename[1024] = {'\0'};
        FILE *f = popen(FILEDIALOG_OPEN_COMMAND, "r");
        if (fgets(filename, 1024, f) != nullptr) {
            char *newline_at = strrchr(filename, '\n');
            if (newline_at != nullptr) {
                *newline_at = '\0'; // remove the newline character at the end
            }

            
            if (this->_audio_player.load_file(filename)) {
                file_label->set_caption("Could Not Load selected File.");
            }
            else {
                file_label->set_caption(filename);
            }
            this->perform_layout();
        }
        pclose(f);
    });


    Button *play_button = new Button(_root, "Play");
    play_button->set_callback([this] () {
        this->_audio_player.play();
    });

    Button *stop_button = new Button(_root, "Stop");
    stop_button->set_callback([this] () {
        this->_audio_player.stop();
    });
    
    EffectControl *stretcher_control = new EffectControl(_root, "Pitch Shifter");
    stretcher_control->display_property_list(_audio_player.pitch_shifter->get_property_descs());
    stretcher_control->set_callback(
        [this] (uint32_t ind, EffectPropPayload data) {_audio_player.pitch_shifter->set_property(ind, data); }
    );

    const char* def_effect_name = "WSOLA Pitch Shifter";
    Button *effect_selection_button = new Button(_root, def_effect_name);
    effect_selection_button->set_flags(Button::Flags::ToggleButton);

    Window *effect_selection_window = new Window(this);
    effect_selection_window->set_visible(false);
    new Label(effect_selection_window, "Select Pitch Shifting Algorithm");

    std::string effect_names[AudioPlayer::NPitchShifters] = {
        "WSOLA Pitch Shifter",
        "PSOLA Formant Shifter",
        "Phase Vocoder",
        "Phase Vocoder '''Done right'''",
        "Formant Shifter",
    };
    for (uint32_t i = 0; i < AudioPlayer::NPitchShifters; i++) {
        std::string effect_name = effect_names[i];
        Button *e_button = new Button(
            effect_selection_window, effect_name
        );
        e_button->set_callback([this, i, stretcher_control, effect_selection_window, effect_selection_button, effect_name] () {
            _audio_player.set_pitch_shifter(i);
            stretcher_control->display_property_list(
                _audio_player.pitch_shifter->get_property_descs()
            );
            stretcher_control->set_effect_name(effect_name);
            effect_selection_window->set_visible(false);
            effect_selection_button->set_pushed(false);
            effect_selection_button->set_caption(effect_name);
            
            perform_layout();
        });
    }
    effect_selection_window->set_title("");
    effect_selection_window->set_layout(new BoxLayout(
        Orientation::Vertical,
        Alignment::Fill,
        15,
        6
    ));

    effect_selection_button->set_change_callback([effect_selection_window] (bool pressed) {
        effect_selection_window->set_visible(pressed);
        effect_selection_window->center();
    });

    set_resize_callback([this] (const Vector2i &new_size) {
        _root->set_fixed_size(new_size);
        perform_layout();
    });

    perform_layout();


    _draw_plots();

}

MenguPitchy::~MenguPitchy() {
    
}
void MenguPitchy::draw_all() {
    if (m_redraw) {
        _draw_plots();
    }
    nanogui::Screen::draw_all();
}

void MenguPitchy::_draw_plots() {
    // Draw Samples
    std::vector<float> &vals = _sample_graph->get_values();
    std::vector<std::complex<float>> complex_vals;
    complex_vals.resize(NSamples);

    _audio_player.sample_buffer.to_array(vals.data());
    
    
    for (uint32_t i = 0; i < NSamples; i++) {
        complex_vals[i] = std::complex<float>(vals[i], 0.0f);
    }
    
    // Redraw frequencies
    std::vector<Complex> complex_freq;
    complex_freq.resize(NSamples, Complex(0,0));
    _fft.transform(complex_vals.data(), complex_freq.data());
    std::vector<float> &real_freq = _freq_graph->get_values();
    for (size_t i = 0; i < NSamples ; i++) {
        real_freq[i] = 2 * sqrtf(std::norm(complex_freq[i]));
    }
}

float MenguPitchy::_display_func(float x, float freq, float offset) {
    static const float SampleScale = 1.0f;
    const float wave1 = 0.5 * Mengu::dsp::cos((freq * x * SampleScale + offset) * MATH_TAU);
    const float wave2 = 0.3 * Mengu::dsp::cos(0.4f * (freq * x * SampleScale + offset) * MATH_TAU);
    const float wave3 = -0.0 * Mengu::dsp::cos(0.2f * (freq * x * SampleScale + offset) * MATH_TAU);
    return wave1 + wave2 + wave3;
}

int main(int argc, char** argv) {

    nanogui::init();
    MenguPitchy *app = new MenguPitchy();
    app->set_visible(true);
    app->redraw();
    
    // unfortunatley we have to set the frameratehere
    try {
        nanogui::mainloop(1.0 / MenguPitchy::FramesPerSecond * 1'000);

    }
    catch(std::exception e) {
        std::cout << e.what() << std::endl;
    }

    nanogui::shutdown();

    

    delete app;
    return 0;
}