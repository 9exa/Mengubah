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
    InputSlider(Widget *parent);

    void set_value(float value);
    float get_value() const { return _value; }

    void set_callback(const std::function<void (float)> &callback);
    const std::function<void (float)> &get_callback() const { return _callback; }

    void set_range(std::pair<float, float> range);



protected:
    Slider *_slider;
    FloatBox<float> *_input_box;

    std::function<void (float)> _callback;

    float _value;

    float _min_value = -1.0f;
    float _max_value = 1.0f;
};

}


#endif