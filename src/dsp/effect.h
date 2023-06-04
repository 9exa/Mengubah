#ifndef MENGA_EFFECT
#define MENGA_EFFECT

#include <cstdint>
#include <dsp/common.h>
#include <templates/cyclequeue.h>
#include <vector>

namespace Mengu {
namespace dsp {

// Types of Properties that an Effect has and how they can be edited by gui
// If this was Rust (a better language) this would all be one enum
enum EffectPropType {
    Toggle, // Edited With a ToggleButton
    Slider, // Edited with A Slider. A min and max value must be declared. Stores real number
    Knob, // Edited with A Knob. A min and max value must be declared. Stores real number
    Counter, // Edited with a counter. A step size must be declared. Stores real numbers, but often casted into an int
};

enum EffectPropContScale {
    Linear,
    Exp,
};

// Description for the editable properties of an Effect. 
struct EffectPropDesc {
    EffectPropType type;
    const char *name;
    const char *desc;
    union {
        struct {
            float min_value;
            float max_value;
            float step_size;
            EffectPropContScale scale;
        } slider_data; // use by slider, knob and counter
    };
};

// Data used to get and set EffectProperty data
struct EffectPropPayload {
    EffectPropType type;
    union {
        bool on; // Used by Toggle
        float value; // used by slider, knob, and counter
    };
};


// 

// object that takes in the next value signal (time or frequency domain)
// and can be queried for the next value in the process signal.
class Effect {
public:
    virtual ~Effect() = default;
    // tells an EffectChain what type of input the effect expects
    enum InputDomain {
        Time = 0,
        Spectral = 1,
        Frequency = 1
    };    
    virtual InputDomain get_input_domain() = 0;

    // push new value of signal
    virtual void push_signal(const Complex *input, const uint32_t &size) = 0;
    // Last value of transformed signal
    virtual uint32_t pop_transformed_signal(Complex *output, const uint32_t &size) = 0;
    // number of samples that can be output given the current pushed signals of the Effect
    virtual uint32_t n_transformed_ready() const = 0;
    // resets state of effect to make it reading to take in a new sample
    virtual void reset() = 0;
    // The properties that this Effect exposes to be changed by GUI. 
    // The index that they are put in is considered the props id
    virtual std::vector<EffectPropDesc> get_property_descs() const = 0;
    // Sets a property with the specified id the value declared in the payload
    virtual void set_property(uint32_t id, EffectPropPayload data) = 0;
    // Gets the value of a property with the specified id
    virtual EffectPropPayload get_property(uint32_t id) const = 0;
};

// Represents a series of effects chained consequtivly. Processed on demand
class EffectChain {
public:
    EffectChain(uint32_t buffer_size);
    ~EffectChain();

    // push a new signal. Unlike an Effect the pushed signal can be an arbitrary size as it is stored in a ringbuffer
    void push_signal(const Complex *input, const uint32_t &size);

    // Last values of transformed signal
    void pop_transformed_signal(Complex *output, const uint32_t &size);

    // add an Effect
    void append_effect(Effect *effect);

    // apply all effects
    void process();

private:
    uint32_t _buffer_size;
    CycleQueue<Complex> _input_buffer;
    std::vector<Complex> _transformed_buffer;
    std::vector<Effect *> _effects;
};

}
}



#endif