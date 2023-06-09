#ifndef MENGA_AUDIO_PLAYER
#define MENGA_AUDIO_PLAYER

#include "dsp/effect.h"
#include "dsp/fft.h"
#include "miniaudio.h"


#include <cstdint>
#include <templates/cyclequeue.h>
#include <dsp/pitchshifter.h>

namespace Mengu {

class AudioPlayer {
public:
    AudioPlayer();
    ~AudioPlayer();

    void load_file(const char *path);
    
    void play();

    void stop();

    CycleQueue<float> sample_buffer;

    CycleQueue<Complex> left_buffer;
    CycleQueue<Complex> right_buffer;

    static inline const int32_t BufferSize = 1 << 10;
    
    dsp::Effect *pitch_shifter;
    static constexpr uint32_t NPitchShifters = 5;
    dsp::Effect *pitch_shifters[NPitchShifters];

    void set_pitch_shifter(uint32_t ind);


private:
    struct DeviceData {
        Mengu::AudioPlayer *player;
        ma_decoder *decoder;
    };
    ma_device _device;
    ma_decoder _decoder;
    DeviceData ddata;


    static void _data_callback(ma_device *device, void *output, const void *input, ma_uint32 frame_count);

    bool file_loaded = false;
};

};


#endif