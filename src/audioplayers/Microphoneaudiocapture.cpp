#include "audioplayers/Microphoneaudiocapture.h"
#include "dsp/common.h"
#include "dsp/pitchshifter.h"
#include "dsp/timestretcher.h"
#include "extras/miniaudio_split/miniaudio.h"
#include <cstdint>
#include <stdexcept>
#include <string>
#include <iostream>
#include <vector>

using namespace Mengu;

MicrophoneAudioCapture::MicrophoneAudioCapture() {

    ma_context context;
    if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS) {
    
        std::string err_msg ("Could not establich audio context ");

        throw std::runtime_error(err_msg);
    }

    ma_device_info* pPlaybackInfos;
    ma_uint32 playbackCount;
    ma_device_info* pCaptureInfos;
    ma_uint32 captureCount;

    if (ma_context_get_devices(&context, &pPlaybackInfos, &playbackCount, &pCaptureInfos, &captureCount) != MA_SUCCESS) {
        throw std::runtime_error("Could not get devices");
    }

    // for (ma_uint32 i = 0; i < playbackCount; i++ ){
    //     std::cout <<  i << ". " << pPlaybackInfos[i].name << std::endl;
    // }

    _ddata = {this};
    // ma_device_info capture_info =pCaptureInfos[1];
    // ma_device_info playback = pPlaybackInfos[3];
    ma_device_config config = ma_device_config_init(ma_device_type_duplex);
    // config.capture.pDeviceID = &capture_info.id;
    config.capture.format = ma_format_f32;
    // config.playback.pDeviceID = &playback.id;
    config.playback.format    = ma_format_f32;
    config.playback.channels  = 1;
    // config.sampleRate         = 44100;
    config.dataCallback       = _data_callback;
    config.pUserData          = &_ddata;
    config.deviceType = ma_device_type_duplex;


    ma_device_init(nullptr, &config, &_device);
    ma_device_start(&_device);

    raw_bufferf.resize(1<<12);


    // _pitch_shifters[0] = new dsp::TimeStretchPitchShifter(new dsp::WSOLATimeStretcher(), _device.capture.channels);
    // _pitch_shifters[0] = new dsp::TimeStretchPitchShifter(new dsp::PhaseVocoderDoneRightTimeStretcher(), _device.capture.channels);
    // _pitch_shifters[1] = new dsp::TimeStretchPitchShifter(new dsp::PSOLATimeStretcher(), _device.capture.channels);

}

MicrophoneAudioCapture::~MicrophoneAudioCapture() {
    ma_device_stop(&_device);
    ma_device_uninit(&_device);

    for (auto effect: _effects) {
        delete effect;
    }
    
}

const std::vector<std::string> &MicrophoneAudioCapture::get_playback_devices() const {
    return _playback_device_names;
}

void MicrophoneAudioCapture::add_effect(dsp::Effect *effect) {
    _effects.push_back(effect);
}

void MicrophoneAudioCapture::remove_effect(uint32_t at) {
    _effects.erase(_effects.begin() + at);
}

std::vector<dsp::Effect *> &MicrophoneAudioCapture::get_effects() {
    return _effects;
}

void MicrophoneAudioCapture::refresh_context() {
    ma_uint32 playbackCount;
    ma_device_info* pCaptureInfos;
    ma_uint32 captureCount;

    if (ma_context_get_devices(&_context, &_playback_devices, &playbackCount, &pCaptureInfos, &captureCount) != MA_SUCCESS) {
        throw std::runtime_error("Could not get devices");
    }

    // copy names
    _playback_device_names.clear();
    for (ma_uint32 device_ind = 0; device_ind < playbackCount; device_ind++) {
        _playback_device_names.emplace_back(_playback_devices[device_ind].name);
    }
}

void MicrophoneAudioCapture::_data_callback(ma_device *device, void *output, const void *input, ma_uint32 frame_count) {
    const float *inputf = (float *)input;
    float *outputf = (float *)output;
    DData *ddata = (DData *)device->pUserData;
    MicrophoneAudioCapture *capture = ddata->capture;

    std::vector<Complex> cbuffer(frame_count);
    std::vector<Complex> cbufferout(frame_count);

    for (ma_uint32 frame = 0; frame < frame_count; frame++) {
        cbuffer[frame] = inputf[frame];
    }

    for (auto effect: capture->_effects) {
        effect->push_signal(cbuffer.data(), frame_count);
        effect->pop_transformed_signal(cbuffer.data(), frame_count);
    }

    for (ma_uint32 frame = 0; frame < frame_count; frame++) {
        outputf[frame] = cbuffer[frame].real();
        capture->raw_bufferf.push_back(cbuffer[frame].real());
    }
}
