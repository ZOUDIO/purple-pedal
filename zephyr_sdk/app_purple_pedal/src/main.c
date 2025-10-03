#include <zephyr/zbus/zbus.h>
#include <zephyr/logging/log.h>
#include "common.h"
#include "app_version.h"

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

ZBUS_CHAN_DEFINE(gamepad_report_out_chan,  /* Name */
		 struct gamepad_report_out, /* Message type */
		 NULL, /* Validator */
		 NULL, /* User data */
		 ZBUS_OBSERVERS(gp_rpt_usb_handler, gp_rpt_led_handler), /* observers */
		 ZBUS_MSG_INIT(0)		      /* Initial value {0} */
);

ZBUS_CHAN_DEFINE(gamepad_adc_ctrl_chan,  /* Name */
		 enum app_adc_action, /* Message type */
		 NULL, /* Validator */
		 NULL, /* User data */
		 ZBUS_OBSERVERS(gp_adc_ctrl_handler), /* observers */
		 ZBUS_MSG_INIT(APP_ADC_ACTION_NONE)		      /* Initial value {0} */
);

// static const struct app_version version = {
// 	.minor = CONFIG_APP_VERSION_MINOR,
// 	.major = CONFIG_APP_VERSION_MAJOR,
// };


int main(void)
{
	LOG_INF("PurplePedal App Version: %u.%u", APP_VERSION_MAJOR, APP_VERSION_MINOR);
	app_adc_init();
    app_usb_init();
}
