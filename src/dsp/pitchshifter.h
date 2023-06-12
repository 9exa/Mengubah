/*
* (Real time) Pitch shifting object implementations
*/
#ifndef MENGA_PITCH_SHIFTER
#define MENGA_PITCH_SHIFTER

#include "dsp/correlation.h"
#include "dsp/effect.h"
#include "dsp/sampling.h"
#include "dsp/timestretcher.h"
#include "fft.h"
#include <cstdint>
#include <dsp/common.h>
#include <dsp/fft.h>
#include <templates/cyclequeue.h>
#include <templates/vecdeque.h>
#include <vector>

namespace Mengu {
namespace dsp {

class PitchShifter: public Effect {
public:

    ~PitchShifter() {}
    virtual InputDomain get_input_domain() override {
        return InputDomain::Time;
    }

    virtual void set_shift_factor(const float &factor) {
        _shift_factor = factor;
    };

    virtual std::vector<EffectPropDesc> get_property_descs() const override;
    
    virtual void set_property(uint32_t id, EffectPropPayload data) override;

    virtual EffectPropPayload get_property(uint32_t id) const override;
protected:
    float _shift_factor = 1.0f;
};

// Shifter in the frequency domain that uses LPC to estimate formant preservation. 
class PhaseVocoderPitchShifterV2: public PitchShifter {
public:
    PhaseVocoderPitchShifterV2();
    ~PhaseVocoderPitchShifterV2();

    virtual void push_signal(const Complex *input, const uint32_t &size) override;
    virtual uint32_t pop_transformed_signal(Complex *output, const uint32_t &size) override;

    virtual uint32_t n_transformed_ready() const override;

    virtual void reset() override;
private:
    // raw time-domain data
    VecDeque<Complex> _raw_buffer;
    // time domain data after pitch_shift
    VecDeque<Complex> _transformed_buffer;

    static constexpr uint32_t ProcSize = 1 << 9;
    static constexpr uint32_t OverlapSize = 1 << 6;

    LPC<ProcSize, 30> _lpc;

    uint32_t _samples_processed;
};

// Shifts by resampling a time stretcher
class TimeStretchPitchShifter: public PitchShifter {
public:
    TimeStretchPitchShifter(TimeStretcher *stretcher, uint32_t nchannels);
    ~TimeStretchPitchShifter();

    virtual void push_signal(const Complex *input, const uint32_t &size) override;
    virtual uint32_t pop_transformed_signal(Complex *output, const uint32_t &size) override;

    virtual uint32_t n_transformed_ready() const override;

    virtual void reset() override;
    
    virtual void set_shift_factor(const float &factor) override;

protected:
    float _formant_shift = 1.0f;
private:
    static constexpr uint32_t MinResampleInputSize = 1 << 10;
    // Size of transformed buffer before we increase resampling to compensate for drift
    static constexpr uint32_t IncreaseResampleThreshold = 5000;
    static constexpr uint32_t StandardResampleThreshold = 3000;

    // raw time-domain data
    VecDeque<Complex> _raw_buffer;
    // time domain data after pitch_shift
    VecDeque<Complex> _transformed_buffer;
    

    LinearResampler _resampler;
    TimeStretcher *_stretcher;


};
/*
class PTimeStretchPitchShifter: public PitchShifter {
public:
    PTimeStretchPitchShifter(uint32_t nchannels);
    ~PTimeStretchPitchShifter();

    virtual void push_signal(const Complex *input, const uint32_t &size) override;
    virtual uint32_t pop_transformed_signal(Complex *output, const uint32_t &size) override;

    virtual uint32_t n_transformed_ready() override;

    virtual void reset() override;
    
    virtual void set_shift_factor(const float &factor) override;

private:
    static constexpr uint32_t MinResampleInputSize = 1 << 10;
    // Size of transformed buffer before we increase resampling to compensate for drift
    static constexpr uint32_t IncreaseResampleThreshold = 10000;
    static constexpr uint32_t StandardResampleThreshold = 4000;

    // raw time-domain data
    VecDeque<Complex> _raw_buffer;
    // time domain data after pitch_shift
    VecDeque<Complex> _transformed_buffer;

    LinearResampler _resampler;
    PSOLATimeStretcher _pitch_shifting_stretcher;

};
*/
}; // namespace dsp
}; // namespace Mengu


#endif