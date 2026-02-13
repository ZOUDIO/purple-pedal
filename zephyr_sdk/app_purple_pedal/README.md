# 1. Purple Pedal Application

## 1.1 Build the application
```shell
west build -b purple_pedal --sysbuild zephyr_sdk/app_purple_pedal

west build -b purple_pedal --sysbuild zephyr_sdk/app_purple_pedal -- -DEXTRA_CONF_FILE="overlay-shell.conf"

west build -b nrf52840dk/nrf52840 --sysbuild zephyr_sdk/app_purple_pedal

west build -b nrf52840dk/nrf52840 --sysbuild zephyr_sdk/app_purple_pedal -- -DEXTRA_CONF_FILE="overlay-shell.conf"

```

There are 2 possible options for firmware update:

1. update via USB CDC ACM: https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/subsys/mgmt/mcumgr/smp_svr

2. update via USB DFU: https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/subsys/usb/dfu


## 1.2 DFU process

### 1.2.1 On Linux
```
sudo dfu-util --alt 0 --download build/app_purple_pedal/zephyr/zephyr.signed.bin --detach-delay 20

sudo dfu-util --alt 0 --download build/app_purple_pedal/zephyr/zephyr.signed.bin
```
### 1.2.2 On windows:

Need to download Windows version of dfu-util:

https://dfu-util.sourceforge.net/

https://dfu-util.sourceforge.net/releases/

https://dfu-util.sourceforge.net/releases/dfu-util-0.11-binaries.tar.xz

Use Zadig tool to install WinUSB driver for the HID device:

https://github.com/libusb/libusb/wiki/Windows#How_to_use_libusb_on_Windows

Note that this Zadig WinUSB driver install needs to be done twice, the device shall disconnect once and connect with another VID/PID.

run from windows terminal:
```
.\dfu-util.exe -d 0483:a575 --alt 0 --download zephyr.signed.bin
```

To prevent DFU failure, it is recommended to turn off the program that is using the HID so that the USB bus is not busy and transmission is less likely to go wrong.

### 1.2.3 LED status during mcuboot phase
The idea of mcuboot status hook is got from:

https://github.com/zephyrproject-rtos/zephyr/tree/v4.2.0/tests/boot/mcuboot_recovery_retention

a module is added and linked when building MCUBoot, so that the hook function can be called.
this can be used to update LED status.


When there is update, the output log shows below:

```
uart:~$ *** Booting MCUboot v2.2.0-54-g4eba8087fa60 ***
*** Using Zephyr OS build v4.2.0 ***
mcuboot_status: 0
E: mcuboot_status: 0

mcuboot_status: 1
E: mcuboot_status: 1

mcuboot_status: 2
E: mcuboot_status: 2
```
status 1 is MCUBOOT_STATUS_UPGRADING, direct booting does NOT have this

On Docker, the device shall be attache twice after DFU is initiated, because it changes the mode of operation and the device needs to be "reattached"

TODO: how to show LED status during firmware upgrading process:
https://github.com/search?q=repo%3Azephyrproject-rtos%2Fzephyr+mcuboot-led&type=code&p=4

CONFIG_MCUBOOT_INDICATION_LED:
https://github.com/zephyrproject-rtos/mcuboot/blob/d5b0dcb9aaee397fc105ae2228e8030038c3d871/boot/zephyr/io.c#L62C8-L62C37

take a look at:
https://docs.zephyrproject.org/apidoc/latest/group__usbd__api.html

How to add more USBD detailed information

## 1.3 Code modification of Zephyr AD7124 driver:

Below code modification is needed to improve Zephyr AD7124 driver and add the function we need in PurplePedal.

Zephyr AD7124 driver does not enable Zero latency mode,
after enabling this mode, AND also set the output data rate to 1500 SPS,
channel1 jitter is reduced.

