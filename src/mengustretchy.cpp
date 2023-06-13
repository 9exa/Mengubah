#include <complex>
#include <cstdint>
#include <nanogui/common.h>
#include <nanogui/vector.h>
#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/slider.h>
#include <nanogui/layout.h>
#include <nanogui/button.h>
#include <nanogui/widget.h>
#include <vector>

#include "audioplayers/TimeStretchAudioplayer.h"
#include "dsp/common.h"
#include "dsp/effect.h"
#include "dsp/fft.h"
#include "dsp/interpolation.h"
#include "dsp/linalg.h"
#include "dsp/timestretcher.h"
#include "gui/effectcontrol.h"
#include "gui/lineplotgpu.h"
#include "gui/vcombobox.h"
#include "gui/filedialog.h"
#include "mengumath.h"
#include "nanogui/label.h"
#include "templates/cyclequeue.h"


#define FILEDIALOG_OPEN_COMMAND "zenity --file-selection --file-filter=\'Music files (ogg,wav,mp3) | *.ogg *.wav *.mp3\'"

using namespace Mengu;

static float _wrap_phase(float phase) {
    return fposmod(phase + MATH_PI, MATH_TAU) - MATH_PI;
}

// difference between the unwrapped phases of 2 complex numbers. since, phases are preiodinc, picks the closest one to the estimate
static float _phase_diff(Complex next, Complex prev, float est = 0.0f) {
    return _wrap_phase(std::arg(next) - std::arg(prev) - est) + est;
} 

class App: public nanogui::Screen {
public:
    static const uint32_t NSamples = 1 << 12;
    static const uint32_t FramesPerSecond = 120;

    static inline const float MinStretch = 1.0f;
    static inline const float MaxStretch = 3.0f;

    // CycleQueue<Complex> samples; 

    LinePlotGPU *base_graph;
    LinePlotGPU *freq_graph;

    TimeStretchAudioPlayer *audio_player;

    EffectControl *_stretcher_control;

    dsp::FFT fft;

    App() : nanogui::Screen(nanogui::Vector2i(1280, 720), "Recon test"),
            fft(TimeStretchAudioPlayer::BufferSize) {

        using namespace nanogui;

        // audio_player = nullptr;
        audio_player = new TimeStretchAudioPlayer();

        BoxLayout *fill_layout = new BoxLayout(Orientation::Vertical);
        fill_layout->set_alignment(Alignment::Fill);
        set_layout(fill_layout);

        base_graph = new LinePlotGPU(this, "base_graph");
        base_graph->get_values().resize(TimeStretchAudioPlayer::BufferSize);

        freq_graph = new LinePlotGPU(this, "Frequency Graph");
        freq_graph->get_values().resize(TimeStretchAudioPlayer::BufferSize / 2);
        freq_graph->set_min_value(0);

        Widget *file_label_parent = new Widget(this);
        file_label_parent->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Middle));
        file_label_parent->set_fixed_width(this->width());

        Label *file_label = new Label(file_label_parent, "No File loaded");
        file_label->set_fixed_height(40);
        file_label->set_font_size(30);

        Button *file_button = new Button(this, "Load File");
        file_button->set_callback([this, file_label] () {
            // load a new audio file
            std::string filename = open_file_dialog({{"wav", "WaveForm"}, {"mp3", "AudioPlayer"}, {"ogg", "Vorbis Audio"}});
            if (!filename.empty()) {
                if (audio_player->load_file(filename.data())) {
                    file_label->set_caption("File could not be loaded");
                }
                else {
                    file_label->set_caption(filename);
                }
                perform_layout();
            }

        });


        Widget *playing_buttons = new Widget(this);
        GridLayout *button_layout = new GridLayout(
            Orientation::Horizontal, 
            2, 
            Alignment::Fill,
            12,
            12
        );
        playing_buttons->set_layout(button_layout);


        Button *play_button = new Button(playing_buttons, "Play");
        play_button->set_callback([this] () {
            this->audio_player->play();
        });

        Button *stop_button = new Button(playing_buttons, "Stop");
        stop_button->set_callback([this] () {
            this->audio_player->stop();
        });

        _stretcher_control = new EffectControl(this, "Time Stretcher");
        _stretcher_control->display_property_list(audio_player->time_stretcher->get_property_descs());
        _stretcher_control->set_callback(
            [this] (uint32_t ind, EffectPropPayload data) {audio_player->time_stretcher->set_property(ind, data); }
        );

        VComboBox *effect_combo = new VComboBox(this, "Time Stretch Method");
        effect_combo->set_items({
            "Phase Vocoder TimeStretcher",
            "Phase Vocoder 'Done Right' TimeStretcher",
            "OLA TimeStretcher",
            "WSOLA TimeStretcher",
            "PSOLA TimeStretcher",
        });
        effect_combo->set_callback([this, effect_combo] (uint32_t ind) {
            audio_player->time_stretcher = audio_player->time_stretchers[ind];
            effect_combo->close();
        });
        
        perform_layout();

    }

    ~App() {
        if (audio_player != nullptr) {
            delete audio_player;
        }
    }

    virtual void draw_all() override {
        std::vector<Complex> samples = audio_player->sample_buffer.to_vector();

        std::vector<float> &base_values = base_graph->get_values();
        for (uint32_t i = 0; i < base_values.size(); i++) {
            base_values[i] = samples[i].real();
        }

        std::vector<Complex> complex_freq(samples.size());
        fft.transform(samples.data(), complex_freq.data());

        std::vector<float> &freq_values = freq_graph->get_values();
        for (uint32_t i = 0; i < freq_values.size() / 2; i++) {
            freq_values[i] = std::norm(complex_freq[i]);
        }

        nanogui::Screen::draw_all();
        perform_layout();
    }
};


int main() {    
    nanogui::init();

    App *app = new App();
    app->set_visible(true);
    app->draw_all();

    try {
        nanogui::mainloop(1.0 / App::FramesPerSecond * 1'000);

    }
    catch(std::exception e) {
        std::cout << e.what() << std::endl;
    }
    nanogui::shutdown();

    delete app;
    return 0;
}