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
#include "mengumath.h"
#include "templates/cyclequeue.h"


#define DIRPATH "/home/wes/Music"
#define FILEPATH "/home/wes/Music/Treasured Creation - SSN Ghost Peppered OST.ogg"
#define FILEDIALOG_OPEN_COMMAND "/usr/bin/zenity --file-selection --file-filter=\'Music files (ogg,wav,mp3) | *.ogg *.wav *.mp3\'"

using namespace Mengu;



class App: public nanogui::Screen {
public:
    static const uint32_t NSamples = 1 << 12;
    static const uint32_t FramesPerSecond = 120;

    // CycleQueue<Complex> samples; 

    LinePlotGPU *freq_graph;
    LinePlotGPU *envelope_graph;
    LinePlotGPU *log_freq_graph;
    LinePlotGPU *log_envelope_graph;

    TimeStretchAudioPlayer *audio_player;

    dsp::LPC<TimeStretchAudioPlayer::BufferSize, 60> lpc;

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

        perform_layout();

    }

    ~App() {
        if (audio_player != nullptr) {
            delete audio_player;
        }
    }

    virtual void draw_all() override {
        Complex raw_samples[TimeStretchAudioPlayer::BufferSize];
        audio_player->sample_buffer.to_array(raw_samples);
        lpc.load_sample(raw_samples);

        const auto &freqs = lpc.get_freq_spectrum();
        const auto &envelope = lpc.get_envelope();
        
        float envelope_max = 0.0f;
        for (uint32_t i = 0; i < TimeStretchAudioPlayer::BufferSize / 2; i++) {
            if (envelope_max < envelope[i]) envelope_max = envelope[i];
        }
        
        for (uint32_t i = 0; i < TimeStretchAudioPlayer::BufferSize / 2; i++) {
            freq_graph->get_values()[i] = std::sqrt(std::norm(freqs[i]));
            envelope_graph->get_values()[i] = envelope[i] / envelope_max;
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
    nanogui::shutdown();

    delete app;
    return 0;
}