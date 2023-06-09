#include "Audioplayer.h"
#include "dsp/common.h"
#include "dsp/fft.h"
#include "dsp/formantshifter.h"
#include "dsp/pitchshifter.h"
#include "dsp/timestretcher.h"
#include "extras/miniaudio_split/miniaudio.h"
#include "templates/cyclequeue.h"
#include <cstdint>
#include <vector>

#define STB_VORBIS_HEADER_ONLY
#include "extras/stb_vorbis.c"    /* Enables Vorbis decoding. */



#include <stdexcept>
#include <iostream>
#include <string>


Mengu::AudioPlayer::AudioPlayer() {
    pitch_shifters[0] = new Mengu::dsp::SOLAPitchShifter(new dsp::WSOLATimeStretcher, 1);
    pitch_shifters[1] = new Mengu::dsp::SOLAPitchShifter(new dsp::PSOLATimeStretcher, 1);
    pitch_shifters[2] = new Mengu::dsp::SOLAPitchShifter(new dsp::PhaseVocoderDoneRightTimeStretcher(true), 1);
    pitch_shifters[3] = new Mengu::dsp::SOLAPitchShifter(new dsp::PhaseVocoderTimeStretcher(true), 1);
    pitch_shifters[4] = new Mengu::dsp::LPCFormantShifter();
    // pitch_shifters[2] = new Mengu::dsp::PhaseVocoderPitchShifterV2();
    // pitch_shifter = new Mengu::dsp::PhaseVocoderPitchShifter(BufferSize);
    pitch_shifter = pitch_shifters[0];
    left_buffer.resize(BufferSize);
    right_buffer.resize(BufferSize);
    
}

Mengu::AudioPlayer::~AudioPlayer() {
    if (file_loaded) {
        ma_device_uninit(&_device);
        ma_decoder_uninit(&_decoder);
    }
    for (uint32_t i = 0; i < NPitchShifters; i++){
        delete pitch_shifters[i];
    }
}

void Mengu::AudioPlayer::load_file(const char *path) {
    if (file_loaded) {
        ma_device_uninit(&_device);
        ma_decoder_uninit(&_decoder);
    }
    ma_result result = ma_decoder_init_file(path, nullptr, &_decoder);
    if (result != MA_SUCCESS) {
        std::string err_msg ("Could not load file with ");
        err_msg += std::to_string(result);

        throw std::runtime_error(err_msg);
    }

    ma_device_config device_config = ma_device_config_init(ma_device_type_playback);
    device_config.playback.format = _decoder.outputFormat;
    device_config.playback.channels = _decoder.outputChannels;
    device_config.sampleRate = _decoder.outputSampleRate;
    device_config.dataCallback = _data_callback;
    ddata = {this, &_decoder};
    device_config.pUserData = &ddata;

    if (ma_device_init(nullptr, &device_config, &_device) != MA_SUCCESS) {
        throw std::runtime_error("Could not init audio device");
    }

    ma_device_set_master_volume(&_device, 0.5);

    file_loaded = true;
}

void Mengu::AudioPlayer::play() {
    if (file_loaded) {
        stop();

        ma_decoder_seek_to_pcm_frame(&_decoder, 0);
        if (ma_device_start(&_device) != MA_SUCCESS) {
            throw "could not play file";
        }
    }
}

void Mengu::AudioPlayer::stop() {
    if (file_loaded) {
        if (ma_device_stop(&_device) != MA_SUCCESS) {
            throw "could not stop file";
        }
    }

    pitch_shifter->reset();
}

void Mengu::AudioPlayer::set_pitch_shifter(uint32_t ind) {
    if (ind < NPitchShifters) {
        pitch_shifter = pitch_shifters[ind];
    }
}

void Mengu::AudioPlayer::_data_callback(ma_device *device, void *output, const void *input, ma_uint32 frame_count) {
    DeviceData *ddata = (DeviceData *)device->pUserData;
    AudioPlayer *player = ddata->player;
    CycleQueue<float> &buffer = player->sample_buffer;
    auto *pitch_shifter = player->pitch_shifter;

    ma_decoder *decoder = ddata->decoder;
    if (decoder == nullptr) {
        return;
    }


    ma_uint32 input_channels = decoder->outputChannels;
    ma_uint32 output_channels = device->playback.channels;

    float *outputf = (float *)output;

    std::vector<Complex> left_samples(frame_count);

    for (uint32_t j = 0; j < 1; j++) {
        ma_decoder_read_pcm_frames(decoder, output, frame_count, nullptr);
        
        for (ma_uint32 i = 0; i < frame_count; i++) {
            left_samples[i] = Complex(outputf[input_channels * i]);
        }

        pitch_shifter->push_signal(left_samples.data(), frame_count);
    }
    
    pitch_shifter->pop_transformed_signal(left_samples.data(),frame_count);

    for (ma_uint32 channel = 0; channel < output_channels; channel++) {
        for (ma_uint32 i = 0; i < frame_count; i++) {
            outputf[output_channels * i + channel] = 0.5* left_samples[i].real();
        }
    }

    int sample_coeff = 8;
    for (ma_uint32 i = 0; i * sample_coeff < frame_count; i++) {
        buffer.push_back(outputf[i * sample_coeff]);
    }


    (void)input;
}
