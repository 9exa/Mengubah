#include "dsp/filter.h"
#include "dsp/common.h"
#include "gui/lineplotgpu.h"
#include "gui/inputslider.h"
#include "mengumath.h"


#include "nanogui/layout.h"
#include "nanogui/slider.h"
#include <complex>
#include <cstdint>
#include <nanogui/screen.h>
#include <string>
#include <format>

using namespace Mengu;
using namespace dsp;

class FilterTest : public nanogui::Screen {
public:
    static constexpr uint32_t FramesPerSecond = 15;
private:
    float _a1 = 0.0f;
    float _a2 = 0.0f;
    float _b0 = 0.0f;
    float _b1 = 0.0f;
    float _b2 = 0.0f;

    LinePlotGPU *_gain_graph;

    static constexpr uint32_t NSamples = 4410;
    
public:

    FilterTest() : nanogui::Screen(nanogui::Vector2i(1280, 720), "Filter test") {
        using namespace nanogui;

        _gain_graph = new LinePlotGPU(this);
        _gain_graph->set_min_value(-5);
        _gain_graph->set_max_value(5);
        _gain_graph->set_fixed_height(400);
        _gain_graph->get_values().resize(NSamples);


        InputSlider *a1_slider = new InputSlider(this);
        a1_slider->set_range({-3.0f, 3.0f});
        a1_slider->set_callback([this] (float value) {
            _a1 = value;
        });

        InputSlider *a2_slider = new InputSlider(this);
        a2_slider->set_range({-3.0f, 3.0f});
        a2_slider->set_callback([this] (float value) {
            _a2 = value;
        });

        InputSlider *b0_slider = new InputSlider(this);
        b0_slider->set_range({-3.0f, 3.0f});
        b0_slider->set_callback([this] (float value) {
            _b0 = value;
        });

        InputSlider *b1_slider = new InputSlider(this);
        b1_slider->set_range({-3.0f, 3.0f});
        b1_slider->set_callback([this] (float value) {
            _b1 = value;
        });

        InputSlider *b2_slider = new InputSlider(this);
        b2_slider->set_range({-3.0f, 3.0f});
        b2_slider->set_callback([this] (float value) {
            _b2 = value;
        });

        set_layout(new BoxLayout(Orientation::Vertical, Alignment::Fill));
        perform_layout();
    }

    virtual void draw_all() override {
        // just redraw the graph
        for (uint32_t i = 0; i < NSamples; i++) {
            float omega = (float) MATH_PI * i / NSamples;
            Complex z = std::polar(1.0f, omega);
            
            Complex h = quad_filter_trans(z, _a1, _a2, _b0, _b1, _b2);
            float gain = 10.0f * log10(std::norm(h));
            _gain_graph->get_values()[i] = gain;
        }

        Screen::draw_all();
    }
};

int main() {
    nanogui::init();

    FilterTest *app = new FilterTest();
    app->set_visible(true);
    app->draw_all();

    try {
        nanogui::mainloop(1.0 / FilterTest::FramesPerSecond * 1'000);

    }
    catch(std::exception e) {
        std::cout << e.what() << std::endl;
    }
    nanogui::shutdown();

    delete app;
    return 0;
}