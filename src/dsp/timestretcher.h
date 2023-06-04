#ifndef MENGA_TIME_STRETCHER
#define MENGA_TIME_STRETCHER

#include "dsp/common.h"
#include "dsp/correlation.h"
#include "dsp/effect.h"
#include "dsp/fft.h"
#include "templates/vecdeque.h"
#include <array>
#include <cstdint>
#include <vector>

namespace Mengu {
namespace dsp {

class TimeStretcher: public Effect {
public:
    virtual ~TimeStretcher() {}
    virtual InputDomain get_input_domain() override;

    virtual void set_stretch_factor(const float &scale);

    virtual std::vector<EffectPropDesc> get_property_descs() const override;

    virtual void set_property(uint32_t id, EffectPropPayload data) override;

    virtual EffectPropPayload get_property(uint32_t id) const override;

protected:
    float _stretch_factor = 1.0f;


    // keep track of resampled size error due to rounding errors
    double _stretched_sample_truncated = 0.0;
};

// Classic timeshifter be scale the phases of frequency bins in the time dimension
class PhaseVocoderTimeStretcher: public TimeStretcher {
public:
    PhaseVocoderTimeStretcher(bool _preserve_formants = false);

    virtual void push_signal(const Complex *input, const uint32_t &size) override;
    virtual uint32_t pop_transformed_signal(Complex *output, const uint32_t &size) override;

    virtual uint32_t n_transformed_ready() const override;

    // virtual void set_stretch_factor(const float &stretch_factor) override;

    virtual void reset() override;
protected:

    static constexpr uint32_t WindowSize = 1 << 9;
    static constexpr uint32_t SynthesisHopSize = 400;

    static constexpr uint32_t NStoredWindows = 2;

    std::array<float, WindowSize / 2> _prev_raw_mag2s;
    std::array<float, WindowSize / 2> _prev_raw_phases;
    // phases of the last transformed samples
    std::array<float, WindowSize / 2> _last_scaled_phases;

    VecDeque<Complex> _raw_buffer;
    VecDeque<Complex> _transformed_buffer;

    virtual std::array<float, WindowSize / 2> _calc_scaled_magnitudes();
    // expects time_deltas to be scaled
    virtual std::array<float, WindowSize / 2> _calc_scaled_phases(const std::array<Complex, WindowSize / 2> &curr_freqs, const uint32_t hopsize);

    std::array<Complex, WindowSize / 2> _load_new_freq_window(const std::array<Complex, WindowSize> &sample);
    void _replace_prev_freqs(const std::array<float, WindowSize / 2> &curr_mags, const std::array<float, WindowSize / 2> &curr_phases);
private:

    // FFT _fft;
    // for finding the envelope and doing fft
    LPC<WindowSize, 50> _lpc;

    bool _preserve_formants = false;

    std::array<Complex, WindowSize> _calc_new_samples(const float *amplitudes, const float *phase_deltas);

};

// 'Phase vocoder done right' implementation
class PhaseVocoderDoneRightTimeStretcher: public PhaseVocoderTimeStretcher {
public:
    PhaseVocoderDoneRightTimeStretcher(bool _preserve_formants = false);
protected:
    virtual std::array<float, WindowSize / 2> _calc_scaled_phases(const std::array<Complex, WindowSize / 2> &curr_phases, const uint32_t hopsize) override;
private:
    // Phase propagation algorithm as described in the paper
    std::array<float, WindowSize / 2> _propagate_phase_gradients(const std::array<float, WindowSize / 2> &phase_time_deltas, 
                                                             const std::array<float, WindowSize / 2> &phase_freq_deltas, 
                                                             const std::array<float, WindowSize / 2> &last_stretched_phases, 
                                                             const std::array<float, WindowSize / 2> &prev_mag2s,
                                                             const std::array<float, WindowSize / 2> &next_mag2s,
                                                             const float stretch_factor,
                                                             const float tolerance = 1e-5f);
};

// Syncronised OverLap and Add time stretcher with fixed window size
class OLATimeStretcher: public TimeStretcher {
public:
    OLATimeStretcher(uint32_t w_size);

    static inline const float MinScale = 0.05;

