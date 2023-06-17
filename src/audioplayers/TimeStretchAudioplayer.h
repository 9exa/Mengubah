#ifndef MENGA_TIME_STRETCH_AUDIO_PLAYER
#define MENGA_TIME_STRETCH_AUDIO_PLAYER

#include "dsp/common.h"
#include "dsp/fft.h"
#include "dsp/timestretcher.h"
#include "miniaudio.h"


#include <cstdint>
#include <templates/cyclequeue.h>
#include <dsp/pitchshifter.h>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

namespace Mengu {

class TimeStretchAudioPlayer {
public:
    TimeStretchAudioPlayer();
    ~TimeStretchAudioPlayer();

    uint32_t load_file(const fs::path &file_path);
    
    void play();

    void stop();

    void set_stretch_factor(float f);

    CycleQueue<Complex> sample_buffer;

    std::vector<Complex> left_buffer;
    std::vector<Complex> right_buffer;

    static inline const int32_t BufferSize = 1 << 11;
    
    static constexpr uint8_t NTimeStretcher = 5;
    dsp::TimeStretcher *time_stretcher;
    dsp::TimeStretcher *time_stretchers[NTimeStretcher];



private:
    struct DeviceData {
        Mengu::TimeStretchAudioPlayer *player;
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