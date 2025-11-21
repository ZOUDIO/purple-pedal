#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/usb/usbd_msg.h>

#define ADC_NODE_ID DT_ALIAS(pedal_adc)
#define ADC_CHANNEL_COUNT DT_CHILD_NUM(ADC_NODE_ID)
#define ADC_SAMPLE_PERIOD K_MSEC(10)

//below are arbitrary settings for nRF52840. STM32 might differ
#define CONFIG_SEQUENCE_SAMPLES (1)
#define ADC_NUM_BITS (24)

#define USB_DEVICE_CONTROLLER_ID DT_NODELABEL(zephyr_udc0)
#define HID_DEVICE_ID DT_NODELABEL(hid_dev_0)

#define PWM_LED_PEDAL_NODE_ID	 DT_NODELABEL(pwm_leds_pedal)
#define PWM_LED_STATUS_NODE_ID	 DT_NODELABEL(pwm_leds_rgb)

// enum app_adc_action {
// 	/** The sequence should be continued normally. */
// 	APP_ADC_ACTION_NONE = 0,
// 	APP_ADC_ACTION_START,
// 	APP_ADC_ACTION_STOP,
// };

enum app_state{
	APP_STATE_IDLE=0,
	APP_STATE_CONNECTED,
	APP_STATE_HID_WORKING,
	APP_STATE_DFU,
	APP_STATE_NUM,
};

struct __packed gamepad_report_out{
    int16_t accelerator;
    int16_t brake;
    int16_t clutch;
};

// struct __packed app_version{
// 	uint8_t minor;
// 	uint8_t major;
// };

enum usb_event_type{
	EVENT_HID = 0,
	EVENT_USBD_MSG,
};

struct usb_event{
	enum usb_event_type type;
	union {
		bool hid_ready;
		enum usbd_msg_type usbd_msg;
	};
};

#define GAMEPAD_REPORT_VALUE_MAX (INT16_MAX)
#define GAMEPAD_REPORT_VALUE_MIN (INT16_MIN)

typedef int (*gamepad_sample_ready_callback)(void);

int app_adc_init(void);
int app_usb_init(void);

//void gamepad_set_status_led(const enum app_state state);
void post_usb_event(struct usb_event event);

#ifdef __cplusplus
}
#endif
