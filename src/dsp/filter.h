/**
 * @file filter.h
 * @author your name (you@domain.com)
 * @brief Filter functions
 * @version 0.1
 * @date 2023-06-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef MENGU_FILTER
#define MENGU_FILTER

#include "dsp/common.h"

// the transfer function for a quadratic filter with denominator coefficients a1, a2 and numerator cofficients b0, b1, b2
// assumes z is on a unit circle
Complex quad_filter_trans(Complex z, float a1, float a2, float b0, float b1, float b2);


#endif