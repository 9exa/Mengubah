#include <cstdint>
#include <lv2.h>
#include <vector>

#include "audioplayers/Audioplayer.h"
#include "dsp/common.h"
#include "dsp/effect.h"
#include "dsp/pitchshifter.h"
#include "dsp/timestretcher.h"


using namespace Mengu;
using namespace dsp;

struct PluginHandler {
    std::vector<Effect *> effects;
    float *in_buffer;
    float *out_buffer;
    float *pitch_shift;
};

/* internal core methods */
static LV2_Handle instantiate (const struct LV2_Descriptor *descriptor, double sample_rate, const char *bundle_path, const LV2_Feature *const *features) {
    PluginHandler *plugin = new PluginHandler;
    plugin->effects = {
        new TimeStretchPitchShifter(new WSOLATimeStretcher(), 1),
        new TimeStretchPitchShifter(new PSOLATimeStretcher(), 1),
        new TimeStretchPitchShifter(new PhaseVocoderTimeStretcher(true), 1),
        new TimeStretchPitchShifter(new PhaseVocoderDoneRightTimeStretcher(true), 1),
    };

    return plugin;
}

static void connect_port (LV2_Handle instance, uint32_t port, void *data_location) {
    PluginHandler* plugin = (PluginHandler*) instance;
    if (plugin == nullptr) return;

    switch (port)
    {
    case 0:
        plugin->in_buffer = (float*) data_location;
        break;

    case 1:
        plugin->out_buffer = (float*) data_location;
        break;

    case 2:
        plugin->pitch_shift = (float*) data_location;
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

    Effect *effect = plugin->effects[0];
    effect->set_property(
        0, 
        EffectPropPayload {
            .type = Slider,
            .value = *(plugin->pitch_shift),
    });

    Complex *cbuffer = new Complex[sample_count];
    for (uint32_t i = 0; i < sample_count; i++) {
        cbuffer[i] = (Complex) plugin->in_buffer[i];
    }

    effect->push_signal(cbuffer, sample_count);
    effect->pop_transformed_signal(cbuffer, sample_count);

    for (uint32_t i = 0; i < sample_count; i++) {
        plugin->out_buffer[i] = cbuffer[i].real();
    }

    delete[] cbuffer;

    // for(uint32_t i = 0; i < sample_count; i++) {
    //     plugin->out_buffer[i] = plugin->in_buffer[i];
    // }
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

    for (auto effect: plugin->effects) {
        delete effect;
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
