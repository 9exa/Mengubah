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

// Changes the pitch of a signal in real time by shifting the ShortFFT (one halves) of the time domain signal
// expects input in the Time Domain 
class PhaseVocoderPitchShifter: public PitchShifter {

public:
    PhaseVocoderPitchShifter(uint32_t buffer_size);

    ~PhaseVocoderPitchShifter();

    virtual void set_shift_factor(const float &factor) override;

    virtual void push_signal(const Complex *input, const uint32_t &size) override;
    virtual uint32_t pop_transformed_signal(Complex *output, const uint32_t &size) override;

    virtual uint32_t n_transformed_ready() const override;

    virtual void reset() override;

    bool use_interp = false;

    const VecDeque<Complex> &get_raw_buffer();
    

protected:
    float _shift_factor_reciprical = 1.0f; // resampling will be done by dividing origional index by the shift factor

private:
    // raw time-domain data
    VecDeque<Complex> _raw_buffer;
    // time domain data after pitch_shift
    VecDeque<Complex> _transformed_buffer;

    uint32_t _raw_buffer_size;
    uint32_t _transformed_buffer_size;    

    void _recalculate_transformed_buffer();

    // Sets the phase of the new samples to be contiguous with previous samples
    void _determine_phase(Complex *new_samples, const uint32_t &size);

    void _mix_into_transformed(const Complex *new_samples, const uint32_t &size, const int method = 0);
    
    // shift one half of the of a frequence spectrum by shift_factor. Set reverse to true to shift the negative half.
    void _shift_freqs(const Complex *input, Complex *output, const uint32_t &size, const bool &reverse);

    // number of samples of overlapp between each neighbouring sfft segmeent
    uint32_t _window_size;
    // size of sample packets to perform transform 
    uint32_t _sfft_size;

    // used to sync phase between overlapping sffts
    std::vector<Complex> _last_sfft;
    // "phases" are stored as indexes to the _fft's _es to prevent having to do trig functions to convert between actual phases
    // std::vector<uint32_t> _last_phases;

    // number of raw samples yet to transformed to frequency domain
    uint32_t _n_unprocessed;

    // Declaring _fft last ensures that _sfft_size will initialise first and can be used to initialise _fft
    FFT _fft;


    // bufferes used in in
    std::vector<Complex> _freq_;
    std::vector<Complex> _shifted_;
    std::vector<Complex> _result_;

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
class SOLAPitchShifter: public PitchShifter {
public:
    SOLAPitchShifter(TimeStretcher *stretcher, uint32_t nchannels);
    ~SOLAPitchShifter();

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
class PSOLAPitchShifter: public PitchShifter {
public:
    PSOLAPitchShifter(uint32_t nchannels);
    ~PSOLAPitchShifter();

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