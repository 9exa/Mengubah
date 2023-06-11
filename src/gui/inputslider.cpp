#include "inputslider.h"
#include "mengumath.h"
#include "nanogui/layout.h"
#include "nanogui/slider.h"
#include "nanogui/textbox.h"
#include "nanogui/widget.h"

using namespace Mengu;

InputSlider::InputSlider(Widget *parent): Widget(parent) {
    _input_box = new FloatBox<float>(this);
    _input_box->set_callback([this] (float value) {
        value = CLAMP(value, _min_value, _max_value);
        _value = value;
        _slider->set_value(value);
        if (_callback != nullptr) {
            _callback(value);
        }
    });
    _input_box->set_editable(true);
    _input_box->number_format("%.2f");

    _slider = new Slider(this);
    _slider->set_callback([this] (float value) {
        value = CLAMP(value, _min_value, _max_value);
        _value = value;
        _input_box->set_value(value);
        if (_callback != nullptr) {
            _callback(value);
        }
    });

    using Anchor = AdvancedGridLayout::Anchor;
    AdvancedGridLayout *layout = new AdvancedGridLayout({80, 50}, {40});
    layout->set_col_stretch(1, 1.0f);
    layout->set_anchor(_input_box, Anchor(0, 0));
    layout->set_anchor(_slider, Anchor(1, 0));
    set_layout(layout);
}

void InputSlider::set_value(float value) {
    _value = value;
    _input_box->set_value(value);
    _slider->set_value(value);
}

void InputSlider::set_callback(const std::function<void (float)> &callback) {
    _callback = callback;
}

void InputSlider::set_range(std::pair<float, float> range) {
    _min_value = range.first;
    _max_value = range.second;
    _slider->set_range(range);
    _input_box->set_min_max_values(range.first, range.second);
}
