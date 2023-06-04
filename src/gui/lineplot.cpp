#include "gui/lineplot.h"
#include "mengumath.h"
#include <nanogui/opengl.h>

using namespace Mengu;
using namespace nanogui;

LinePlot::LinePlot(Widget *parent, const std::string &caption) 
    : Widget(parent), _caption(caption) {}


float LinePlot::calc_val_y_pos(float val) {
    return m_pos.y() + m_size.y() * (1.0f - inverse_lerp(_min_value, _max_value, val));
}

Vector2i LinePlot::preferred_size(NVGcontext *) const {
    return Vector2i(180, 90);
}

void LinePlot::draw(NVGcontext *ctx) {
    Widget::draw(ctx);

    // background
    nvgBeginPath(ctx);
    nvgRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y());
    nvgFillColor(ctx, _background_color);
    nvgFill(ctx);

    if (_values.size() < 2) {
        return;
    }

    //lines
    nvgBeginPath(ctx);
    nvgMoveTo(ctx, m_pos.x(), calc_val_y_pos(_values[0]));
    for (size_t i = 1; i < _values.size(); i++) {
        const float vx = m_pos.x() + i * m_size.x() / (float) (_values.size() - 1);
        const float vy = calc_val_y_pos(_values[i]);
        nvgLineTo(ctx, vx, vy);
    }
    nvgStrokeColor(ctx, _line_color);
    nvgStrokeWidth(ctx, _line_width);
    nvgStroke(ctx);

    //bounds
    nvgBeginPath(ctx);
    nvgRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y());
    nvgStrokeColor(ctx, Color(100, 255));
    nvgStroke(ctx);

}