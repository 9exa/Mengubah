#include <cstdint>
#include <lv2.h>
#include <array>

#include "dsp/common.h"
#include "dsp/effect.h"
#include "dsp/pitchshifter.h"
#include "dsp/formantshifter.h"
#include "dsp/timestretcher.h"



using namespace Mengu;
using namespace dsp;

struct PluginHandler {
    std::array<PitchShifter *, 3> pitch_shifters;
    std::array<Effect *, 2> formant_shifters;
    const float *in_buffer;
    float *out_buffer;
    const float *pitch_shifter_ind;
    const float *pitch_shift;
    const float *formant_shifter_ind;
    const float *formant_shift;
};

enum PitchShiftInd {
    WSOLAPitch = 0,
    PhaseVocoderPitch = 1,
    PhaseVocoderDoneRightPitch = 2,
};
enum FormantShiftInd {
    LPCFormant = 0,
    PSOLAFormant = 1,
};

/* internal core methods */
static LV2_Handle instantiate (const struct LV2_Descriptor *descriptor, double sample_rate, const char *bundle_path, const LV2_Feature *const *features) {
    PluginHandler *plugin = new PluginHandler;
    plugin->pitch_shifters = {
        new TimeStretchPitchShifter(new WSOLATimeStretcher(), 1),
        new TimeStretchPitchShifter(new PhaseVocoderTimeStretcher(), 1),
        new TimeStretchPitchShifter(new PhaseVocoderDoneRightTimeStretcher(), 1),
    };
    plugin->formant_shifters = {
        new LPCFormantShifter(),
        new TimeStretchPitchShifter(new PSOLATimeStretcher(), 1),
    };

    return plugin;
}

static void connect_port (LV2_Handle instance, uint32_t port, void *data_location) {
    PluginHandler* plugin = (PluginHandler*) instance;
    if (plugin == nullptr) return;

    switch (port)
    {
    case 0:
        plugin->in_buffer = (const float*) data_location;
        break;
    
    case 1:
        plugin->out_buffer = (float*) data_location;
        break;

    case 2:
        plugin->pitch_shifter_ind = (const float*) data_location;

    case 3:
        plugin->pitch_shift = (const float*) data_location;
        break;
    
    case 4:
        plugin->formant_shifter_ind = (const float*) data_location;
        break;

    case 5:
        plugin->formant_shift = (const float*) data_location;
        break;
    
    default:
        break;
    }
}

static void activate (LV2_Handle instance)
{
    /* not needed here */
}

static void run (LV2_Handle instance, uint32_t sample_count)
{
    PluginHandler* plugin = (PluginHandler*) instance;
    if (plugin == nullptr) return;
    if ((!plugin->in_buffer) || (!plugin->out_buffer) || (!plugin->pitch_shift)) return;

    // apply effects
    Effect *pitch_shifter = plugin->pitch_shifters[static_cast<PitchShiftInd>(*plugin->pitch_shifter_ind)];
    Effect *formant_shifter = plugin->formant_shifters[static_cast<FormantShiftInd>(*plugin->formant_shifter_ind)];

    pitch_shifter->set_property(0, EffectPropPayload {
        .type = Slider,
        .value = *plugin->pitch_shift,
    });
    formant_shifter->set_property(0, EffectPropPayload {
        .type = Slider,
        .value = *plugin->formant_shift,
    });

    static constexpr uint32_t ProcSize = 1 << 10;
    Complex cbuffer[ProcSize];
    uint32_t num_processed = 0;

    while (num_processed < sample_count) {
        uint32_t num_this_pass = MIN(ProcSize, sample_count - num_processed);
        for (uint32_t i = 0; i < num_this_pass; i++) {
            cbuffer[i] = (Complex) plugin->in_buffer[num_processed + i];
        }

        pitch_shifter->push_signal(cbuffer, num_this_pass);
        pitch_shifter->pop_transformed_signal(cbuffer, num_this_pass);
        
        formant_shifter->push_signal(cbuffer, num_this_pass);
        formant_shifter->pop_transformed_signal(cbuffer, num_this_pass);

        

        for (uint32_t i = 0; i < num_this_pass; i++) {
            plugin->out_buffer[num_processed + i] = cbuffer[i].real();
        }

        num_processed += num_this_pass;
    }
    

    

    
}

static void deactivate (LV2_Handle instance)
{
    /* not needed here */
}

static void cleanup (LV2_Handle instance) {
    PluginHandler *plugin = (PluginHandler *) instance;
    if (plugin == nullptr) {
        return;
    }

    for (auto pitch_shifter: plugin->pitch_shifters) {
        delete pitch_shifter;
    }

    for (auto formant_shifter: plugin->formant_shifters) {
        delete formant_shifter;
    }

    delete plugin;
}

static const void* extension_data (const char *uri)
{
    return nullptr;
}

/* descriptor */
static LV2_Descriptor const descriptor =
{
    "https://github.com/9exa/Mengubah/Mengubah",
    instantiate,
    connect_port,
    activate /* or NULL */,
    run,
    deactivate /* or NULL */,
    cleanup,
    extension_data /* or NULL */
};

/* interface */
LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor (uint32_t index)
{
    if (index == 0) return &descriptor;
    else return NULL;
}
