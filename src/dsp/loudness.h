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
#include <cstdint>
#include <stdint.h>

// Loudness Units relative to Full Scale in the frequency domain
// first (positive) half of the frequency spectrum only
// Only supports 1 channel
float LUFS_freq(Complex *freqs, uint32_t size, uint32_t sample_rate = 48000);

#endif