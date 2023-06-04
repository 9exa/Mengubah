/**
 * \file gui/lineplot.h
 * \brief draws a line plot given a vector of values
*/
#ifndef LINEPLOT_GPU
#define LINEPLOT_GPU

#include <cstddef>
#include <nanogui/shader.h>
#include <nanogui/renderpass.h>
#include <nanogui/texture.h>
#include <nanogui/canvas.h>
#include <iostream>


namespace Mengu {



using namespace nanogui;

class LinePlotShader: public Shader {
    public:
        LinePlotShader(RenderPass *render_pass, const std::string &name);
        void print_all_buffers();
};

class LinePlotGPU: public Canvas {
    
private:
    float _line_width = 2.0f;
    Color _line_color = Color(100, 255);
    Color _background_color = Color(20, 128);
    std::vector<float> _values;
    std::string _caption;

    LinePlotShader *_shader;
    Texture *_texture;

    uint16_t _shader_plot_size;

    static inline const uint32_t DefaultShaderPlotMax = 256;

    float _min_value = -1.0f;
    float _max_value = 1.0f;


public:

    LinePlotGPU(Widget *parent, const std::string &caption = "Untitled");
    ~LinePlotGPU();

    float get_line_width() const { return _line_width; }
    void setLineWidth(const float &width) {_line_width = width; }

    Color get_line_color() const { return _line_color; }
    void set_line_color(const Color &color) {_line_color = color; }

    void set_min_value(const float &v);
    float get_min_value() const;

    void set_max_value(const float &v);
    float get_max_value() const;

    Color get_background_color() const { return _background_color; }
    void set_background_color(const Color &color) {_background_color = color; }

    std::vector<float> &get_values() { return _values; }
    void set_values(const std::vector<float> &values) { _values = values; }

    virtual Vector2i preferred_size(NVGcontext *) const override;

    virtual void draw_contents() override; 

private:
    // returns the vertical screen position of a value to be drawn onto the lineplot
    float calc_val_y_pos(float val);

};

};
#endif // #define LINEPLOT_GPU