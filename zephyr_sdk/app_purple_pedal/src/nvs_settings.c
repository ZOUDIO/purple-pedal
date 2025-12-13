#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#include "common.h"
LOG_MODULE_REGISTER(nvs_settings, CONFIG_APP_LOG_LEVEL);

#define SETTING_GAMEPAD_ROOT "gamepad"

#define SETTING_SUB_CALIBRATION "calib"

#define SETTING_CALIBRATION SETTING_GAMEPAD_ROOT"/"SETTING_SUB_CALIBRATION

#define SETTING_CALIBRATION_LEN (sizeof(struct gamepad_calibration))

static struct gamepad_calibration gp_calibration = {
    .offset = {1,2,3},
    .scale = {5,6,7},
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