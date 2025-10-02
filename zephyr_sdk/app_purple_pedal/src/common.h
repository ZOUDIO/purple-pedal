#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define ADC_NODE_ID DT_ALIAS(pedal_adc)
#define ADC_CHANNEL_COUNT DT_CHILD_NUM(ADC_NODE_ID)
#define ADC_SAMPLE_PERIOD K_MSEC(10)

//below are arbitrary settings for nRF52840. STM32 might differ
#define CONFIG_SEQUENCE_SAMPLES (1)
#define ADC_NUM_BITS (12)

#define USB_DEVICE_CONTROLLER_ID DT_NODELABEL(zephyr_udc0)
#define HID_DEVICE_ID DT_NODELABEL(hid_dev_0)

#define PWM_LED_PEDAL_NODE_ID	 DT_NODELABEL(pwm_leds_pedal)
#define PWM_LED_RGB_NODE_ID	 DT_NODELABEL(pwm_leds_rgb)

enum app_adc_action {
	/** The sequence should be continued normally. */
	APP_ADC_ACTION_NONE = 0,
	APP_ADC_ACTION_START,
	APP_ADC_ACTION_STOP,
};

struct __packed gamepad_report_out{
    int16_t accelerator;
    int16_t brake;
    int16_t clutch;
};

#define GAMEPAD_REPORT_VALUE_MAX (INT16_MAX)
#define GAMEPAD_REPORT_VALUE_MIN (INT16_MIN)

typedef int (*gamepad_sample_ready_callback)(void);

int app_adc_init(void);
int app_usb_init(void);

#ifdef __cplusplus
}
#endif
