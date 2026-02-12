#include <zephyr/zbus/zbus.h>
#include <zephyr/smf.h>
#include <zephyr/logging/log.h>
#include "common.h"
#include "app_version.h"
// Preset
#include "preset.h" 
#include "curve.h"

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

// This is the active live RAM configuration
struct_calibration_t current_calib;
struct_curves_t      current_curves;

ZBUS_CHAN_DEFINE(gamepad_report_out_chan,  /* Name */
		 struct gamepad_report_out, /* Message type */
		 NULL, /* Validator */
		 NULL, /* User data */
		 ZBUS_OBSERVERS(gp_rpt_usb_handler, gp_rpt_led_handler), /* observers */
		 ZBUS_MSG_INIT(.report_id=GAMEPAD_INPUT_REPORT_ID, .clutch=0, .brake=0, .accelerator=0)		      /* Initial value {0} */
);

ZBUS_CHAN_DEFINE(gamepad_feature_report_raw_val_chan,  /* Name */
		 struct gamepad_feature_rpt_raw_val, /* Message type */
		 NULL, /* Validator */
		 NULL, /* User data */
		 ZBUS_OBSERVERS_EMPTY, /* observers */
		 ZBUS_MSG_INIT(.report_id=GAMEPAD_FEATURE_REPORT_RAW_VAL_ID, .clutch_raw=0, .brake_raw=0, .accelerator_raw=0)		      /* Initial value {0} */
);

ZBUS_CHAN_DEFINE(gamepad_status_chan,  /* Name */
		 enum app_state, /* Message type */
		 NULL, /* Validator */
		 NULL, /* User data */
		 ZBUS_OBSERVERS(gp_adc_ctrl_handler, gp_status_led_handler), /* observers */
		 ZBUS_MSG_INIT(APP_STATE_IDLE)		      /* Initial value {0} */
);

//forward declaration
static const struct smf_state usb_sm_states[];

struct usb_state_machine{
	struct smf_ctx sm_ctx;
	const struct smf_state *sm_states;
	struct k_msgq* usb_msgq;
	struct usb_event event;
};

/* USB state machine */
K_MSGQ_DEFINE(usb_sm_msgq, sizeof(struct usb_event), 10, 1); 
static struct usb_state_machine usb_sm = {.usb_msgq = &usb_sm_msgq, .sm_states = usb_sm_states};

// Memory flag for disconnect
volatile uint8_t pedal_connected_flags = 0x07;

static void idle_entry(void *o)
{
	//gamepad_set_status_led(APP_STATE_IDLE);
	LOG_DBG("idle_entry()");
	const enum app_state state = APP_STATE_IDLE;
	int err = zbus_chan_pub(&gamepad_status_chan, &state, K_MSEC(2));
	if (err < 0){
		LOG_ERR("zbus_chan_pub() error (%d)\n", err);
	}	
}

static enum smf_state_result idle_run(void *o)
{
	struct usb_state_machine *usb_sm = CONTAINER_OF(o, struct usb_state_machine, sm_ctx);
	if(usb_sm->event.type == EVENT_HID){
		if(usb_sm->event.hid_ready){
			smf_set_state(&usb_sm->sm_ctx, &usb_sm->sm_states[APP_STATE_HID_WORKING]);
		}
	}
	else if(usb_sm->event.type == EVENT_USBD_MSG){
		if(usb_sm->event.usbd_msg == USBD_MSG_VBUS_READY){
			smf_set_state(&usb_sm->sm_ctx, &usb_sm->sm_states[APP_STATE_CONNECTED]);
		}
	}
	else{
		LOG_ERR("unrecognized usb_event type %d", usb_sm->event.type);
	}
	return SMF_EVENT_HANDLED;
}

static void connected_entry(void *o)
{
	//gamepad_set_status_led(APP_STATE_CONNECTED);
	LOG_DBG("connected_entry()");
	const enum app_state state = APP_STATE_CONNECTED;
	int err = zbus_chan_pub(&gamepad_status_chan, &state, K_MSEC(2));
	if (err < 0){
		LOG_ERR("zbus_chan_pub() error (%d)\n", err);
	}	
}

static enum smf_state_result connected_run(void *o)
{
	struct usb_state_machine *usb_sm = CONTAINER_OF(o, struct usb_state_machine, sm_ctx);

	if(usb_sm->event.type == EVENT_HID){
		if(usb_sm->event.hid_ready){
			smf_set_state(&usb_sm->sm_ctx, &usb_sm->sm_states[APP_STATE_HID_WORKING]);
		}
	}
	else if(usb_sm->event.type == EVENT_USBD_MSG){
		if(usb_sm->event.usbd_msg == USBD_MSG_VBUS_REMOVED){
			smf_set_state(&usb_sm->sm_ctx, &usb_sm->sm_states[APP_STATE_IDLE]);
		}
		else if(usb_sm->event.usbd_msg == USBD_MSG_DFU_APP_DETACH){
			smf_set_state(&usb_sm->sm_ctx, &usb_sm->sm_states[APP_STATE_DFU]);
		}
	}
	else{
		LOG_ERR("unrecognized usb_event type %d", usb_sm->event.type);
	}
	return SMF_EVENT_HANDLED;
}

