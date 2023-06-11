/**
 * @file effectcontrol.h
 * @author 9exa
 * @date 2023-06-01
 *
 * @brief Widget that lists properties of an Effect along with the approprate 
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "dsp/effect.h"
#include "gui/effectcontrol.h"
#include "gui/inputslider.h"
#include "nanogui/button.h"
#include "nanogui/common.h"
#include "nanogui/label.h"
#include "nanogui/layout.h"
#include "nanogui/slider.h"
#include "nanogui/textarea.h"
#include "nanogui/vector.h"

#include <cstdint>
#include <functional>
#include <nanogui/widget.h>
#include <string>

using namespace Mengu;
using namespace nanogui;

EffectControl::EffectControl(Widget *parent, const std::string &effect_name) : 
    Widget(parent) {
    Layout *layout = new GridLayout(
        Orientation::Horizontal,
        1, 
        Alignment::Fill, 
        10
    );
    set_layout(layout);

    Widget *header = new Widget(this);
    _effect_name_label = new Label(header, effect_name);
    _delete_button = new Button(header, "delete");

    using Anchor = AdvancedGridLayout::Anchor;
    AdvancedGridLayout *header_layout = new AdvancedGridLayout(
        {300, 0},
        {40},
        4
    );
    header->set_layout(header_layout);
    header_layout->set_anchor(_effect_name_label, Anchor(
        0, 0,
        Alignment::Middle
    ));
    header_layout->set_anchor(_delete_button, Anchor(
        1, 0,
        Alignment::Maximum
    ));
    header_layout->set_col_stretch(1, 1);
    _delete_button->set_visible(false);

    // _clear_properties();    
}

void EffectControl::set_callback(const std::function<void (uint32_t id, EffectPropPayload data)> &callback) {
    _callback = callback;
}
const std::function<void (uint32_t id, EffectPropPayload data)> &EffectControl::get_callback() {
    return _callback;
}

void EffectControl::set_effect_name(const std::string &effect_name) { 
    _effect_name_label->set_caption(effect_name);
}

const std::string &EffectControl::get_effect_name() const { 
    return _effect_name_label->caption(); 
}

void EffectControl::set_delete_button_enabled(bool enabled) {
    _delete_button->set_visible(enabled);
}

void EffectControl::set_delete_button_callback(const std::function<void ()> &callback) {
    _delete_button->set_callback(callback);
}


void EffectControl::display_property_list(const std::vector<EffectPropDesc> &prop_descs) {
    _clear_properties();

    for (uint32_t i = 0; i < prop_descs.size(); i++) {
        _add_property(i, prop_descs[i]);

    }
}

Widget *EffectControl::_create_prop_widget(Widget *parent, uint32_t ind, const EffectPropDesc &prop_desc) const {
    switch (prop_desc.type) {
        case Toggle: {
            Button *effect_button = new Button(parent, "On");
            auto callback = [this, ind] (bool pressed) {
                EffectPropPayload data { .type = Toggle, .on = pressed };
                this->_callback(ind, data);
            };
            effect_button->set_change_callback(callback);
            effect_button->set_flags(Button::ToggleButton);
            return (Widget*) effect_button;
        }
        case Slider:
        case Knob:
        case Counter: {
            // nanogui::Slider *effect_slider = new nanogui::Slider(parent);
            InputSlider *effect_slider = new InputSlider(parent);
            std::function<void (float)> callback;
            switch (prop_desc.slider_data.scale) {
                case Linear:
                    effect_slider->set_range({
                        prop_desc.slider_data.min_value, 
                        prop_desc.slider_data.max_value
                    });
                    callback = [this, ind] (float value) {
                        EffectPropPayload data { .type = Slider, .value = value };
                        this->_callback(ind, data);
                    };
                    break;
                case Exp:
                    // Slider is an exponential scale with the base of max_value
                    float base = ABS(prop_desc.slider_data.max_value);
                    effect_slider->set_range({ -1.0f, 1.0f });
                    callback = [this, ind, base] (float value) {
                        EffectPropPayload data { .type = Slider, .value = pow(base, value) };
                        this->_callback(ind, data);
                    };
                    break;
            }
            effect_slider->set_callback(callback);
            return (Widget*) effect_slider;
        }
    }
}

void EffectControl::_add_property(uint32_t ind, const EffectPropDesc &prop_desc) {    
    Label *prop_label = new Label(_property_widgets_container, prop_desc.name);
    prop_label->set_color(Color(0.2f, 1.0f, 0.1f, 1.0f));
    
    // Load the actual editable widget
    Widget *prop_widget = _create_prop_widget(_property_widgets_container, ind, prop_desc);

    // put them in the grid
    AdvancedGridLayout *container_layout = (AdvancedGridLayout*) _property_widgets_container->layout();
    int row_num = _property_widgets_container->child_count() / 2 - 1;
    if (row_num >= container_layout->row_count() || true) {
        container_layout->append_row(50);
    }
    using Anchor = AdvancedGridLayout::Anchor;
    container_layout->set_anchor(prop_label, Anchor(0, row_num, Alignment::Middle));
    container_layout->set_anchor(prop_widget, Anchor(1, row_num));

}

void EffectControl::_clear_properties() {
    if (_property_widgets_container != nullptr) {
        remove_child(_property_widgets_container);
    }
    _property_widgets_container = new Widget(this);
    AdvancedGridLayout *container_layout = new AdvancedGridLayout( {100, 300}, {}, 5);
    container_layout->set_col_stretch(1, 1.0f);
    _property_widgets_container->set_layout(container_layout);
    
}