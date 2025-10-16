#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/drivers/led.h>
#include <zephyr/logging/log.h>

#include "common.h"

LOG_MODULE_REGISTER(led, CONFIG_APP_LOG_LEVEL);

//#define LED_PWM_NODE_ID	 DT_COMPAT_GET_ANY_STATUS_OKAY(pwm_leds)
static const struct device *led_pwm_pedal = DEVICE_DT_GET(PWM_LED_PEDAL_NODE_ID);

static void gamepad_set_led_brightness(const struct device *dev, uint32_t led, int32_t pedal_value)
{
	uint8_t brightness = (pedal_value - GAMEPAD_REPORT_VALUE_MIN) * 100 / (GAMEPAD_REPORT_VALUE_MAX - GAMEPAD_REPORT_VALUE_MIN);
	//LOG_DBG("LED: %u, brightness: %u", led, brightness);
	int err = led_set_brightness(dev, led, brightness);
	if(err){
		LOG_ERR("led_set_brightness() returns %d", err);
	}
}

static void gamepad_report_led_cb(const struct zbus_channel *chan)
{
	const struct gamepad_report_out *rpt = zbus_chan_const_msg(chan);
	//LOG_INF("gamepad report: accelerator = %d, brake = %d, clutch = %d",rpt->accelerator, rpt->brake, rpt->clutch);
	//set LED PWM level according to gamepad report data
	gamepad_set_led_brightness(led_pwm_pedal, 0, rpt->accelerator);
	gamepad_set_led_brightness(led_pwm_pedal, 1, rpt->brake);
	gamepad_set_led_brightness(led_pwm_pedal, 2, rpt->clutch);
}
ZBUS_LISTENER_DEFINE(gp_rpt_led_handler, gamepad_report_led_cb);

static const struct device *led_pwm_status = DEVICE_DT_GET(PWM_LED_STATUS_NODE_ID);

#define NUM_STATUS_LED (3)
struct status_led_pattern{
	//uint32_t led[NUM_STATUS_LED];
	uint32_t delay_on[NUM_STATUS_LED]; //Time period (in milliseconds) an LED should be ON
	uint32_t delay_off[NUM_STATUS_LED];
};

const struct status_led_pattern patterns[APP_STATE_NUM] = {
	[APP_STATE_IDLE]	= {.delay_on = {125, 125, 125}, .delay_off = {125, 125, 125}},
	[APP_STATE_CONNECTED] 		= {.delay_on = {125, 125, 125}, .delay_off = {125, 125, 125}},
	[APP_STATE_HID_WORKING] 	= {.delay_on = {125, 125, 125}, .delay_off = {125, 125, 125}},
	[APP_STATE_DFU] 			= {.delay_on = {125, 125, 125}, .delay_off = {125, 125, 125}},
};

static void gamepad_status_led_cb(const struct zbus_channel *chan)
{
	int err;
	const enum app_state *state = zbus_chan_const_msg(chan);
	LOG_DBG("set status LED to status %d", *state);

	for(uint32_t i=0; i<NUM_STATUS_LED; i++){
		err = led_blink(led_pwm_status, i, patterns[*state].delay_on[i], patterns[*state].delay_off[i]);
		if(err){
			LOG_ERR("led_blink() returns %d", err);
		}
	}
}
ZBUS_LISTENER_DEFINE(gp_status_led_handler, gamepad_status_led_cb);

// void gamepad_set_status_led(const enum app_state state)
// {
// 	int err;
// 	LOG_DBG("set status LED to status %d", state);

// 	for(uint32_t i=0; i<NUM_STATUS_LED; i++){
// 		err = led_blink(led_pwm_status, i, patterns[state].delay_on[i], patterns[state].delay_off[i]);
// 		if(err){
// 			LOG_ERR("led_blink() returns %d", err);
// 		}
// 	}
// }