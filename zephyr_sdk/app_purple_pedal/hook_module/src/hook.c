/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdio.h>
#include <bootutil/mcuboot_status.h>

#include <zephyr/kernel.h>
// #include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hook, CONFIG_MCUBOOT_LOG_LEVEL);

// #define LED0_NODE DT_ALIAS(led3)
// static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
//#define PWM_LED_STATUS_NODE_ID	 DT_NODELABEL(pwmleds)

//TODO: use PWM LED will make shell UART slow and missing character on NRF52. So prefer to use GPIO LED.
//#define PWM_LED_STATUS_NODE_ID	 DT_INST(0, pwm_leds) //this applies for nRF52840dk
#define PWM_LED_STATUS_NODE_ID	 DT_INST(0, gpio_leds) //this applies for nRF52840dk
#define NUM_STATUS_LED DT_CHILD_NUM(PWM_LED_STATUS_NODE_ID)
static const struct device *led_pwm_status = DEVICE_DT_GET(PWM_LED_STATUS_NODE_ID);

struct status_led_pattern{
	//uint32_t led[NUM_STATUS_LED];
	uint32_t delay_on[NUM_STATUS_LED]; //Time period (in milliseconds) an LED should be ON
	uint32_t delay_off[NUM_STATUS_LED];
};

const struct status_led_pattern pattern = {.delay_on = {125}, .delay_off = {125}};

void mcuboot_status_change(mcuboot_status_type_t status)
{
	//printk("mcuboot_status: %d\n", status);
    LOG_ERR("mcuboot_status: %d\n", status);
	int err;
	for(uint32_t i=0; i<NUM_STATUS_LED; i++){
		err = led_blink(led_pwm_status, i, pattern.delay_on[i], pattern.delay_off[i]);
		if(err){
			LOG_ERR("led_blink() returns %d", err);
		}
	}
}