#include "gui/lineplotgpu.h"
#include "mengumath.h"
#include <cstdint>
#include <nanogui/opengl.h>
#include <vector>

using namespace Mengu;
using namespace nanogui;



LinePlotGPU::LinePlotGPU(Widget *parent, const std::string &caption) 
    : Canvas(parent), _caption(caption) {
        
    render_pass()->set_clear_color(0, Color(1.0f, 0.0f, 0.032f, 1.f));

    _shader = new LinePlotShader(render_pass(), "LinePlot");

    // draw it on a square

    uint32_t indices[3*2] = {
        0, 1, 2,
        2, 3, 0
    };

    float positions[3*4] = {
        -1.f, -1.f, 0.f,
        1.f, -1.f, 0.f,
        1.f, 1.f, 0.f,
        -1.f, 1.f, 0.f
    };

    // float texcoord[2*4] {
    //     -1.f, -1.f,
    //     1.f, -1.f,
    //     1.f, 1.f,
    //     -1.f, 1.f
    // };
    m_draw_border = true;
    m_border_color = Color(0.f,1.f,0.f,1.0);

    _values.resize(256);
    _texture = new Texture(
        Texture::PixelFormat::R,
        Texture::ComponentFormat::Float32,
        Vector2i(DefaultShaderPlotMax, 1)
    );
    _shader_plot_size = DefaultShaderPlotMax;

    _shader->set_buffer("indices", VariableType::UInt32, {3*2}, indices);
    _shader->set_buffer("position", VariableType::Float32, {4, 3}, positions);
    _shader->set_texture("data", _texture);

    _shader->set_uniform("min_val", _min_value);
    _shader->set_uniform("max_val", _max_value);


    // _shader->set_buffer("texcoord", VariableType::Float32, {4, 2}, texcoord);


}

LinePlotGPU::~LinePlotGPU() {
    delete _shader;
    delete _texture;
}

void LinePlotGPU::set_min_value(const float &v) {
    _min_value = v;
    _shader->set_uniform("min_val", v);
}

float LinePlotGPU::get_min_value() const { return _min_value; }


void LinePlotGPU::set_max_value(const float &v) {
    _max_value = v;
    _shader->set_uniform("max_val", v);
}

float LinePlotGPU::get_max_value() const { return _max_value; }


float LinePlotGPU::calc_val_y_pos(float val) {
    return m_pos.y() + m_size.y() * (1.0f - inverse_lerp(_min_value, _max_value, val));
}

Vector2i LinePlotGPU::preferred_size(NVGcontext *) const {
    return Vector2i(180, 90);
}

void LinePlotGPU::draw_contents() {
    Vector2f scale = Vector2f(0.5f * m_size.x(), 0.5f * m_size.y());
    _shader->set_uniform("scale", scale);


    //update plot
    if (_shader_plot_size != _values.size()) {
        _shader_plot_size = _values.size();
        _texture->resize(Vector2i(_shader_plot_size, 1));
    }
    
    _texture->upload((uint8_t *)_values.data());



    _shader->begin();
    _shader->draw_array(Shader::PrimitiveType::Triangle, 0, 6, true);
    _shader->end();

    
}

static std::string make_vertex_shader() {
    return (std::string)(R"(
            #version 330
            in vec3 position;

            out vec2 uv;

            uniform vec2 scale;

            vec2 pos;
            void main() {
                pos = vec2(position.x * scale.x, position.y * scale.y);
                gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);
                
                // I'm not sure why but but pos corresponds to (-1, 1)
                uv = gl_Position.xy;

                uv = uv * 0.5 + 0.5;
            })");
}

static std::string make_fragment_shader() {
    static const char* FragmentTemplate =
            R"(
            #version 330
            in vec2 uv;

            out vec4 color;
            
            uniform sampler2D data;
            uniform float max_val;
            uniform float min_val;

            vec4 DEFCOL = vec4(0.03, 0.04, 0.06, 1.0f);
            vec4 UNDERCOL = vec4(1.0, 0.0, 0.8, 1.0f);

            float threshold;

            // cloaser to one if  y is close to 
            float plot(float y, float point, float width){
                return  smoothstep( point-width, point, y) -
                        smoothstep( point, point+width, y);
            }

            void main() {
                float y_val = mix(min_val, max_val, uv.y);

                threshold = texture(data, vec2(uv.x, 0.5)).r;

                color = mix(DEFCOL, UNDERCOL, plot(y_val, threshold, 0.1));
                
            })";
    std::string fragment_shader = FragmentTemplate;

    return fragment_shader;

}

LinePlotShader::LinePlotShader(RenderPass *render_pass, const std::string &name) 
    : Shader(render_pass, name, make_vertex_shader(), make_fragment_shader()) {
    // make compatible with arrays
}
