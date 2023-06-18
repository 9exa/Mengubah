/**
 * @file inputslider.h
 * @author 9exa (9exa@github.com)
 * @brief A slider control with text input
 * @version 0.1
 * @date 2023-06-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef MENGU_INPUT_SLIDER
#define MENGU_INPUT_SLIDER
#include <functional>
#include <nanogui/slider.h>
#include <nanogui/textbox.h>
#include <nanogui/widget.h>

using namespace nanogui;

namespace Mengu {

class InputSlider: public Widget {
public:
    InputSlider(Widget *parent, bool use_exp = false);

    void set_value(float value);
    float get_value() const { return _value; }

    void set_callback(const std::function<void (float)> &callback);
    const std::function<void (float)> &get_callback() const { return _callback; }

    void set_range(std::pair<float, float> range);

    void set_use_exp(bool use_exp);
    void set_base(float base);



protected:
    nanogui::Slider *_slider; // need to specify nanogui:: here because Slider is an identifier used in Effect.h, which may be #included with this file
    FloatBox<float> *_input_box;

    std::function<void (float)> _callback;

    float _value;

    float _min_value = -1.0f;
    float _max_value = 1.0f;

    bool _use_exp;
    float _base = 2.0f;
};

}


#endif