/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdio.h>
#include <bootutil/mcuboot_status.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hook, CONFIG_MCUBOOT_LOG_LEVEL);

#define LED0_NODE DT_ALIAS(led3)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

void mcuboot_status_change(mcuboot_status_type_t status)
{
	printk("mcuboot_status: %d\n", status);
    LOG_ERR("mcuboot_status: %d\n", status);

    // int ret;
	// bool led_state = true;
	// if (!gpio_is_ready_dt(&led)) {
	// 	return;
	// }
	// ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	// if (ret < 0) {
	// 	return;
	// }
    // gpio_pin_set_dt(&led, 1);
    // k_sleep(K_FOREVER);
}