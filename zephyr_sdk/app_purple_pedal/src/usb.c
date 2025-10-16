#include <sample_usbd.h>

#include <zephyr/kernel.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/class/usbd_hid.h>
#include <zephyr/usb/class/usbd_dfu.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/sys/reboot.h>

#include "common.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(usb, CONFIG_APP_LOG_LEVEL);

/**
 * @brief Simple HID Gamepad report descriptor.
 */
#define HID_USAGE_SIM_CTRL		0x02

#define HID_GAMEPAD_REPORT_DESC() {				\
	HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),		\
	HID_USAGE(HID_USAGE_GEN_DESKTOP_GAMEPAD),	\
	HID_COLLECTION(HID_COLLECTION_APPLICATION),	\
		HID_USAGE_PAGE(HID_USAGE_SIM_CTRL),	\
		/* Axis Usage of Accelerator, Brake, Clutch */				\
		HID_USAGE(0xC4), \
		HID_USAGE(0xC5), \
		HID_USAGE(0xC6), \
		HID_LOGICAL_MIN16(0x00, 0x80),	\
		HID_LOGICAL_MAX16(0xff, 0x7f),	\
		HID_REPORT_SIZE(16),	\
		HID_REPORT_COUNT(3),	\
		/*   Input (Data,Var,Abs)*/\
		HID_INPUT(0x02),		\
	HID_END_COLLECTION,			\
}
//TODO: add usage page LED and LED outputs. see HUT doc section 11 LED Page.

#define GAMEPAD_REPORT_OUT_LEN sizeof(struct gamepad_report_out)

//static const struct device *hid_dev = DEVICE_DT_GET_ONE(zephyr_hid_device);
static const struct device *hid_dev = DEVICE_DT_GET(HID_DEVICE_ID);

static const uint8_t hid_report_desc[] = HID_GAMEPAD_REPORT_DESC();

//USBD_DEVICE_DEFINE(dfu_usbd, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)), 0x2fe3, 0xffff);
USBD_DEVICE_DEFINE(dfu_usbd, DEVICE_DT_GET(USB_DEVICE_CONTROLLER_ID), 0x2fe3, 0xffff);
USBD_DESC_LANG_DEFINE(sample_lang);
USBD_DESC_CONFIG_DEFINE(fs_cfg_desc, "DFU FS Configuration");
USBD_DESC_CONFIG_DEFINE(hs_cfg_desc, "DFU HS Configuration");

static const uint8_t attributes = (IS_ENABLED(CONFIG_SAMPLE_USBD_SELF_POWERED) ?
				   USB_SCD_SELF_POWERED : 0) |
				  (IS_ENABLED(CONFIG_SAMPLE_USBD_REMOTE_WAKEUP) ?
				   USB_SCD_REMOTE_WAKEUP : 0);
/* Full speed configuration */
USBD_CONFIGURATION_DEFINE(sample_fs_config,attributes,CONFIG_SAMPLE_USBD_MAX_POWER, &fs_cfg_desc);
/* High speed configuration */
USBD_CONFIGURATION_DEFINE(sample_hs_config,attributes,CONFIG_SAMPLE_USBD_MAX_POWER, &hs_cfg_desc);

/* semaphore used to track available buf in USB HID */
static K_SEM_DEFINE(sem_hid_in_buf, CONFIG_USBD_HID_IN_BUF_COUNT, CONFIG_USBD_HID_IN_BUF_COUNT);

static void gamepad_iface_ready(const struct device *dev, const bool ready)
{
	//here we turn adc front-end on and off
	LOG_DBG("iface status: %d", ready);
	int err;
	const enum app_state state = ready ? APP_STATE_HID_WORKING : APP_STATE_CONNECTED;
	if(state == APP_STATE_HID_WORKING){
		err = k_sem_init(&sem_hid_in_buf, CONFIG_USBD_HID_IN_BUF_COUNT, CONFIG_USBD_HID_IN_BUF_COUNT);
		if(err){
			LOG_ERR("k_sem_init() returns %d", err);
		}
	}
	struct usb_event event = {.type = EVENT_HID, .hid_ready = ready};
	post_usb_event(event);
}

