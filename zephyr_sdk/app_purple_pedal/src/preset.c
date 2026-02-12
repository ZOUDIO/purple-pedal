#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "preset.h"

LOG_MODULE_REGISTER(storage, CONFIG_APP_LOG_LEVEL);

// Temp buffers for loading
static struct_calibration_t temp_calib;
static struct_curves_t temp_curves;

/* * UNIVERSAL HANDLER
 * Handles both "active/..." and "preset/..." keys.
 * * Scenarios:
 * 1. "active/calib" -> Parse "calib"
 * 2. "preset/0/calib" -> Parse "0/calib" -> Parse ID 0 -> Parse "calib"
 */
static int storage_handle_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
    const char *next = name;
    
    // Check if this is a PRESET key (starts with a digit like "0/...")
    if (isdigit((unsigned char)*name)) {
        // It's a preset! Skip the ID and slash.
        // "0/calib" -> atoi gets 0.
        // strchr finds the slash.
        const char *slash = strchr(name, '/');
        if (!slash) return -ENOENT;
        next = slash + 1; // "calib"
    }
    
    // Now 'next' points to the type ("calib" or "curve")
    if (strcmp(next, "calib") == 0) {
        if (len != sizeof(temp_calib)) return -EINVAL;
        return read_cb(cb_arg, &temp_calib, sizeof(temp_calib));
    }
    
    if (strcmp(next, "curve") == 0) {
        if (len != sizeof(temp_curves)) return -EINVAL;
        return read_cb(cb_arg, &temp_curves, sizeof(temp_curves));
    }

    return -ENOENT;
}

// Register TWO roots: "active" and "preset"
// They both point to the same handler logic for simplicity.
SETTINGS_STATIC_HANDLER_DEFINE(h_active, "active", NULL, storage_handle_set, NULL, NULL);
SETTINGS_STATIC_HANDLER_DEFINE(h_preset, "preset", NULL, storage_handle_set, NULL, NULL);


void storage_init(void)
{
    settings_subsys_init();
}

// BOOT: Load "active"
int storage_load_active(struct_calibration_t *calib, struct_curves_t *curves)
{
    int rc;
    LOG_INF("Loading Active Config...");
    
    // Load Calibration
    rc = settings_load_subtree("active/calib");
    if (rc == 0) *calib = temp_calib; // Copy buffer to user RAM
    
    // Load Curves
    rc = settings_load_subtree("active/curve");
    if (rc == 0) *curves = temp_curves;
    
    return 0;
}

// PERSIST: Save RAM to "active"
int storage_save_active(struct_calibration_t *calib, struct_curves_t *curves)
{
    int rc;
    rc = settings_save_one("active/calib", calib, sizeof(*calib));
    rc |= settings_save_one("active/curve", curves, sizeof(*curves));
    return rc;
}

// SAVE PRESET: Save RAM to "preset/X"
int storage_save_preset(int slot, struct_calibration_t *calib, struct_curves_t *curves)
{
    char key[32];
    int rc;
    
    snprintf(key, sizeof(key), "preset/%d/calib", slot);
    rc = settings_save_one(key, calib, sizeof(*calib));
    
    snprintf(key, sizeof(key), "preset/%d/curve", slot);
    rc |= settings_save_one(key, curves, sizeof(*curves));
    
    return rc;
}

// LOAD PRESET: Load "preset/X" -> RAM -> "active"
int storage_load_preset(int slot, struct_calibration_t *calib, struct_curves_t *curves)
{
    char key[32];
    int rc;

    // Load Calib from Preset
    snprintf(key, sizeof(key), "preset/%d/calib", slot);
    rc = settings_load_subtree(key);
    if (rc == 0) *calib = temp_calib;

    // Load Curve from Preset
    snprintf(key, sizeof(key), "preset/%d/curve", slot);
    rc = settings_load_subtree(key);
    if (rc == 0) *curves = temp_curves;
    
    // AUTO-SAVE to Active
    // This ensures if the user reboots now, they stay on this preset.
    storage_save_active(calib, curves);
    
    return 0;
}