```c
static int adc_ad7124_filter_cfg(const struct device *dev, const struct ad7124_channel_config *cfg)
{
	int filter_setup = 0;
	int filter_mask = 0;
	int ret;

	// filter_setup = FIELD_PREP(AD7124_FILTER_CONF_REG_FILTER_MSK, cfg->props.filter_type) |
	// 	       FIELD_PREP(AD7124_FILTER_FS_MSK, cfg->props.odr_sel_bits);
	// filter_mask = AD7124_FILTER_CONF_REG_FILTER_MSK | AD7124_FILTER_FS_MSK;

	filter_setup = FIELD_PREP(AD7124_FILTER_CONF_REG_FILTER_MSK, cfg->props.filter_type) |
		       FIELD_PREP(AD7124_FILTER_FS_MSK, cfg->props.odr_sel_bits) |
			   FIELD_PREP(GENMASK(16, 16), 1);
	filter_mask = AD7124_FILTER_CONF_REG_FILTER_MSK | AD7124_FILTER_FS_MSK | GENMASK(16, 16);

	/* Set filter type and odr*/
	ret = adc_ad7124_reg_write_msk(dev, AD7124_FILTER(cfg->cfg_slot), AD7124_FILTER_REG_LEN,
				       filter_setup, filter_mask);
	if (ret) {
		return ret;
	}

	return 0;
}
```

To enable the burnout detection
add below lines in adc_ad7124_setup_cfg() function:

```c
static int adc_ad7124_setup_cfg(const struct device *dev, const struct ad7124_channel_config *cfg)
{
	const struct adc_ad7124_config *config = dev->config;
	int ret;
	int configuration_setup = 0;
	int configuration_mask = 0;
	int ref_internal = 0;

	if (cfg->props.bipolar) {
		configuration_setup |= AD7124_CFG_REG_BIPOLAR;
	}

	if (cfg->props.inbuf_enable) {
		configuration_setup |= AD7124_CFG_REG_AIN_BUFP | AD7124_CFG_REG_AINN_BUFM;
	}

	if (cfg->props.refbuf_enable) {
		configuration_setup |= AD7124_CFG_REG_REF_BUFP | AD7124_CFG_REG_REF_BUFM;
	}

	configuration_setup |= FIELD_PREP(AD7124_SETUP_CONF_REG_REF_SEL_MSK, cfg->props.refsel);
	configuration_setup |= FIELD_PREP(AD7124_SETUP_CONF_PGA_MSK, cfg->props.pga_bits);
	configuration_mask |= AD7124_SETUP_CONFIGURATION_MASK;

	//added by Shan to give default burnout current.
#define AD7124_SETUP_CONF_BURNOUT_MSK         GENMASK(10, 9)
	configuration_setup |= FIELD_PREP(AD7124_SETUP_CONF_BURNOUT_MSK, 0x1); //01 = burnout current source on, 0.5 μA.
	configuration_mask |= AD7124_SETUP_CONFIGURATION_MASK;

	...
}
```

TODO: try to leverage the zephyr patch feature:
https://docs.zephyrproject.org/latest/develop/west/zephyr-cmds.html#working-with-patches-west-patch

TODO: AD4130/1 might be a better choice.

## 1.4 Implementing Calibration function

Considering loadcell may have initial offset, and offset can be negative differential voltage,
ADC shall be configured as bipolar operation so that it can detect and compensate this offset.


Calibration usese USB HID feature report.

HIDApiTest tool can be used for windows to debug and send / receive HID feature report:

https://github.com/todbot/hidapitester

below command can read / write feature report:
```
//read raw values
.\hidapitester.exe --vidpid 2FE3/0005 --usagePage 0x1 --usage 0x04 --open --read-feature 2

//read calibration settings
.\hidapitester.exe --vidpid 2FE3/0005 --usagePage 0x1 --usage 0x04 --open --read-feature 3

//set calibration settings
.\hidapitester.exe --vidpid 2FE3/0005 --usagePage 0x1 --usage 0x04 --open --send-feature 3,0x01,0x02,0x03
 
 .\hidapitester.exe --vidpid 2FE3/0005 --usagePage 0x1 --usage 0x04 --open --send-feature 3,0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x00,0x4D,0x62,0x10,0x00,0x4D,0x62,0x10,0x00,0x4D,0x62,0x10,0x00

//smallar scale factor
 .\hidapitester.exe --vidpid 2FE3/0005 --usagePage 0x1 --usage 0x04 --open --send-feature 3,0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x00,0x4D,0x62,0x08,0x00,0x4D,0x62,0x08,0x00,0x4D,0x62,0x08,0x00

//offset factor
.\hidapitester.exe --vidpid 2FE3/0005 --usagePage 0x1 --usage 0x04 --open --send-feature 3,0x00,0xf0,0x81,0x00,0x00,0xf0,0x81,0x00,0x00,0xf0,0x81,0x00,0x4D,0x62,0x08,0x00,0x4D,0x62,0x08,0x00,0x4D,0x62,0x08,0x00

 //read dualsense of PS5:
 .\hidapitester.exe --vidpid 054C/0CE6 --open --get-report-descriptor
```

