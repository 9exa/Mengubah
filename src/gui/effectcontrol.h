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


#ifndef MENGA_EFFECT_CONTROL
#define MENGA_EFFECT_CONTROL
#include "dsp/effect.h"
#include "nanogui/button.h"
#include "nanogui/label.h"

#include <cstdint>
#include <functional>
#include <nanogui/widget.h>
#include <string>

namespace Mengu {

using namespace nanogui;
using namespace dsp;

class EffectControl : public Widget {
public:
    EffectControl(Widget *parent, const std::string &effect_name = "");

    void set_callback(const std::function<void (uint32_t id, EffectPropPayload data)> &callback);
    const std::function<void (uint32_t id, EffectPropPayload data)> &get_callback();

    void set_effect_name(const std::string &effect_name);
    const std::string &get_effect_name() const;

    // sets whether or not to enable the 'delete' button to delete this effect
    void set_delete_button_enabled(bool enabled);

    void set_delete_button_callback(const std::function<void ()> &callback);

    // Makes the control and track the effect;
    void display_property_list(const std::vector<EffectPropDesc> &prop_descs);

private:


    // Property Widget creation function
    Widget *_create_prop_widget(Widget *parent, uint32_t ind, const EffectPropDesc &prop_desc) const; // should probably be const, but connects to a non-const callback

    void _add_property(uint32_t ind, const EffectPropDesc &prop_desc);
    void _clear_properties();

    // The Widget displaying the name of the effect
    Label *_effect_name_label;

    // Holds each properties associated widget as its children;
    Widget *_property_widgets_container = nullptr;

    // gets called when widgets are edited
    std::function<void (uint32_t id, EffectPropPayload data)> _callback;


    // Whether or not this effect can be deleted via the 'delete' button
    Button *_delete_button;

    // function that plays when the delete button is pressed
    std::function<void ()> _delete_button_callback;
};

};

#endif