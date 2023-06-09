/**
 * @file formantshifter.h
 * @author 9exa
 * @brief Shifts the formants of a signal in real time.
 * @date 2023-06-08 
 */

#ifndef MENGU_FORMANT_SHIFTER
#define MENGU_FORMANT_SHIFTER

#include "dsp/common.h"
#include "dsp/correlation.h"
#include "dsp/effect.h"
#include "templates/vecdeque.h"

namespace Mengu {
namespace dsp {

// shifts formants using lpc envelope estimation
class LPCFormantShifter: public Effect {
public:
    // tells an EffectChain what type of input the effect expects
    virtual InputDomain get_input_domain() override;

    // push new value of signal
    virtual void push_signal(const Complex *input, const uint32_t &size) override;

    // Last value of transformed signal
    virtual uint32_t pop_transformed_signal(Complex *output, const uint32_t &size) override;

    // number of samples that can be output given the current pushed signals of the Effect
    virtual uint32_t n_transformed_ready() const override;
    
    // resets state of effect to make it reading to take in a new sample
    virtual void reset() override;

    // The properties that this Effect exposes to be changed by GUI. 
    // The index that they are put in is considered the props id
    virtual std::vector<EffectPropDesc> get_property_descs() const override;
    
    // Sets a property with the specified id the value declared in the payload
    virtual void set_property(uint32_t id, EffectPropPayload data) override;

    // Gets the value of a property with the specified id
    virtual EffectPropPayload get_property(uint32_t id) const override;
private:
    VecDeque<Complex> _raw_buffer;

    static constexpr uint32_t ProcSize = 1 << 10;
    LPC<ProcSize, 50> _lpc;

    static void _reconstruct_freq(const float *residuals, 
                                const float *envelope, 
                                const float *phases,
                                Complex *output, 
                                const float env_shift_factor);

    float _shift_factor = 1.0f;
};

}
}
#endif