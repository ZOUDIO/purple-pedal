#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#include "common.h"
LOG_MODULE_REGISTER(nvs_settings, CONFIG_APP_LOG_LEVEL);

#define SETTING_GAMEPAD_ROOT "gamepad"

#define SETTING_SUB_CALIBRATION "calib"

#define SETTING_SUB_CURVE "curve"

#define SETTING_SUB_CURVE_ACTIVE "active"

#define SETTING_CALIBRATION SETTING_GAMEPAD_ROOT"/"SETTING_SUB_CALIBRATION
#define SETTING_CURVE_ACTIVE SETTING_GAMEPAD_ROOT"/"SETTING_SUB_CURVE"/"SETTING_SUB_CURVE_ACTIVE

#define SETTING_CALIBRATION_LEN (sizeof(struct gamepad_calibration))

static struct gamepad_calibration gp_calibration = {
    .offset = {[0 ... SETTING_INDEX_TOTAL-1] = LOAD_CELL_DEFAULT_OFFSET},
    .scale = {[0 ... SETTING_INDEX_TOTAL-1] = LOAD_CELL_DEFAULT_SCALE},
};

static struct gamepad_curve_context gp_curve_ctx = {
	.active_curve_slot = 0,
	.curve_slot = {
		[0 ... GAMEPAD_TOTAL_CURVE_SLOT_NUM-1] = {
			.accelerator = {[0 ... GAMEPAD_FEATURE_REPORT_CURVE_NUM_POINTS-1] = 0},
			.brake = {[0 ... GAMEPAD_FEATURE_REPORT_CURVE_NUM_POINTS-1] = 0},
			.clutch = {[0 ... GAMEPAD_FEATURE_REPORT_CURVE_NUM_POINTS-1] = 0},
		}
	}
};

int gampepad_setting_handle_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	int rc;
	const char *next;
	if (settings_name_steq(name, SETTING_SUB_CALIBRATION, &next) && !next) {
		if (len != SETTING_CALIBRATION_LEN) {
			return -EINVAL;
		}
		rc = read_cb(cb_arg, &gp_calibration, SETTING_CALIBRATION_LEN);
		if (rc >= 0) {
			return 0;
		}
		return rc;
	}
    return -ENOENT;
}

SETTINGS_STATIC_HANDLER_DEFINE(gamepad, SETTING_GAMEPAD_ROOT, NULL, gampepad_setting_handle_set, NULL, NULL);

int app_setting_init(void)
{
	//initialize default values for curves
	for(size_t slot=0; slot<GAMEPAD_TOTAL_CURVE_SLOT_NUM; slot++){
		for(size_t i=0; i<GAMEPAD_FEATURE_REPORT_CURVE_NUM_POINTS; i++){
			uint16_t val = (uint16_t)(UINT16_MAX * i / (GAMEPAD_FEATURE_REPORT_CURVE_NUM_POINTS-1));
			gp_curve_ctx.curve_slot[slot].accelerator[i] = val;
			gp_curve_ctx.curve_slot[slot].brake[i] = val;
			gp_curve_ctx.curve_slot[slot].clutch[i] = val;
		}
	}

	int rc = settings_subsys_init();
	if (rc) {
		LOG_ERR("settings subsys initialization: fail (err %d)", rc);
		return -EINVAL;
	}
	rc = settings_load();
	if (rc) {
		LOG_ERR("settings subsys load: fail (err %d)", rc);
		return -EINVAL;
	}
	LOG_DBG("settings subsys initialization: OK.");
	return 0;

}

const struct gamepad_calibration *get_calibration(void)
{
    return &gp_calibration;
}

int set_calibration(const struct gamepad_calibration *calib)
{
    int err = settings_save_one(SETTING_CALIBRATION, calib, SETTING_CALIBRATION_LEN);
    if(err) return err;
    return settings_load_subtree(SETTING_GAMEPAD_ROOT);
}

int set_active_curve_slot(uint8_t slot)
{
	if(slot > GAMEPAD_FEATURE_REPORT_CURVE_SLOT_NUM){
		return -EINVAL;
	}
	int err = settings_save_one(SETTING_CURVE_ACTIVE, &slot, sizeof(slot));
	if(err) return err;
	return settings_load_subtree(SETTING_GAMEPAD_ROOT);
}

int set_curve_slot(uint8_t slot_id, const struct gamepad_curve *curve)
{
	uint8_t slot = slot_id - GAMEPAD_FEATURE_REPORT_CURVE_SLOT_ID_BASE;
	if(slot == 0 || slot > GAMEPAD_FEATURE_REPORT_CURVE_SLOT_NUM){
		return -EINVAL;
	}
	char setting_name[64];
	snprintf(setting_name, sizeof(setting_name), SETTING_GAMEPAD_ROOT"/"SETTING_SUB_CURVE"/slot%d", slot);
	int err = settings_save_one(setting_name, curve, sizeof(struct gamepad_curve));
	if(err) return err;
	return settings_load_subtree(SETTING_GAMEPAD_ROOT);
}

const struct gamepad_curve* get_curve_slot(uint8_t slot_id)
{	
	uint8_t slot = slot_id - GAMEPAD_FEATURE_REPORT_CURVE_SLOT_ID_BASE;
	if(slot > GAMEPAD_FEATURE_REPORT_CURVE_SLOT_NUM){
		return NULL;
	}
	return &gp_curve_ctx.curve_slot[slot];
}

const struct gamepad_curve* get_active_curve_slot(void)
{
	return get_curve_slot(gp_curve_ctx.active_curve_slot + GAMEPAD_FEATURE_REPORT_CURVE_SLOT_ID_BASE);
}
