
#include <nanogui/button.h>
#include <nanogui/screen.h>
#include <nanogui/vscrollpanel.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>

#include "gui/vcombobox.h"
#include "nanogui/layout.h"
#include "nanogui/widget.h"

using namespace Mengu;
using namespace nanogui;

VComboBox::VComboBox(Widget *parent, const std::string &caption): Widget(parent) {
    
    _opening_button = new Button(this, caption);
    _item_container = new Widget(this);

    _opening_button->set_flags(Button::Flags::ToggleButton);
    _opening_button->set_change_callback([this] (bool pressed) {
        if (pressed) {
            open();
        }
        else {
            close();
        }
    });

    _item_container->set_visible(false);

    set_layout(new BoxLayout(
        Orientation::Vertical, 
        Alignment::Fill,
        5
    ));
    _item_container->set_layout(new BoxLayout(
        Orientation::Vertical, 
        Alignment::Fill,
        20,
        4
    ));

    _callback = [] (int) {};
}

void VComboBox::set_caption(const std::string &caption) {
    _opening_button->set_caption(caption);
}

void VComboBox::set_items(const std::vector<std::string> &new_items) {
    int i;
    // Create or rewrite buttons to display the items
    for (i = 0; i < new_items.size(); i++) {
        const std::string &item = new_items[i];
        if (_buttons.size() <= i) {
            Button *button = _item_container->add<Button>(item);
            button->set_callback([this, i] () {
                this->_callback(i);
            });
            _buttons.push_back(button);
            button->set_button_group(_buttons);
        }
        else {
            _buttons[i]->set_caption(item);
        }
    }
    // Hide extra buttons
    for (; i < _buttons.size(); i++) {
        _buttons[i]->set_visible(false);
    }

    _items = new_items;
}

void VComboBox::set_selected_index(int ind) {
    if (ind < 0 || ind >= _buttons.size()) {
        throw std::range_error(
            "Tried select the item at index " + std::to_string(ind) + " which does not exist"
        );
    }
    _selected_index = ind;
}



void VComboBox::open() {
    _opening_button->set_pushed(true);
    _item_container->set_visible(true);
    screen()->perform_layout();
    // _opened_toggled_callback(true);
}

void VComboBox::close() {
    _opening_button->set_pushed(false);
    _item_container->set_visible(false);
    // _opened_toggled_callback(false);
}