#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>

#include "common.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(led, CONFIG_APP_LOG_LEVEL);

static void gamepad_report_led_cb(const struct zbus_channel *chan)
{
	const struct gamepad_report_out *rpt = zbus_chan_const_msg(chan);

	LOG_INF("gamepad report: accelerator = %d, brake = %d, clutch = %d",rpt->accelerator, rpt->brake, rpt->clutch);

	//TODO: set LED PWM level according to gamepad report data
}

ZBUS_LISTENER_DEFINE(gp_rpt_led_handler, gamepad_report_led_cb);