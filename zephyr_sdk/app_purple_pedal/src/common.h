#ifndef APP_COMMON_H_
#define APP_COMMON_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/usb/usbd_msg.h>
#include "curve.h" // For CURVE_POINTS/CHANNELS

#define ADC_NODE_ID DT_ALIAS(pedal_adc)
#define ADC_CHANNEL_COUNT DT_CHILD_NUM(ADC_NODE_ID)
// NickR: Set sample period to 1kHZ instead of 100Hz
//#define ADC_SAMPLE_PERIOD K_MSEC(10)
#define ADC_SAMPLE_PERIOD K_USEC(1000)

#define CONFIG_SEQUENCE_SAMPLES (1)
#define ADC_NUM_BITS (24)
#define ADC_GAIN (128)
#define ADC_VAL_MAX (BIT_MASK(ADC_NUM_BITS))
#define ADC_VAL_MID (BIT(ADC_NUM_BITS-1))

#define LOAD_CELL_MV_V (1)
#define LOAD_CELL_DEFAULT_SCALE ((uint64_t)ADC_VAL_MID * ADC_GAIN * LOAD_CELL_MV_V / 1000ULL)
#define LOAD_CELL_DEFAULT_OFFSET (ADC_VAL_MID)
#define LOAD_CELL_DISCONNECT_THRESHOLD (ADC_VAL_MAX * 99 / 100)

// NickR: --- EMA FILTER VARIABLES ---
// Strength: 0 (No filter) to 255 (Max lag). 
// 50 is a good balance for sim racing (smooth but fast).
#define FILTER_ALPHA 50

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

#define GAMEPAD_INPUT_REPORT_ID (0x01)
#define GAMEPAD_FEATURE_REPORT_RAW_VAL_ID (0x02)
#define GAMEPAD_FEATURE_REPORT_CALIB_ID (0x03)
#define GAMEPAD_FEATURE_REPORT_UID_ID (0x04)
#define GAMEPAD_FEATURE_REPORT_UID_LENGTH (12) //12bytes UID length for STM32
// NickR: Set feature report 10 for curve calibration
#define GAMEPAD_FEATURE_REPORT_CURVE_ID  (0x0A)
// Preset Command IDs ---
#define GAMEPAD_FEATURE_REPORT_SAVE_PRESET_ID (0x0B) // ID 11
#define GAMEPAD_FEATURE_REPORT_LOAD_PRESET_ID (0x0C) // ID 12

// 3 Channels, 65 Points each
#define CURVE_POINTS 65
#define POINTS_PER_CHUNK 28
#define LAST_CHUNK_INDEX  ((CURVE_POINTS - 1) / POINTS_PER_CHUNK)
#define CURVE_CHANNELS 3

// The "Active" state in RAM matches these structs
typedef struct {
    uint32_t offset[3];
    uint32_t scale[3];
} struct_calibration_t;

typedef struct {
    uint16_t points[CURVE_CHANNELS][CURVE_POINTS];
} struct_curves_t;

enum app_state{
	APP_STATE_IDLE=0,
	APP_STATE_CONNECTED,
	APP_STATE_HID_WORKING,
	APP_STATE_DFU,
	APP_STATE_NUM,
};

enum gamepad_setting_index{
	SETTING_INDEX_CLUTCH = 0,
	SETTING_INDEX_BRAKE,
	SETTING_INDEX_ACCELERATOR,
	SETTING_INDEX_TOTAL,
};

// struct gamepad_calibration{
// 	int32_t offset[SETTING_INDEX_TOTAL];
// 	int32_t scale[SETTING_INDEX_TOTAL];
// }__packed;

struct gamepad_report_out{
	uint8_t report_id;
	uint16_t clutch;
    uint16_t brake;
	uint16_t accelerator;
}__packed;

struct gamepad_feature_rpt_raw_val{
	uint8_t report_id;
	uint32_t clutch_raw: 24; //need to define as uint32_t otherwise c compiler will process sign bit
	uint32_t brake_raw: 24;
	uint32_t accelerator_raw:24;
}__packed;

struct gamepad_feature_rpt_calib{
	uint8_t report_id;
	struct_calibration_t calib;
}__packed;

struct gamepad_feature_rpt_uid{
	uint8_t report_id;
	uint8_t uid[GAMEPAD_FEATURE_REPORT_UID_LENGTH];
}__packed;

// NickR: Curve calibration packet structure
// 64 Bytes total allowed
// 28 bytes per chunk
struct gamepad_feature_rpt_curve {
    uint8_t report_id;      // Must be 10 (0x0A)
    uint8_t channel_index;  // 0=Clu, 1=Brk, 2=Acc
    uint8_t chunk_index;    // 0=Start, 1=Middle, 2=End
    uint8_t point_count;    // How many points in this packet? (Max 28)
    uint16_t points[POINTS_PER_CHUNK];    // Payload
	uint8_t padding[32 - POINTS_PER_CHUNK];		// Padding for 64 bytes
} __packed;

// Preset data struct
struct gamepad_feature_rpt_preset_action {
    uint8_t report_id;
    uint8_t slot_id;   // 0, 1, 2...
} __packed;

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
int app_setting_init(void);

// The active RAM states
extern struct_calibration_t current_calib;
extern struct_curves_t current_curves;

// const struct gamepad_calibration *get_calibration(void);
// int set_calibration(const struct gamepad_calibration *calib);
//void gamepad_set_status_led(const enum app_state state);
void post_usb_event(struct usb_event event);

#ifdef __cplusplus
}
#endif
#endif /* APP_COMMON_H_ */