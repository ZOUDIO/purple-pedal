
#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>

#include "common.h"

#define ADC_NODE DT_ALIAS(pedal_adc)
static const struct device *adc = DEVICE_DT_GET(ADC_NODE);
static const struct adc_channel_cfg channel_cfgs[] = {
	DT_FOREACH_CHILD_SEP(ADC_NODE, ADC_CHANNEL_CFG_DT, (,))};

int app_adc_init(void)
{

}

int app_adc_start(gamepad_sample_ready_callback callback)
{

}

int ap_adc_stop(void)
{

}