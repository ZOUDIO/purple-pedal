#include <zephyr/zbus/zbus.h>
#include <zephyr/logging/log.h>
#include "common.h"

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

int main(void)
{
    app_usb_init();
}
