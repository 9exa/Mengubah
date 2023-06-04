/**
 * @file vcombobox.h
 * @brief ComboBox that expands vertically, as opposed to a popup on the side, which is what
 * the default nanogui Combobox
 */
#ifndef MENGA_VCOMBOBOX
#define MENGA_VCOMBOBOX

#include "nanogui/widget.h"
#include <functional>
#include <nanogui/button.h>
#include <nanogui/vscrollpanel.h>
#include <string>
#include <vector>

namespace Mengu {
using namespace nanogui;

class VComboBox: public Widget {
public:
    VComboBox(Widget *parent, const std::string &caption);

    void set_caption(const std::string &caption);
    const std::string &get_caption() const { return _opening_button->caption(); };

    void set_items(const std::vector<std::string> &new_items);
    const std::vector<std::string> &get_items() const { return _items; }

    void set_selected_index(int ind);
    int get_selected_index() const { return _selected_index; };

    void set_callback(const std::function<void(int)> &callback) { _callback = callback; }

    // set the function that is called when this combobox is open/closed
    void set_opened_toggled_callback(const std::function<void(bool)> &callback) { _opened_toggled_callback = callback; };

    void open();
    void close();

    

private:
    // scroll panel 
    VScrollPanel *_vscrollpanel;

    //Button that opens the combobox
    Button *_opening_button;
    // what the button that opens the combobox says
    // std::string _caption;

    // Things that the user chooses
    std::vector<std::string> _items;
    
    //The buttons that the user clicks on the select an item
    std::vector<Button *> _buttons;

    Widget *_item_container;

    // The index of the item that is selected (-1 if none are selected)
    int _selected_index = -1;

    // function that is called when an item is selected
    std::function<void(int)> _callback;

    // function that runs when VComboBox is opened/closed
    std::function<void(bool)> _opened_toggled_callback;

};

}

#endif