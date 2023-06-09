#ifndef FTAPP
#define FTAPP
#include <cmath>
#include <vector>

#include <nanogui/vector.h>
#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/slider.h>
#include <nanogui/layout.h>

#include "gui/lineplot.h"
#include "gui/lineplotgpu.h"
#include "dsp/fft.h"
#include "dsp/pitchshifter.h"
#include "nanogui/widget.h"
#include "templates/cyclequeue.h"
#include "audioplayers/Audioplayer.h"


class MenguPitchy: public nanogui::Screen {
public:
    // Number of points to be displayed
    static inline const uint32_t NSamples = 2 << 10;
    static inline const uint32_t SmolSamples = 2 << 10;
    static const unsigned short FramesPerSecond = 60;

    // How many radians the input value of the displayed function incriments per second
    static constexpr float DataSpeed = 0.5f;

    MenguPitchy();
    ~MenguPitchy();

    // basically the update() / process() function
    virtual void draw_all() override;


private:
    static float _display_func(float x, float freq, float offset = 0.0f);
    void _draw_plots();

    const float MaxFreq = NSamples >> 2;
    const float MinFreq = 1.0;

    Mengu::LinePlotGPU *_sample_graph;
    Mengu::LinePlotGPU *_freq_graph;
    Mengu::dsp::FFT _fft;

    // Used as a proxy for the whole main screen, so we can also have Child Windows moving around for item selection
    nanogui::Widget *_root;

    Mengu::AudioPlayer _audio_player;
};

#endif