#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <iostream>
#include <vector>

#include <nanogui/common.h>
#include <nanogui/vector.h>
#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/slider.h>
#include <nanogui/layout.h>

#include "dsp/common.h"
#include "dsp/fastmath.h"
#include "dsp/interpolation.h"
#include "dsp/pitchshifter.h"
#include "gui/lineplotgpu.h"
#include "mengumath.h"
#include "templates/cyclequeue.h"
#include "templates/vecdeque.h"

using namespace Mengu;

class App: public nanogui::Screen {
    const uint32_t NSamples = 1600;

    LinePlotGPU *base_graph;
    LinePlotGPU *reconstructed_graph;
    LinePlotGPU *expected_graph;

    LinePlotGPU *raw_buffer_graph;

    std::vector<Complex> base_values;
    CycleQueue<Complex> reconstructed_values;
    std::vector<Complex> expected_values;

    dsp::PhaseVocoderPitchShifter pvpitchshifter;

    // Proportion of the base graph that gets processed per second
    float scroll_speed = 0.5;


    uint32_t frames_passed = 0;

    void _recalculate_values() {
        for (uint32_t i = 0; i < NSamples; i++) {
            float x = (float) (i + frames_passed) / NSamples;
            float x2 = (float) (2 * i + frames_passed) / NSamples;
            base_values[i] = std::sin(x * MATH_TAU);
            expected_values[i] = std::sin(x2 * MATH_TAU);
        }
    }

public:

    // Expected number of draw_all() calls per second
    static const uint32_t FramesPerSecond = 120;

    App() : nanogui::Screen(nanogui::Vector2i(800, 600), "Recon test"),
            pvpitchshifter(1 << 8) {
        BoxLayout *fill_layout = new BoxLayout(Orientation::Vertical);
        fill_layout->set_alignment(Alignment::Fill);
        set_layout(fill_layout);

        base_graph = new LinePlotGPU(this, "Base");
        base_graph->set_min_value(-1.0f);
        base_graph->set_max_value(1.0f);
        base_graph->get_values().resize(NSamples);

        reconstructed_graph = new LinePlotGPU(this, "Reconstructed");
        reconstructed_graph->set_min_value(-1.0f);
        reconstructed_graph->set_max_value(1.0f);
        reconstructed_graph->get_values().resize(NSamples);

        expected_graph = new LinePlotGPU(this, "Expected");
        expected_graph->set_min_value(-1.0f);
        expected_graph->set_max_value(1.0f);
        expected_graph->get_values().resize(NSamples);

        base_values.resize(NSamples);
        reconstructed_values.resize(NSamples);
        expected_values.resize(NSamples);

        raw_buffer_graph = new LinePlotGPU(this, "buffer");

        perform_layout();

        // Test of one time pitch shift
        std::vector<Complex> one_time_based;
        std::vector<Complex> one_time_shifted;
        const uint32_t NOneTime = 1 << 12;
        for (int i = 0; i < NOneTime; i++) {
            float x = (float) i / NOneTime;
            one_time_based.push_back(std::sin(x * MATH_TAU));
        }
        dsp::PhaseVocoderPitchShifter pp = dsp::PhaseVocoderPitchShifter(NSamples);

    }

    virtual void draw_all() override {
        const uint32_t frames_this_pass = NSamples * scroll_speed / FramesPerSecond;
        frames_passed += frames_this_pass;

        _recalculate_values();

        pvpitchshifter.push_signal(base_values.data(), frames_this_pass);
        
        Complex *recon_this_frame = new Complex[frames_this_pass];

        pvpitchshifter.pop_transformed_signal(recon_this_frame, frames_this_pass);
        
        for (uint32_t i = 0; i < frames_this_pass; i++) {
            reconstructed_values.push_back(recon_this_frame[i]);
        }
        

        for (uint32_t i = 0; i < NSamples; i++) {
            base_graph->get_values()[i] = base_values[i].real();
        
            reconstructed_graph->get_values()[i] = reconstructed_values[i].real();

            expected_graph->get_values()[i] = expected_values[i].real();
        }

        auto raw_buffer = pvpitchshifter.get_raw_buffer();
        raw_buffer_graph->get_values().resize(raw_buffer.size());
        for (uint32_t i = 0; i < raw_buffer.size(); i++ ) {
            (raw_buffer_graph->get_values()[i] = raw_buffer[i].real());

        }

        

        nanogui::Screen::draw_all(); 

        delete [] recon_this_frame;
    }

};

// Series of graphs that showcase the reconstruction of a sin signal using the PhaseVocoderPitchShifter object

int main() {
    nanogui::init();

    int j = 0;

    // Vecdeque test
    VecDeque<Complex> v;
    v.resize(30);
    for (; j < 20; j++) {
        v.push_back(j);
    }
    

    std::vector<Complex> v2(20);

    v.to_array(v2.data(), 20);

    v.pop_front_many(nullptr, 10);

    for (; j < 30; j++) {
        v.push_back(j);
    }

    v.to_array(v2.data(), 20);
    v.pop_front_many(nullptr, 10);

    std::cout << v.size() << std::endl;

    for (; j < 50; j++) {
        v.push_back(j);
    }

    v.pop_front_many(v2.data(), 20);

    for (int i = 0; i < 20; i++) {
        std::cout << v2[i] << std::endl;
    }

    std::cout << v.size() << std::endl;

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