static int gamepad_get_report(const struct device *dev,const uint8_t type, const uint8_t id, const uint16_t len,uint8_t *const buf)
{
	LOG_WRN("Get Report not implemented, Type %u ID %u", type, id);
	return 0;
}

static void gamepad_input_report_done(const struct device *dev, const uint8_t *const report)
{
	//LOG_WRN("gamepad_input_report_done() implemented");
	//give semaphore here
	//LOG_DBG("gamepad_input_report_done() done.");
	k_sem_give(&sem_hid_in_buf);
}

static int gamepad_set_report(const struct device *dev, const uint8_t type, const uint8_t id, const uint16_t len, const uint8_t *const buf)
{
	LOG_DBG("called");
	return 0;
}

static void gamepad_set_idle(const struct device *dev, const uint8_t id, const uint32_t duration)
{
	LOG_DBG("called with duration: %u", duration);
}

static uint32_t gamepad_get_idle(const struct device *dev, const uint8_t id)
{
	LOG_INF("Get Idle %u to %u", id, 1000);
	return 1000;
}

static void gamepad_set_protocol(const struct device *dev, const uint8_t proto)
{
	LOG_INF("Protocol changed to %s", proto == 0U ? "Boot Protocol" : "Report Protocol");
}

static void gamepad_output_report(const struct device *dev, const uint16_t len, const uint8_t *const buf)
{
	LOG_HEXDUMP_DBG(buf, len, "o.r.");
	//kb_set_report(dev, HID_REPORT_TYPE_OUTPUT, 0U, len, buf);
}

struct hid_device_ops gampepad_ops = {
	.iface_ready = gamepad_iface_ready,
	.input_report_done = gamepad_input_report_done,
	.get_report = gamepad_get_report,
	.set_report = gamepad_set_report,
	.set_idle = gamepad_set_idle,
	.get_idle = gamepad_get_idle,
	.set_protocol = gamepad_set_protocol,
	.output_report = gamepad_output_report,
};

static void switch_to_dfu_mode(struct usbd_context *const ctx);

static void msg_cb(struct usbd_context *const usbd_ctx, const struct usbd_msg *const msg)
{
	LOG_INF("USBD message: %s", usbd_msg_type_string(msg->type));

	if (msg->type == USBD_MSG_CONFIGURATION) {
		LOG_INF("\tConfiguration value %d", msg->status);
	}
	if(msg->type == USBD_MSG_SUSPEND){
		struct usb_event event = {.type = EVENT_USBD_MSG, .usbd_msg = USBD_MSG_SUSPEND};
		post_usb_event(event);
	}
	if (usbd_can_detect_vbus(usbd_ctx)) {
		if (msg->type == USBD_MSG_VBUS_READY) {
			if (usbd_enable(usbd_ctx)) {
				LOG_ERR("Failed to enable device support");
			}
			struct usb_event event = {.type = EVENT_USBD_MSG, .usbd_msg = USBD_MSG_VBUS_READY};
			post_usb_event(event);
		}

		if (msg->type == USBD_MSG_VBUS_REMOVED) {
			if (usbd_disable(usbd_ctx)) {
				LOG_ERR("Failed to disable device support");
			}
			struct usb_event event = {.type = EVENT_USBD_MSG, .usbd_msg = USBD_MSG_VBUS_REMOVED};
			post_usb_event(event);
		}
	}

	if (msg->type == USBD_MSG_DFU_APP_DETACH) {
		switch_to_dfu_mode(usbd_ctx);
		//here publish new status to stop the ADC, set LED mode
		struct usb_event event = {.type = EVENT_USBD_MSG, .usbd_msg = USBD_MSG_DFU_APP_DETACH};
		post_usb_event(event);
	}

	if (msg->type == USBD_MSG_DFU_DOWNLOAD_COMPLETED) {
		if (IS_ENABLED(CONFIG_BOOTLOADER_MCUBOOT) && IS_ENABLED(CONFIG_USBD_DFU_FLASH)) {
			LOG_DBG("DFU download complete, reboot system...");
			k_msleep(5000);
			boot_request_upgrade(true); //true/false: whether the image should be used permanently or only tested once.
			//here trigger a system reset
			sys_reboot(SYS_REBOOT_WARM);
		}
	}
}

