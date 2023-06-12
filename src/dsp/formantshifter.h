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
#include "dsp/loudness.h"
#include "templates/vecdeque.h"
#include <cstdint>

namespace Mengu {
namespace dsp {

// shifts formants using lpc envelope estimation
class LPCFormantShifter: public Effect {
public:
    LPCFormantShifter();
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
    VecDeque<Complex> _transformed_buffer;

    static constexpr uint32_t ProcSize = 1 << 9;
    static constexpr uint32_t HopSize = ProcSize * 3 / 5;
    static constexpr uint32_t OverlapSize = ProcSize - HopSize;

    LPC<ProcSize, 25> _lpc;

    void _shift_by_env(const Complex *input, 
                          Complex *output, 
                          const float *envelope, 
                          const float shift_factor);


    // Amplifies the formant_shifted samples so they have the same LUFS loudness as the raw_sample
    void _rescale_shifted_freqs(const Complex *raw_sample, Complex *shifted_sample);
    float _shift_factor = 1.0f;

    // cache the coefficents that result from the filters in LUFS calculation
    Complex _LUFS_coeffs[ProcSize];

    LUFSFilter _raw_sample_filter;
    LUFSFilter _shifted_sample_filter;
};

}
}
#endif