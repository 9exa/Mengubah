#include "inputslider.h"
#include "mengumath.h"
#include "nanogui/layout.h"
#include "nanogui/slider.h"
#include "nanogui/tabwidget.h"
#include "nanogui/textbox.h"
#include "nanogui/widget.h"
#include <iostream>
#include <math.h>

using namespace Mengu;

InputSlider::InputSlider(Widget *parent, bool use_exp): Widget(parent) {
    _input_box = new FloatBox<float>(this);
    _input_box->set_callback([this] (float value) {
        float max_value = _use_exp ? pow(_base, _max_value) : _max_value;
        float min_value = _use_exp ? pow(_base, _min_value) : _min_value;
        value = CLAMP(value, min_value, max_value);
        _value = value;
        if (_use_exp) {
            _slider->set_value(logb(_base, value));
        }
        else {
            _slider->set_value(value);
        }
        if (_callback != nullptr) {
            _callback(value);
        }
    });
    _input_box->set_editable(true);
    _input_box->number_format("%.2f");

    _slider = new Slider(this);
    _slider->set_callback([this] (float value) {
        value = CLAMP(value, _min_value, _max_value);

        _use_exp ? _value = pow(_base, value) : _value = value;
        _input_box->set_value(_value);
        if (_callback != nullptr) {
            _callback(_value);
        }
    });

    using Anchor = AdvancedGridLayout::Anchor;
    AdvancedGridLayout *layout = new AdvancedGridLayout({80, 50}, {40});
    layout->set_col_stretch(1, 1.0f);
    layout->set_anchor(_input_box, Anchor(0, 0));
    layout->set_anchor(_slider, Anchor(1, 0));
    set_layout(layout);

    _use_exp = use_exp;
}

void InputSlider::set_value(float value) {
    _value = value;
    _input_box->set_value(value);

    if (_use_exp) { _slider->set_value(logb(_base, value)); }
    else { _slider->set_value(value); }
}

void InputSlider::set_callback(const std::function<void (float)> &callback) {
    _callback = callback;
}

void InputSlider::set_range(std::pair<float, float> range) {
    _min_value = range.first;
    _max_value = range.second;
    _slider->set_range(range);
    if (_use_exp) {
        _input_box->set_min_max_values(pow(_base, range.first), pow(_base, range.second));
    }
    else {
        _input_box->set_min_max_values(range.first, range.second);
    }
}

void InputSlider::set_use_exp(bool use_exp) {
    _use_exp = use_exp;
    if (_use_exp) {
        _input_box->set_min_max_values(pow(_base, _min_value), pow(_base, _max_value));
    }
    else {
        _input_box->set_min_max_values(_min_value, _max_value);
    }
}

void InputSlider::set_base(float base) {
    _base = base;
    if (_use_exp) {
        _input_box->set_min_max_values(pow(_base, _min_value), pow(_base, _max_value));
    }
}
