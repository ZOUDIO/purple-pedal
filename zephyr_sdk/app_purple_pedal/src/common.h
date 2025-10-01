#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

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
