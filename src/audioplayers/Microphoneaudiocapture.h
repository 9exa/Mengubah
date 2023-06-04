#ifndef MENGA_MICROPHONE_AUDIO_CAPTURE
#define MENGA_MICROPHONE_AUDIO_CAPTURE
#include "dsp/effect.h"
#include "extras/miniaudio_split/miniaudio.h"
#include "templates/cyclequeue.h"
#include "templates/vecdeque.h"
#include <cstdint>
#include <miniaudio.h>
#include <string>
#include <vector>
#include "dsp/pitchshifter.h"

namespace Mengu {

class MicrophoneAudioCapture {
    // Reads microphone input, changes it(?) and outputs it, somewhere
public:
    MicrophoneAudioCapture();
    ~MicrophoneAudioCapture();

    CycleQueue<float> raw_bufferf;

    // Sets the active playback device
    void select_playback_device(int32_t device_ind);

    const std::vector<std::string> &get_playback_devices() const;

    void add_effect(dsp::Effect *effect);
    void remove_effect(uint32_t at);

    std::vector<dsp::Effect *> &get_effects();

    void refresh_context();
private:
    struct DData {
        MicrophoneAudioCapture *capture;
    };
    DData _ddata;

    ma_device _device;
    ma_context _context;
    ma_device_info *_playback_devices;
    std::vector<std::string> _playback_device_names;

    std::vector<dsp::Effect *> _effects; 

    static void _data_callback(ma_device *device, void *output, const void *input, ma_uint32 frame_count);
};

}

#endif