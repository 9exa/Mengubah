#include <array>
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
#include "dsp/correlation.h"
#include "dsp/fft.h"
#include "dsp/interpolation.h"
#include "dsp/linalg.h"
#include "dsp/timestretcher.h"
#include "gui/lineplotgpu.h"
#include "gui/filedialog.h"
#include "mengumath.h"
#include "templates/cyclequeue.h"

using namespace Mengu;



class App: public nanogui::Screen {
public:
    static const uint32_t NSamples = 1 << 12;
    static const uint32_t FramesPerSecond = 120;

    // CycleQueue<Complex> samples; 

    LinePlotGPU *freq_graph;
    LinePlotGPU *envelope_graph;
    LinePlotGPU *shifted_freq_graph;
    LinePlotGPU *shifted_envelope_graph;
    LinePlotGPU *log_freq_graph;
    LinePlotGPU *log_envelope_graph;

    float shift_factor = 1.5;



    TimeStretchAudioPlayer *audio_player;

    dsp::LPC<TimeStretchAudioPlayer::BufferSize, 60> lpc;
    dsp::LPC<TimeStretchAudioPlayer::BufferSize, 60> lpc2;

    App() : nanogui::Screen(nanogui::Vector2i(1280, 720), "Recon test")
            {

        using namespace nanogui;

        // audio_player = nullptr;
        audio_player = new TimeStretchAudioPlayer();

        BoxLayout *fill_layout = new BoxLayout(Orientation::Vertical);
        fill_layout->set_alignment(Alignment::Fill);
        set_layout(fill_layout);

        freq_graph = new LinePlotGPU(this, "Frequency Graph");
        freq_graph->get_values().resize(TimeStretchAudioPlayer::BufferSize / 2);
        freq_graph->set_min_value(0);
        
        envelope_graph = new LinePlotGPU(this, "Envelope Graph");
        envelope_graph->get_values().resize(TimeStretchAudioPlayer::BufferSize / 2);
        envelope_graph->set_min_value(0);

        shifted_freq_graph = new LinePlotGPU(this, "Shifted Frequency Graph");
        shifted_freq_graph->get_values().resize(TimeStretchAudioPlayer::BufferSize / 2);
        shifted_freq_graph->set_min_value(0.0f);

        shifted_envelope_graph = new LinePlotGPU(this, "Shifted Envelope Graph");
        shifted_envelope_graph->get_values().resize(TimeStretchAudioPlayer::BufferSize / 2);
        shifted_envelope_graph->set_min_value(0.0f);


        log_freq_graph = new LinePlotGPU(this, "Log Frequency Graph");
        log_freq_graph->get_values().resize(TimeStretchAudioPlayer::BufferSize / 2);
        log_freq_graph->set_max_value(0);
        log_freq_graph->set_min_value(-40);
        
        log_envelope_graph = new LinePlotGPU(this, "Log Envelope Graph");
        log_envelope_graph->get_values().resize(TimeStretchAudioPlayer::BufferSize / 2);
        log_envelope_graph->set_max_value(0);
        log_envelope_graph->set_min_value(-40);

        Widget *playing_buttons = new Widget(this);
        BoxLayout *button_layout = new BoxLayout(Orientation::Horizontal);
        button_layout->set_alignment(Alignment::Middle);
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
            std::string filename = open_file_dialog({{"wav", "WaveForm"}, {"ogg", "WaveForm"}, {"mp3", "AudioPlayer"}});
            if (!filename.empty()) {
                if (audio_player->load_file(filename)) {
                    std::cout << "file not loaded" << std::endl;
                }
                else {
                    std::cout << filename << " loaded" << std::endl;
                }
            }

        });

        Slider *shift_slider = new Slider(this);
        shift_slider->set_callback([this] (float v) {
            shift_factor = v;
        });
        shift_slider->set_range({0.5, 2.0});

        perform_layout();

    }

    ~App() {
        if (audio_player != nullptr) {
            delete audio_player;
        }
    }
    
    static void shift_by_env(const Complex *input, 
                            Complex *output, 
                            const float *envelope, 
                            const uint32_t size, 
                            const float shift_factor) {
        
        for (uint32_t i = 0; i < size; i++) {
            uint32_t shifted_ind = shift_factor * i;
            if (shifted_ind < size) {
                float correction = envelope[shifted_ind] / envelope[i];
                output[i] = correction * input[i];
            }
            else {
                output[i] = input[size - 1];
            }
        }
    }

    // shift the envelope of a lpc frequency spectrum then 
    static void _reconstruct_freq(const float *residuals, 
                                const float *envelope, 
                                const float *mags,
                                const float *phases,
                                Complex *output, 
                                const uint32_t size, 
                                const float env_shift_factor) {
        float envelope_max = 0.0f;
        float residual_max = 0.0f;
        float mag_max = 0.0f;
        for (uint32_t i = 0; i < size; i++) {
            envelope_max = MAX(envelope_max, envelope[i]);
            residual_max = MAX(residual_max, residuals[i]);
            mag_max = MAX(mag_max, mags[i]);
        }

        for (uint32_t i = 0; i < size; i++) {
            uint32_t shifted_ind = env_shift_factor * i;
            float env_mag = shifted_ind < size ? envelope[shifted_ind] : envelope[i];
            float new_mag = mag_max * residuals[i] / residual_max * env_mag / envelope_max;
            output[i] = std::polar(new_mag, phases[i]);
        }
    }

    virtual void draw_all() override {
        Complex raw_samples[TimeStretchAudioPlayer::BufferSize];
        audio_player->sample_buffer.to_array(raw_samples);
        lpc.load_sample(raw_samples);

        const auto &freqs = lpc.get_freq_spectrum();
        const auto &envelope = lpc.get_envelope();
        const auto &residuals = lpc.get_residuals();

        std::array<float, TimeStretchAudioPlayer::BufferSize / 2> mags;
        std::array<float, TimeStretchAudioPlayer::BufferSize / 2> phases;

        float envelope_max = 0.0f;
        for (uint32_t i = 0; i < TimeStretchAudioPlayer::BufferSize / 2; i++) {
            if (envelope_max < envelope[i]) envelope_max = envelope[i];
            mags[i] = std::sqrt(std::norm(freqs[i]));
            phases[i] = std::arg(freqs[i]);
        }

        Complex shifted_freq[TimeStretchAudioPlayer::BufferSize] = {0};
        Complex shifted_samples[TimeStretchAudioPlayer::BufferSize] = {0};
        // float shifted_envelope[TimeStretchAudioPlayer::BufferSize];
        
        // shift_by_env(freqs.data(), shifted_freq, envelope.data(), TimeStretchAudioPlayer::BufferSize / 2, shift_factor);
        _reconstruct_freq(residuals.data(), envelope.data(), mags.data(), phases.data(), shifted_freq, TimeStretchAudioPlayer::BufferSize / 2, shift_factor);
        lpc.get_fft().inverse_transform(shifted_freq, shifted_samples);

        lpc2.load_sample(shifted_samples);
        auto &shifted_envelope = lpc2.get_envelope();

        for (uint32_t i = 0; i < TimeStretchAudioPlayer::BufferSize / 2; i++) {
            freq_graph->get_values()[i] = residuals[i] * envelope[i];
            envelope_graph->get_values()[i] = envelope[i] / envelope_max;

            shifted_freq_graph->get_values()[i] = std::sqrt(std::norm(shifted_freq[i]));
            shifted_envelope_graph->get_values()[i] = shifted_envelope[i] / envelope_max;

            log_freq_graph->get_values()[i] = 20.*std::log10(std::sqrt(std::norm(freqs[i])));
            log_envelope_graph->get_values()[i] = 20.*std::log10(envelope[i] / envelope_max);
        }

        
        nanogui::Screen::draw_all();
    }
};

void test_toeplitz() {
    std::vector<float> toe;
    std::vector<float> y;
    for (uint32_t i = 0; i < 4; i++) {
        toe.push_back(i * 2 + 17);
        y.push_back(i * i + 17);
    }

    std::vector<float> r = Mengu::dsp::solve_sym_toeplitz(toe, y);

    for (float f: r) {
        std::cout << f << ", ";
    }
    std::cout << std::endl;
}

int main() {
    test_toeplitz();
    
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
    delete app;

    nanogui::shutdown();

    return 0;
}