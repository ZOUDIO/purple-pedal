#include "curve.h"
#include <string.h> // for memcpy
#include "common.h"

static bool _curve_active = false; // Default to Linear-Only

// Storage for 3 pedals (Clutch, Brake, Accelerator)
static uint16_t _curves[CURVE_CHANNELS][CURVE_POINTS];

void curve_init(void) {
    // Fill all curves with a default 1:1 linear line (0, 1024, 2048...)
    for (int ch = 0; ch < CURVE_CHANNELS; ch++) {
        for (int i = 0; i < CURVE_POINTS; i++) {
            // 65535 / 64 = 1023.98 -> 1024 steps
            uint32_t val = i * 1024; 
            if (val > 65535) val = 65535;
            _curves[ch][i] = (uint16_t)val;
        }
    }
    _curve_active = false;
    // // --- CLUTCH DEFAULT (Index 0) ---
    // static const uint16_t default_clu[] = {
    //     0, 32, 128, 288, 512, 800, 1152, 1568, 2048, 2592, 3200, 3872, 4608, 
    //     5408, 6272, 7200, 8192, 9248, 10368, 11552, 12800, 14112, 15488, 16928, 
    //     18432, 20000, 21632, 23328, 25088, 26912, 28800, 30752, 32768, 34783, 
    //     36735, 38623, 40447, 42207, 43903, 45535, 47103, 48607, 50047, 51423, 
    //     52735, 53983, 55167, 56287, 57343, 58335, 59263, 60127, 60927, 61663, 
    //     62335, 62943, 63487, 63967, 64383, 64735, 65023, 65247, 65407, 65503, 
    //     65535
    // };
    // // --- BRAKE DEFAULT (Index 1) ---
    // static const uint16_t default_brk[] = {
    //     0, 2, 11, 31, 64, 112, 176, 259, 362, 486, 632, 803, 998, 1219, 1467, 
    //     1743, 2048, 2383, 2749, 3147, 3578, 4042, 4540, 5074, 5644, 6250, 6894, 
    //     7576, 8297, 9058, 9859, 10701, 11585, 12511, 13481, 14494, 15552, 16654, 
    //     17803, 18997, 20238, 21527, 22864, 24249, 25684, 27168, 28702, 30288, 
    //     31925, 33613, 35355, 37149, 38997, 40899, 42856, 44867, 46935, 49058, 
    //     51238, 53475, 55770, 58123, 60534, 63005, 65535
    // };
    // // --- ACCELERATOR DEFAULT (Index 2) ---
    // static const uint16_t default_acc[] = {
    //     0, 2530, 5001, 7412, 9765, 12060, 14297, 16477, 18600, 20668, 22679, 
    //     24636, 26538, 28386, 30180, 31922, 33610, 35247, 36833, 38367, 39851, 
    //     41286, 42671, 44008, 45297, 46538, 47732, 48881, 49983, 51041, 52054, 
    //     53024, 53950, 54834, 55676, 56477, 57238, 57959, 58641, 59285, 59891, 
    //     60461, 60995, 61493, 61957, 62388, 62786, 63152, 63487, 63792, 64068, 
    //     64316, 64537, 64732, 64903, 65049, 65173, 65276, 65359, 65423, 65471, 
    //     65504, 65524, 65533, 65535
    // };
    
    // // Copy defaults into the active curve buffer
    // memcpy(_curves[SETTING_INDEX_CLUTCH],      default_clu, sizeof(default_clu));
    // memcpy(_curves[SETTING_INDEX_BRAKE],       default_brk, sizeof(default_brk));
    // memcpy(_curves[SETTING_INDEX_ACCELERATOR], default_acc, sizeof(default_acc));

    // // Enable the curve logic by default
    // _curve_active = true; 
}

void curve_set_active(bool active) {
    _curve_active = active;
}

bool curve_is_active(void) {
    return _curve_active;
}

void curve_update_data(uint8_t channel, uint8_t start_idx, uint8_t count, uint16_t *data) {
    if (channel >= CURVE_CHANNELS) return;
    if (start_idx + count > CURVE_POINTS) return;

    // Copy new points into RAM
    memcpy(&_curves[channel][start_idx], data, count * sizeof(uint16_t));
}

uint16_t curve_apply(uint8_t channel, uint16_t linear_input) {
    // TOGGLE CHECK: If inactive, pass through the linear value unchanged
    if (!_curve_active) return linear_input;
    if (channel >= CURVE_CHANNELS) return linear_input;

    // Bit-Shift Optimization for 65 points
    uint8_t index = linear_input >> 10; 
    if (index >= (CURVE_POINTS - 1)) index = (CURVE_POINTS - 2);
    
    uint16_t remainder = linear_input & 0x3FF; // Input % 1024

    uint16_t y1 = _curves[channel][index];
    uint16_t y2 = _curves[channel][index + 1];
    
    // Interpolate: y1 + (diff * remainder / 1024)
    int32_t diff = (int32_t)y2 - (int32_t)y1;
    return y1 + ((diff * remainder) >> 10);
}