static void switch_to_dfu_mode(struct usbd_context *const ctx)
{
	int err;

	LOG_INF("Detach USB device");
	usbd_disable(ctx);
	usbd_shutdown(ctx);

	err = usbd_add_descriptor(&dfu_usbd, &sample_lang);
	if (err) {
		LOG_ERR("Failed to initialize language descriptor (%d)", err);
		return;
	}

	if (usbd_caps_speed(&dfu_usbd) == USBD_SPEED_HS) {
		err = usbd_add_configuration(&dfu_usbd, USBD_SPEED_HS, &sample_hs_config);
		if (err) {
			LOG_ERR("Failed to add High-Speed configuration");
			return;
		}

		err = usbd_register_class(&dfu_usbd, "dfu_dfu", USBD_SPEED_HS, 1);
		if (err) {
			LOG_ERR("Failed to add register classes");
			return;
		}

		usbd_device_set_code_triple(&dfu_usbd, USBD_SPEED_HS, 0, 0, 0);
	}

	err = usbd_add_configuration(&dfu_usbd, USBD_SPEED_FS, &sample_fs_config);
	if (err) {
		LOG_ERR("Failed to add Full-Speed configuration");
		return;
	}

	err = usbd_register_class(&dfu_usbd, "dfu_dfu", USBD_SPEED_FS, 1);
	if (err) {
		LOG_ERR("Failed to add register classes");
		return;
	}

	usbd_device_set_code_triple(&dfu_usbd, USBD_SPEED_FS, 0, 0, 0);

	err = usbd_init(&dfu_usbd);
	if (err) {
		LOG_ERR("Failed to initialize USB device support");
		return;
	}

	err = usbd_msg_register_cb(&dfu_usbd, msg_cb);
	if (err) {
		LOG_ERR("Failed to register message callback");
		return;
	}

	err = usbd_enable(&dfu_usbd);
	if (err) {
		LOG_ERR("Failed to enable USB device support");
	}
}

static void gamepad_report_usb_cb(const struct zbus_channel *chan)
{
	const struct gamepad_report_out *rpt = zbus_chan_const_msg(chan);
	//LOG_INF("gamepad report: accelerator = %d, brake = %d, clutch = %d",rpt->accelerator, rpt->brake, rpt->clutch);
	int err = k_sem_take(&sem_hid_in_buf, K_NO_WAIT);
	if(err){
		LOG_ERR("k_sem_take() returns %d. Report not submitted", err);
		return;
	}
	//this semaphore helps to avoid calling hid_device_submit_report() too often.
	//so less likely to crash under Linux when hid is not opened
	//and hid_device_submit_report() always fails.
	err = hid_device_submit_report(hid_dev, sizeof(struct gamepad_report_out), (const uint8_t *const)rpt);
	if(err){
		LOG_ERR("hid_device_submit_report() returns %d", err);
		k_sem_give(&sem_hid_in_buf);
	}
	// else{
	// 	LOG_DBG("hid_device_submit_report() done.");
	// }

}

ZBUS_LISTENER_DEFINE(gp_rpt_usb_handler, gamepad_report_usb_cb);

int app_usb_init(void)
{
	struct usbd_context *sample_usbd;
	//const struct device *hid_dev;
	int ret;

	//hid_dev = DEVICE_DT_GET_ONE(zephyr_hid_device);
	if (!device_is_ready(hid_dev)) {
		LOG_ERR("HID Device is not ready");
		return -EIO;
	}

	ret = hid_device_register(hid_dev,hid_report_desc, sizeof(hid_report_desc),&gampepad_ops);
	if (ret != 0) {
		LOG_ERR("Failed to register HID Device, %d", ret);
		return ret;
	}

	sample_usbd = sample_usbd_init_device(msg_cb);
	if (sample_usbd == NULL) {
		LOG_ERR("Failed to initialize USB device");
		return -ENODEV;
	}

	if (!usbd_can_detect_vbus(sample_usbd)) {
		ret = usbd_enable(sample_usbd);
		if (ret){
			LOG_ERR("Failed to enable device support");
			return ret;
		}
	}
	
	LOG_INF("USB DFU sample is initialized");

	return 0;
}

#if CONFIG_SHELL

//test commands

#endif