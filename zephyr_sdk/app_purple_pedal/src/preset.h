#ifndef APP_PRESET_H_
#define APP_PRESET_H_
#include <stdint.h>

void storage_init(void);

// Boot: Loads "active/" keys into RAM
int storage_load_active(struct_calibration_t *calib, struct_curves_t *curves);

// Save RAM -> "active/" keys (Call this when settings change live)
int storage_save_active(struct_calibration_t *calib, struct_curves_t *curves);

// Save RAM -> "preset/X/" keys
int storage_save_preset(int slot_id, struct_calibration_t *calib, struct_curves_t *curves);

// Load "preset/X/" -> RAM (and auto-save to "active/" so it persists)
int storage_load_preset(int slot_id, struct_calibration_t *calib, struct_curves_t *curves);

#endif