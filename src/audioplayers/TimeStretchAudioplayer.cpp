#include "TimeStretchAudioplayer.h"
#include "dsp/common.h"
#include "dsp/effect.h"
#include "dsp/fft.h"
#include "dsp/timestretcher.h"
#include "extras/miniaudio_split/miniaudio.h"
#include "templates/cyclequeue.h"
#include <cstdint>
#include <ostream>
#include <vector>

#define STB_VORBIS_HEADER_ONLY
#include "extras/stb_vorbis.c"    /* Enables Vorbis decoding. */

#include <stdexcept>
#include <iostream>
#include <string>


Mengu::TimeStretchAudioPlayer::TimeStretchAudioPlayer() {
    left_buffer.resize(BufferSize);
    right_buffer.resize(BufferSize);
    sample_buffer.resize(BufferSize);

    _time_stretchers[0] = new Mengu::dsp::PhaseVocoderTimeStretcher(true);
    _time_stretchers[1] = new Mengu::dsp::PhaseVocoderDoneRightTimeStretcher();
    _time_stretchers[2] = new Mengu::dsp::OLATimeStretcher(1 << 11);
    _time_stretchers[3] = new Mengu::dsp::WSOLATimeStretcher();
    _time_stretchers[4] = new Mengu::dsp::PSOLATimeStretcher();
    

    time_stretcher = _time_stretchers[0];
    
}

Mengu::TimeStretchAudioPlayer::~TimeStretchAudioPlayer() {
    if (file_loaded) {
        ma_device_uninit(&_device);
        ma_decoder_uninit(&_decoder);
    }

    for (uint32_t i = 0; i < NTimeStretcher; i++) {
        delete _time_stretchers[i];
    }
}

void Mengu::TimeStretchAudioPlayer::load_file(const char *path) {
    if (file_loaded) {
        stop();

        ma_device_uninit(&_device);
        ma_decoder_uninit(&_decoder);
    }

    ma_decoder_config decoder_config = ma_decoder_config_init_default();
    decoder_config.format = ma_format_f32;

    ma_result result = ma_decoder_init_file(path, &decoder_config, &_decoder);
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

void Mengu::TimeStretchAudioPlayer::play() {
    if (file_loaded) {
        ma_decoder_seek_to_pcm_frame(&_decoder, 0);
        if (ma_device_start(&_device) != MA_SUCCESS) {
            throw "could not play file";
        }
    }
}

void Mengu::TimeStretchAudioPlayer::stop() {
    if (file_loaded) {
        if (ma_device_stop(&_device) != MA_SUCCESS) {
            throw "could not stop file";
        }
    }

    time_stretcher->reset();
}


void Mengu::TimeStretchAudioPlayer::set_stretch_factor(float f) {
    time_stretcher->set_stretch_factor(f);
    dsp::EffectPropPayload payload {
        .type = dsp::Slider,
        .value = f,
    };
    time_stretcher->set_property(0, payload);
}


void Mengu::TimeStretchAudioPlayer::_data_callback(ma_device *device, void *output, const void *input, ma_uint32 frame_count) {
    DeviceData *ddata = (DeviceData *)device->pUserData;
    TimeStretchAudioPlayer *player = ddata->player;
    CycleQueue<Complex> &sample_buffer = player->sample_buffer;
    // std::vector<Complex> &left_buffer = player->left_buffer;
    // std::vector<Complex> &right_buffer = player->right_buffer;
    dsp::TimeStretcher *time_stretcher = player->time_stretcher;


    ma_decoder *decoder = ddata->decoder;
    if (decoder == nullptr) {
        return;
    }

    ma_uint32 output_channels = device->playback.channels;
    ma_uint32 input_channels = decoder->outputChannels;


    float *outputf = (float *)output;

    std::vector<Complex> left_samples(frame_count);
    std::vector<Complex> left_output(frame_count);

    ma_uint32 n_outputted = time_stretcher->pop_transformed_signal(left_output.data(), frame_count);
    while (n_outputted < frame_count) {
        ma_decoder_read_pcm_frames(decoder, output, frame_count, nullptr);

        for (ma_uint32 i = 0; i < frame_count; i++) {
            left_samples[i] = Complex(outputf[input_channels * i]);
        }

        time_stretcher->push_signal(left_samples.data(), frame_count);
        n_outputted += time_stretcher->pop_transformed_signal(left_output.data() + n_outputted, frame_count - n_outputted);

    }

    for (ma_uint32 channel_num = 0; channel_num < input_channels; channel_num++) {
        for (ma_uint32 i = 0; i < frame_count; i++) {
            // std::cout << "writing to speakers" << input_channels * i + channel_num << std::endl;

            outputf[output_channels * i + channel_num] = left_output[i].real();
        }
    }

    const ma_uint32 sample_coeff = 2;

    for (ma_uint32 i = 0; (i * sample_coeff) < frame_count; i++) {
        sample_buffer.push_back(0.7 * outputf[i * sample_coeff]);
    }

}
