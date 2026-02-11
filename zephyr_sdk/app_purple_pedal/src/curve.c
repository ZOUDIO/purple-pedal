#include "curve.h"
#include <string.h> // for memcpy
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include "common.h"
#include <zephyr/settings/settings.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(curve, CONFIG_LOG_DEFAULT_LEVEL);


uint16_t curve_apply(uint8_t channel, uint16_t linear_input) {
    // Safety check
    if (channel >= CURVE_CHANNELS) return linear_input;

    // --- Optimization: Read directly from Global RAM ---
    // We access 'current_curves' which is defined in common.h/main.c
    
    // 1. Find chunk index
    uint8_t index = linear_input >> 10; 
    
    if (index >= (CURVE_POINTS - 1)) {
        return current_curves.points[channel][CURVE_POINTS - 1];
    }

    // 2. Linear Interpolation
    uint16_t x_start = index << 10;
    uint16_t y_start = current_curves.points[channel][index];
    uint16_t y_end   = current_curves.points[channel][index + 1];

    uint32_t fraction = linear_input - x_start;
    int32_t delta = (int32_t)y_end - (int32_t)y_start;

    // Bit-shift division ( / 1024)
    int32_t result = y_start + ((delta * (int32_t)fraction) >> 10);

    return (uint16_t)result;
}