static void hid_working_entry(void *o)
{
	LOG_DBG("hid_working_entry()");
	const enum app_state state = APP_STATE_HID_WORKING;
	int err = zbus_chan_pub(&gamepad_status_chan, &state, K_MSEC(2));
	if (err < 0){
		LOG_ERR("zbus_chan_pub() error (%d)\n", err);
	}	
}

static enum smf_state_result hid_working_run(void *o)
{
	struct usb_state_machine *usb_sm = CONTAINER_OF(o, struct usb_state_machine, sm_ctx);

	if(usb_sm->event.type == EVENT_HID){
		if(!usb_sm->event.hid_ready){
			smf_set_state(&usb_sm->sm_ctx, &usb_sm->sm_states[APP_STATE_CONNECTED]);
		}
	}
	else if(usb_sm->event.type == EVENT_USBD_MSG){
		if(usb_sm->event.usbd_msg == USBD_MSG_DFU_APP_DETACH){
			smf_set_state(&usb_sm->sm_ctx, &usb_sm->sm_states[APP_STATE_DFU]);
		}
	}
	else{
		LOG_ERR("unrecognized usb_event type %d", usb_sm->event.type);
	}
	return SMF_EVENT_HANDLED;
}

static void dfu_entry(void *o)
{
	LOG_DBG("dfu_entry()");
	const enum app_state state = APP_STATE_DFU;
	int err = zbus_chan_pub(&gamepad_status_chan, &state, K_MSEC(2));
	if (err < 0){
		LOG_ERR("zbus_chan_pub() error (%d)\n", err);
	}	
}

static enum smf_state_result dfu_run(void *o)
{
	struct usb_state_machine *usb_sm = CONTAINER_OF(o, struct usb_state_machine, sm_ctx);

	if(usb_sm->event.type == EVENT_HID){

	}
	else if(usb_sm->event.type == EVENT_USBD_MSG){

	}
	else{
		LOG_ERR("unrecognized usb_event type %d", usb_sm->event.type);
	}
	return SMF_EVENT_HANDLED;
}

static const struct smf_state usb_sm_states[] = {
        [APP_STATE_IDLE] = SMF_CREATE_STATE(idle_entry, idle_run, NULL, NULL, NULL),
        [APP_STATE_CONNECTED] = SMF_CREATE_STATE(connected_entry, connected_run, NULL, NULL, NULL),
        [APP_STATE_HID_WORKING] = SMF_CREATE_STATE(hid_working_entry, hid_working_run, NULL, NULL, NULL),
        [APP_STATE_DFU] = SMF_CREATE_STATE(dfu_entry, dfu_run, NULL, NULL, NULL),
};

void post_usb_event(struct usb_event event)
{
	//TODO: if this function is called by two events, what will happen?
	//a loop should be designed to block reading from message queue and run the state machine
	//state machine code is getting complex, 
	//see zephyr official example:
	//https://github.com/zephyrproject-rtos/zephyr/blob/87a6d5b77d7eeaea29d3e3eafe346a5879c79d4c/samples/subsys/smf/hsm_psicc2/src/hsm_psicc2_thread.c#L316
	int err = k_msgq_put(usb_sm.usb_msgq, &event, K_FOREVER);
	if(err){
		LOG_ERR("k_msgq_put() returns %d", err);
	}
}

static void set_factory_defaults(void)
{
    LOG_INF("Storage empty. Generating Linear Defaults.");

    // Set Calibration Defaults
    for(int i=0; i<3; i++) {
        current_calib.offset[i] = LOAD_CELL_DEFAULT_OFFSET;
        current_calib.scale[i]  = LOAD_CELL_DEFAULT_SCALE;
    }

    // Set Curve Defaults (Linear Ramp)
    // Point 0 = 0, Point 64 = 65535.
    // Each step is 1024 (65535 / 64 approx).
    for (int ch = 0; ch < CURVE_CHANNELS; ch++) {
        for (int i = 0; i < CURVE_POINTS; i++) {
            uint32_t val = i * 1024;
            
            // Clamp the last point to exactly 65535 (since 64*1024 = 65536)
            if (val > 65535) val = 65535;
            
            current_curves.points[ch][i] = (uint16_t)val;
        }
    }
    
    // Save these defaults so next boot is faster
    storage_save_active(&current_calib, &current_curves);
}

int main(void)
{
	int err;

	LOG_INF("PurplePedal App Version: v%u.%u.%u", APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_PATCHLEVEL);


	// Load active preset into RAM
	storage_init();
	// Try to load. If it fails (returns non-zero), create defaults.
    if (storage_load_active(&current_calib, &current_curves) != 0) {
        set_factory_defaults();
    }

	app_adc_init();
    app_usb_init();

	// //init state machine and run the state machine event loop here
	smf_set_initial(&usb_sm.sm_ctx, &usb_sm_states[APP_STATE_IDLE]);
	while(1){
		//k_msleep(1000);
		err = k_msgq_get(usb_sm.usb_msgq, &usb_sm.event, K_FOREVER);
		if(err == 0){
			err = smf_run_state(&usb_sm.sm_ctx);
			if(err){
				LOG_INF("%s terminating state machine thread", __func__);
				break;
			}
		}
		else{
			LOG_ERR("k_msgq_get() returns %d", err);
		}
	}
	return 0;
}
