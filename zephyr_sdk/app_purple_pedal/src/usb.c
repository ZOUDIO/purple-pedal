#include <sample_usbd.h>

#include <zephyr/kernel.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/class/usbd_hid.h>
#include <zephyr/usb/class/usbd_dfu.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/zbus/zbus.h>

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

static const struct device *hid_dev = DEVICE_DT_GET_ONE(zephyr_hid_device);

static const uint8_t hid_report_desc[] = HID_GAMEPAD_REPORT_DESC();

USBD_DEVICE_DEFINE(dfu_usbd, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)), 0x2fe3, 0xffff);
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

ZBUS_CHAN_DECLARE(gamepad_adc_ctrl_chan);
static void gamepad_iface_ready(const struct device *dev, const bool ready)
{
	//here we turn adc front-end on and off
	LOG_DBG("iface status: %d", ready);
	int err;
	const enum app_adc_action action = ready ? APP_ADC_ACTION_START : APP_ADC_ACTION_STOP;
	err = zbus_chan_pub(&gamepad_adc_ctrl_chan, &action, K_MSEC(10));
	if(err){
		LOG_ERR("zbus_chan_pub() returns %d", err);
	}
}

static int gamepad_get_report(const struct device *dev,const uint8_t type, const uint8_t id, const uint16_t len,uint8_t *const buf)
{
	LOG_WRN("Get Report not implemented, Type %u ID %u", type, id);
	return 0;
}

static void gamepad_input_report_done(const struct device *dev, const uint8_t *const report)
{
	LOG_WRN("gamepad_input_report_done() implemented");
}

struct hid_device_ops gampepad_ops = {
	.iface_ready = gamepad_iface_ready,
	.input_report_done = gamepad_input_report_done,
	.get_report = gamepad_get_report,
	.set_report = NULL,
	.set_idle = NULL,
	.get_idle = NULL,
	.set_protocol = NULL,
	.output_report = NULL,
};

static void switch_to_dfu_mode(struct usbd_context *const ctx);

static void msg_cb(struct usbd_context *const usbd_ctx, const struct usbd_msg *const msg)
{
	LOG_INF("USBD message: %s", usbd_msg_type_string(msg->type));

	if (msg->type == USBD_MSG_CONFIGURATION) {
		LOG_INF("\tConfiguration value %d", msg->status);
	}

	if (usbd_can_detect_vbus(usbd_ctx)) {
		if (msg->type == USBD_MSG_VBUS_READY) {
			if (usbd_enable(usbd_ctx)) {
				LOG_ERR("Failed to enable device support");
			}
		}

		if (msg->type == USBD_MSG_VBUS_REMOVED) {
			if (usbd_disable(usbd_ctx)) {
				LOG_ERR("Failed to disable device support");
			}
		}
	}

	if (msg->type == USBD_MSG_DFU_APP_DETACH) {
		switch_to_dfu_mode(usbd_ctx);
	}

	if (msg->type == USBD_MSG_DFU_DOWNLOAD_COMPLETED) {
		if (IS_ENABLED(CONFIG_BOOTLOADER_MCUBOOT) &&
		    IS_ENABLED(CONFIG_APP_USB_DFU_USE_FLASH_BACKEND)) {
			boot_request_upgrade(false);
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

	//TODO: send out the gamepad report
	int err = hid_device_submit_report(hid_dev, sizeof(struct gamepad_report_out), (const uint8_t *const)rpt);
	if(err){
		LOG_ERR("hid_device_submit_report() returns %d", err);
	}

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
	// ret = usbd_enable(sample_usbd);
	// if (ret){
	// 	LOG_ERR("Failed to enable device support");
	// 	return ret;
	// }
	LOG_INF("USB DFU sample is initialized");

	return 0;
}

#if CONFIG_SHELL

//test commands

#endif