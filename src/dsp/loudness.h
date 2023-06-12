/**
 * @file loudness.h
 * @author your name (you@domain.com)
 * @brief Functions for calculating loudness in a signal
 * @version 0.1
 * @date 2023-06-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef MENGU_LOUDNESS
#define MENGU_LOUDNESS

#include "dsp/common.h"
#include "dsp/filter.h"
#include <cstdint>
#include <stdint.h>

namespace Mengu {
namespace dsp {

// Loudness Units relative to Full Scale of a sample in the frequency domain
// first (positive) half of the frequency spectrum only
// Only supports 1 channel
float LUFS_freq(Complex *freqs, uint32_t size, uint32_t sample_rate = 48000);

// The value of the transfer function associated with the described frequency bin
Complex LUFS_filter_transfer(float freq);

// Performs the filter step associated with LUFS
class LUFSFilter {
public:
    LUFSFilter();

    void transform(const float *input, float *output, uint32_t size);

    void reset();

private:
    BiquadFilter _high_shelf_filter; // Stage 1 filter
    BiquadFilter _high_pass_filter; // Stage 2 filter
};

}
}


#endif