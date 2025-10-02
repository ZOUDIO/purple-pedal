#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/drivers/led.h>
#include <zephyr/logging/log.h>

#include "common.h"

LOG_MODULE_REGISTER(led, CONFIG_APP_LOG_LEVEL);

//#define LED_PWM_NODE_ID	 DT_COMPAT_GET_ANY_STATUS_OKAY(pwm_leds)
static const struct device *led_pwm = DEVICE_DT_GET(PWM_LED_PEDAL_NODE_ID);

static void gamepad_set_led_brightness(const struct device *dev, uint32_t led, int32_t pedal_value)
{
	uint8_t brightness = (pedal_value - GAMEPAD_REPORT_VALUE_MIN) * 100 / (GAMEPAD_REPORT_VALUE_MAX - GAMEPAD_REPORT_VALUE_MIN);
	//LOG_DBG("LED: %u, brightness: %u", led, brightness);
	led_set_brightness(dev, led, brightness);
}

static void gamepad_report_led_cb(const struct zbus_channel *chan)
{
	const struct gamepad_report_out *rpt = zbus_chan_const_msg(chan);

	//LOG_INF("gamepad report: accelerator = %d, brake = %d, clutch = %d",rpt->accelerator, rpt->brake, rpt->clutch);
	
	//TODO: set LED PWM level according to gamepad report data
	gamepad_set_led_brightness(led_pwm, 0, rpt->accelerator);
	gamepad_set_led_brightness(led_pwm, 1, rpt->brake);
	gamepad_set_led_brightness(led_pwm, 2, rpt->clutch);
}

ZBUS_LISTENER_DEFINE(gp_rpt_led_handler, gamepad_report_led_cb);

