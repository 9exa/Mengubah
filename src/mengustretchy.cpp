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
#include "mengumath.h"
#include "nanogui/label.h"
#include "templates/cyclequeue.h"


#define DIRPATH "/home/wes/Music"
#define FILEPATH "/home/wes/Music/Treasured Creation - SSN Ghost Peppered OST.ogg"
#define FILEDIALOG_OPEN_COMMAND "/usr/bin/zenity --file-selection --file-filter=\'Music files (ogg,wav,mp3) | *.ogg *.wav *.mp3\'"

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
    LinePlotGPU *windowed_graph;
    LinePlotGPU *freq_graph;
    LinePlotGPU *windowed_freq_graph;

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

        windowed_graph = new LinePlotGPU(this, "windowed");
        windowed_graph->get_values().resize(TimeStretchAudioPlayer::BufferSize);

        freq_graph = new LinePlotGPU(this, "Frequency Graph");
        freq_graph->get_values().resize(TimeStretchAudioPlayer::BufferSize / 2);
        freq_graph->set_min_value(0);

        windowed_freq_graph = new LinePlotGPU(this, "Windowed Frequency");
        windowed_freq_graph->get_values().resize(TimeStretchAudioPlayer::BufferSize / 2);
        windowed_freq_graph->set_min_value(0);

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

        Button *file_button = new Button(this, "Load File");
        file_button->set_callback([this] () {
            // load a new audio file
            char filename[1024] = {'\0'};
            FILE *f = popen(FILEDIALOG_OPEN_COMMAND, "r");
            if (fgets(filename, 1024, f) != nullptr) {
                char *newline_at = strrchr(filename, '\n');
                if (newline_at != nullptr) {
                    *newline_at = '\0'; // remove the newline character at the end
                }

                this->audio_player->load_file(filename);
            }
            pclose(f);

        });

        // Slider *stretch_slider = new Slider(this);
        // stretch_slider->set_range({-1.0f, 1.0f});
        // stretch_slider->set_callback([this] (float value) {
        //     value = Mengu::pow(MaxStretch, value);
        //     audio_player->set_stretch_factor(value);

        // });

        _stretcher_control = new EffectControl(this, "Time Stretcher");
        _stretcher_control->display_property_list(audio_player->time_stretcher->get_property_descs());
        _stretcher_control->set_callback(
            [this] (uint32_t ind, EffectPropPayload data) {audio_player->time_stretcher->set_property(ind, data); }
        );
        
        perform_layout();
        // playing_buttons->set_width(800);


    }

    ~App() {
        if (audio_player != nullptr) {
            delete audio_player;
        }
    }

    virtual void draw_all() override {
        std::vector<Complex> samples = audio_player->sample_buffer.to_vector();
        // for (auto c: new_samples) {
        //     samples.push_back(c);
        // }

        std::vector<float> &base_values = base_graph->get_values();
        for (uint32_t i = 0; i < base_values.size(); i++) {
            base_values[i] = samples[i].real();
        }

        const uint32_t window_size = samples.size() >> 3;
        std::vector<Complex> windowed_samples = samples;
        std::vector<float> &windowed_values = windowed_graph->get_values();

        dsp::window_ends(windowed_samples.data(), windowed_samples.size(), window_size, dsp::hann_window);
        for (uint32_t i = 0; i < windowed_values.size(); i++) {
            windowed_values[i] = windowed_samples[i].real();
        }

        std::vector<Complex> complex_freq(samples.size());
        fft.transform(samples.data(), complex_freq.data());

        std::vector<float> &freq_values = freq_graph->get_values();
        for (uint32_t i = 0; i < freq_values.size() / 2; i++) {
            freq_values[i] = std::norm(complex_freq[i]);
        }

        std::vector<Complex> windowed_freq(windowed_samples.size());
        fft.transform(windowed_samples.data(), windowed_freq.data());
        
        std::vector<float> &windowed_freq_values = windowed_freq_graph->get_values();
        for (uint32_t i = 0; i < windowed_freq_values.size() / 2; i++) {
            windowed_freq_values[i] = std::norm(complex_freq[i]);
        }

        nanogui::Screen::draw_all();
        perform_layout();
    }
};

void test_toeplitz() {
    std::vector<float> toe = {1, 2, 3, 4}; 
    std::vector<float> y = {17, 18, 21, 26};

    std::vector<float> r = Mengu::dsp::solve_sym_toeplitz(toe, y);

    for (float f: r) {
        std::cout << f << ", ";
    }
    std::cout << std::endl;
}

void test_phase_delta() {
    constexpr uint32_t NPhases = 40;
    const float phase_diff = -MATH_PI / 2;
    float phases[NPhases];
    for (uint32_t i = 0; i < NPhases; i++) {
        phases[i] = lerp(-MATH_PI, MATH_PI, (float) i / NPhases);
    }


    for (uint32_t i = 0; i < NPhases; i++) {
        // std::cout << _wrap_phase(phases[i] + phase_diff) << std::endl;
        Complex a = std::polar(1.0f, phases[i]);
        Complex b = std::polar(1.0f, phases[i] + phase_diff);
        std::cout << std::arg(b) << ", " << std::arg(a) << ", " <<  _phase_diff(b, a, 5 * MATH_TAU) << std::endl;
    }

    Complex a = std::polar(1.0f, -1.66101f);
    Complex b = std::polar(1.0f, 1.93086f);
    float d = _phase_diff(b, a, 10 * MATH_TAU);

    std::cout << d << "..." << _wrap_phase(d)<< std::endl;
}

int main() {
    // test_phase_delta();
    // test_toeplitz();
    
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