    // std::vector<Complex> stretch_and_overlap_window2(const Complex *input);

    virtual void push_signal(const Complex *input, const uint32_t &size) override;
    virtual uint32_t pop_transformed_signal(Complex *output, const uint32_t &size) override;

    virtual uint32_t n_transformed_ready() const override;

    virtual void reset() override;
    
private:
    uint32_t _window_size;

    uint32_t _overlap;
    uint32_t _selection_window;
    // uint32_t _sample_skip;

    float _desired_extension = 0.0f;

    VecDeque<Complex> _raw_buffer;
    VecDeque<Complex> _transformed_buffer;

};

// timestrech where extensions are added to best match wave form with. Fixed window size. not fixed soutput size (may be slightly longer)
class WSOLATimeStretcher: public TimeStretcher {
public:
    WSOLATimeStretcher();

    virtual void push_signal(const Complex *input, const uint32_t &size) override;
    virtual uint32_t pop_transformed_signal(Complex *output, const uint32_t &size) override;

    virtual uint32_t n_transformed_ready() const override;
    
    virtual void reset() override;
private:
    VecDeque<Complex> _raw_buffer;
    VecDeque<Complex> _transformed_buffer;

    // length of the the input buffer must be before transforming and size of arrays in intermediate calculations. Should catch up to 1000hz
    static constexpr uint32_t SampleProcSize = 1 << 11;

    // length of a window
    static constexpr uint32_t WindowSize = 1 << 10;

    // stretches the sample, and adds it to the transform buffer, tje position of each window is based on the autocorrelation
    // returns how many frames were used and can be discarded
    uint32_t _stretch_sample_and_add(const Complex *sample);

    // Basically, the beginning of the next overlap(_left_tail_start) can be before the beggining of the last overlap(_right_tail_start)
    // and the size of the overlap can change on each process. So store both.
    uint32_t _last_overlap_start = 0;
    uint32_t _next_overlap_start = 0;


};

// Formant preserving, pitch changing time stretch
// Inherits a lot of algorithmic functionality from SOLA
class PSOLATimeStretcher: public TimeStretcher {
public:
    PSOLATimeStretcher();

    virtual void push_signal(const Complex *input, const uint32_t &size) override;
    virtual uint32_t pop_transformed_signal(Complex *output, const uint32_t &size) override;

    virtual uint32_t n_transformed_ready() const override;

    virtual void reset() override;
private:

    VecDeque<Complex> _raw_buffer;
    VecDeque<Complex> _transformed_buffer;

    static constexpr uint32_t SampleProcSize = 1 << 11;

    // basically how accurate the reconstruction will be. Assumes ~44kHz input, good for operation in up to 16kHz
    static constexpr uint32_t LPCSize = (uint32_t) 1.25 * 16;

    static constexpr uint32_t MinFreqHz = 50;
    static constexpr uint32_t MaxFreqHz = 800;

    static constexpr uint32_t InputSampleRate = 44100;

    static constexpr uint32_t MinFreqInd = MinFreqHz * SampleProcSize / InputSampleRate; 
    static constexpr uint32_t MaxFreqInd = MaxFreqHz * SampleProcSize / InputSampleRate; 

    // the amount of frames in _transformed_buffer that need to remain in case of overlapping future samples
    static constexpr uint32_t MaxBackWindowOverlap = (1.0f / MinFreqHz * InputSampleRate) + 1;

    // for finding fundemental frequency
    // FFT _fft;
    LPC<SampleProcSize, LPCSize> _lpc;

    // returns the index of the fundemental frequency (pitch) in an fft of samples
    int _est_fund_frequency(const Complex *sample);
    // store the last estimated pitches. the median will be used
    VecDeque<int> _last_periods;


    //used to meld windows in the same sample of different length (peaks are not uniformly spaced)
    uint32_t _next_right_window_overlap = 0;

    // estimate the peaks in the upcoming sample
    std::vector<uint32_t> _find_upcoming_peaks(const Complex *samples, const uint32_t est_period);

    // stretches the sample, and adds it to the transform buffer;
    void _stretch_peaks_and_add(const Complex *samples, const std::vector<uint32_t> &est_peaks);
    


};

}
}

#endif