with DiView + PS5 dualsense, Rx and Ry triggers are single directional value, with operating range 0~255:
```
0x85, 0x01,        //   Report ID (1)
0x09, 0x30,        //   Usage (X)
0x09, 0x31,        //   Usage (Y)
0x09, 0x32,        //   Usage (Z)
0x09, 0x35,        //   Usage (Rz)
0x09, 0x33,        //   Usage (Rx)
0x09, 0x34,        //   Usage (Ry)
0x15, 0x00,        //   Logical Minimum (0)
0x26, 0xFF, 0x00,  //   Logical Maximum (255)
0x75, 0x08,        //   Report Size (8)
0x95, 0x06,        //   Report Count (6)
0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
```

## different HID behavior on Linux and Windows

On Windows, the HID device is directly used by the system. There is no error when calling
hid_device_submit_report(). 

On windows, below website can be used to test the HID:

[https://hardwaretester.com/gamepad](https://hardwaretester.com/gamepad)

On Linux, after HID is plugged in, it is not directly used by the system.
hid_device_submit_report() will report error, until the system opens the HID and begin to receive message.

Below command can manually begin to receive HID message from Linux:
```shell
cat /dev/input/event0
```

# 2. How to use the Binary file package:

There are 3 files:

bootloader_and_app_ver01.bin: bootloader + applilcation ver1

app_ver01.signed.bin: application ver1 only, for testing firmware update over USB

app_ver02.signed.bin: application ver2 only, for testing firmware update over USB


## initial programming:

Use JLink J_Flash tool to program bootloader_and_app_ver01.bin to the board.

Use the UART on JLink to monitor the boot message, below bootup message should be seen

```
*** Booting Zephyr OS build v4.3.0 ***
[00:00:00.004,000] <inf> main: PurplePedal App Version: 0.1
[00:00:00.010,000] <inf> usb: USB DFU sample is initialized
...
```
## Firmware update over USB with dfu-util (on Windows system):

### step 1: download dfu-util
Need to download Windows version of dfu-util:

https://dfu-util.sourceforge.net/releases/dfu-util-0.11-binaries.tar.xz

Unzip the package, windows DFU tool is under the /win64 folder.

### step 2: install WinUSB driver using Zadig tool

Download Zadig tool to install WinUSB driver for the HID device:

https://zadig.akeo.ie/

Plug in the PurplePedal board, open Zadig tool, the board show up as "USBD Sample", Install WinUSB driver for this device.

Note that this Zadig WinUSB driver install needs to be done twice, the device shall disconnect once and connect with a different VID/PID.

### step 3: use dfu-util to update the firmware

Copy app_ver01.signed.bin and app_ver02.signed.bin to the /win64 folder for easier access.

run from windows terminal:
```
cd C:\Users\<your_user_name>\Downloads\dfu-util-0.11-binaries\dfu-util-0.11-binaries\win64>

.\dfu-util.exe -d 0483:a575 --alt 0 --download app_ver02.signed.bin

.\dfu-util.exe -d 0483:a575 --alt 0 --download app_ver02.signed.bin -t64

.\dfu-util.exe -d 0483:a575 --alt 0 --download app_ver02.signed.bin -t64
```

When first time running this, dfu-util will fail after device is disconnected. This is because the device appears with another VID/PID under DFU mode. Need to repeat step2 to install WinUSB driver again for this device,
and re-run the dfu-util command. 

Note that this only happens for the first time. After that, it should run without any issue.

After DFU is finished, below message can be seen from JLink UART output.
Note that the device will take 10 to 20 seconds to upgrade and run the new firmware.
Then it shows App Version: 0.2

```
 *** Booting MCUboot v2.2.0-192-g96576b341ee1 ***
*** Using Zephyr OS build v4.3.0 ***

<this will take 10~20 seconds>

*** Booting Zephyr OS build v4.3.0 ***
[00:00:00.004,000] <inf> main: PurplePedal App Version: 0.2

```

Firmware version can also be downgraded, using below command to program app_ver01.signed.bin:

```
.\dfu-util.exe -d 0483:a575 --alt 0 --download app_ver01.signed.bin
```

# 3. How to perform calibration to the pedal

The goal of the calibration process is to compensate the loadcell offset and sensitivity.
The calibration process requires user collaboration to interact with a PC application, 
and follow the instruction to unpress the pedal and press the pedal,
so that the PC application can read the loadcell raw measurements, calculate the calibration values,
and write it into the PurplePedal non-volatile memory.

The PurplePedal side only provides the methods to read the raw loadcell readings and to write the calibration coefficients. The user interaction and coefficient calculation shall be implemented in PC software.

The PurplePedal provides 4 report IDs in total, 

- report ID 0x01 is normal HID input report. 
- report ID 0x02 is the feature report to read the raw loadcell
- report ID 0x03 is the feature report to write the calibration setting.
- report ID 0x04 is the feature report of device unique ID

C code snippet is show as below:
```c
#define GAMEPAD_INPUT_REPORT_ID (0x01)
#define GAMEPAD_FEATURE_REPORT_RAW_VAL_ID (0x02)
#define GAMEPAD_FEATURE_REPORT_CALIB_ID (0x03)
#define GAMEPAD_FEATURE_REPORT_UID_ID (0x04)
```

These feature reports can be read/written with Windows API. Here we show the manual process to perform the calibration by using HIDApiTest tool. This tool can be used for windows to debug and send / receive HID feature report, below is the download link:

https://github.com/todbot/hidapitester/releases/tag/v0.5

Download the zip file for windows-x84_64, and unzip it in Windows. 
This is a command-line tool, use Windows shell to run it.

## Step 1: get the raw loadcell reading when pedal is not pressed to calculate the Offset value

Use hidapitester command to read the feature report 0x02 with raw ADC readings:

```sh
.\hidapitester.exe --vidpid 0483/A575 --usagePage 0x1 --usage 0x04 --open --length 10 --read-feature 2
Opening device, vid/pid:0x0483/A575, usagePage/usage: 1/4
Device opened
Reading 10-byte feature report, report_id 2...read 10 bytes:
 02 83 03 7E B2 26 00 C2 26 00
Closing device
```

Note that the result is 10-byte, the first byte is report ID, 
followed by 3x 24bit RAW ADC value from channel 1 2 and 3.
The 24bit value is in little-endian format.
```
02 			//report ID
83 03 7E 	//channel 1 raw ADC value (low byte, middle byte, high byte)
B2 26 00  	//channel 2 raw ADC value
C2 26 00 	//channel 3 raw ADC value
```

Multiple readings can be taken from the pedal to calculate the average number for each pedal when the pedal is not pressed. This average value is called "Offset".

$$Offset=Avg_{unpressed}$$

## Step 2: get the raw loadcell reading when pedal is fully pressed to calculate the Scale value

Fully press the pedal and use the same process as described in Step 1 to read the feature report 0x02 and get the raw ADC readings.

Multiple readings can be taken from the pedal to calculate the average number for each pedal when the pedal is fully pressed.  The average value when pedal is pressed minused by average value when pedal is not pressed can be taken as the scale value.

If we want to have some "margin" in the pedal press, it is also possible to multiply this average value by 105% and take the result as final "Scale" value.

$$Scale=(Avg_{pressed} - Avg_{unpressed})*margin%$$

## Step 3: Write the calibration 

The original calibration number can be read by below command:
```sh
.\hidapitester.exe --vidpid 0483/A575 --usagePage 0x1 --usage 0x04 --open --length 25 --read-feature 3
Opening device, vid/pid:0x0483/A575, usagePage/usage: 1/4
Device opened
Reading 25-byte feature report, report_id 3...read 25 bytes:
 03 00 00 80 00 00 00 80 00 00 00 80 00 4D 62 10 00 4D 62 10 00 4D 62 10 00
Closing device
```
Note that the result is 25-byte format, with 3 offset values and 3 scale values.
Offset and scale values are provided in 32-bit little-endian format.
```
03 
00 00 80 00 	//channel 1 offset value, 32bit, little-endian
00 00 80 00  	//channel 2 offset value, 32bit, little-endian
00 00 80 00  	//channel 3 offset value, 32bit, little-endian
4D 62 10 00  	//channel 1 scale value, 32bit, little-endian
4D 62 10 00  	//channel 2 scale value, 32bit, little-endian
4D 62 10 00 	//channel 3 scale value, 32bit, little-endian
```

New calibration value can be written by below command:

```sh
 .\hidapitester.exe --vidpid 0483/A575 --usagePage 0x1 --usage 0x04 --open --send-feature 3,0x00,0x03,0x7E,0x00,0x00,0x03,0x7E,0x00,0x00,0x03,0x7E,0x00,0x00,0x54,0x04,0x00,0x00,0x54,0x04,0x00,0x00,0x54,0x04,0x00
```

After that, the new calibration value shall take effect

## How the PurplePedal firmware uses Scale and Offset value

PurplePedal USB HID outputs 0-65535 pedal value. The 24bit raw ADC value is converted to 0-65535 range using below C code:

```c
inline uint16_t raw_to_uint16(int32_t raw, int32_t offset, int32_t scale)
{
	if(raw < offset) return 0;
	int64_t val_64 = ((int64_t)(raw - offset)) * UINT16_MAX / scale;
	return (val_64 > UINT16_MAX)? UINT16_MAX : (uint16_t)val_64;
}
```
The above code guarantees output 0-65535, any value lower than 0 will be treated as 0, any value greater than 65535 will be treated as 65535.

## Default Offset and Scale values

For a newly programmed device that is never calibrated, the firmware includes default offset and scale value.
Note that these values are calculated using the testing loadcell and can be updated according to the loadcell choice in final product.

```sh
.\hidapitester.exe --vidpid 2FE3/0005 --usagePage 0x1 --usage 0x04 --open --read-feature 3
Opening device, vid/pid:0x0483/A575, usagePage/usage: 1/4
Device opened
Reading 64-byte feature report, report_id 3...read 25 bytes:
 03 00 00 80 00 00 00 80 00 00 00 80 00 4D 62 10 00 4D 62 10 00 4D 62 10 00 00 00 00 00 00 00 00
 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
Closing device
```

# 4. How to read STM32 unique ID using DFU

Feature report ID 0x04 is defined for STM32 unique ID.

```c
#define GAMEPAD_INPUT_REPORT_ID (0x01)
#define GAMEPAD_FEATURE_REPORT_RAW_VAL_ID (0x02)
#define GAMEPAD_FEATURE_REPORT_CALIB_ID (0x03)
#define GAMEPAD_FEATURE_REPORT_UID_ID (0x04)
```
this feature report can be read by using HIDApiTest, see previous section for where to find and download it.

Below command shows how to read the STM32 unique ID and the output of the command.

```sh
.\hidapitester.exe --vidpid 0483/a575 --usagePage 0x1 --usage 0x04 --open --length 13 --read-feature 4
Opening device, vid/pid:0x0483/0xA575, usagePage/usage: 1/4
Device opened
Reading 13-byte feature report, report_id 4...read 13 bytes:
 04 20 39 39 51 54 42 50 0E 00 52 00 33
Closing device
```

Note that the result is 13-byte, the first byte is report ID, 
followed by 3x 32bit unique ID information.

To verify the result, also read the STM32 UID by JLink or dfu-util:

```
JLinkExe
outputs below information

Cortex-M0 identified.
1FFF7590 = 00520033 5442500E 20393951 
J-Link>mem32 0x1FFF7590,3
1FFF7590 = 00520033 5442500E 20393951 

```

Using STM32 ROM DFU we can read this:
```
./dfu-util -d 0483:df11 -a 0 -s 0x1FFF7590:12:force -U uid.bin
```

# 5 how to set /get curve values

CONFIG_UDC_BUF_POOL_SIZE controls the maximum 

# 5. reference documents

HID specification:

https://www.usb.org/sites/default/files/hid1_11.pdf

HID usage table:

https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf