/**
 * \file gui/lineplot.h
 * \brief draws a line plot given a vector of values
*/
#ifndef LINEPLOT
#define LINEPLOT

#include <nanogui/widget.h>


namespace Mengu {

using namespace nanogui;

class LinePlot: public Widget {
    
private:
    float _line_width = 2.0f;
    Color _line_color = Color(100, 255);
    Color _background_color = Color(20, 128);
    std::vector<float> _values;
    std::string _caption;

    // returns the vertical screen position of a value to be drawn onto the lineplot
    float calc_val_y_pos(float val);

    float _min_value = 0.0f;
    float _max_value = 1.0f;


public:

    LinePlot(Widget *parent, const std::string &caption = "Untitled");

    void set_min_value(const float &v) { _min_value = v;};
    float get_min_value() const { return _min_value; };

    void set_max_value(const float &v) { _max_value = v;};
    float get_max_value() const { return _max_value; };

    float get_line_width() const { return _line_width; }
    void setLineWidth(const float &width) {_line_width = width; }

    Color get_line_color() const { return _line_color; }
    void set_line_color(const Color &color) {_line_color = color; }

    Color get_background_color() const { return _background_color; }
    void set_background_color(const Color &color) {_background_color = color; }

    std::vector<float> &get_values() { return _values; }
    void set_values(const std::vector<float> &values) { _values = values; }

    virtual Vector2i preferred_size(NVGcontext *) const override;

    virtual void draw(NVGcontext *ctx) override; 

    
};

};
#endif