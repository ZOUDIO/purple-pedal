# Purple Pedal Application

## Build the application
```shell
west build -b purple_pedal --sysbuild zephyr_sdk/app_purple_pedal

west build -b purple_pedal --sysbuild zephyr_sdk/app_purple_pedal -- -DEXTRA_CONF_FILE="overlay-shell.conf"

west build -b nrf52840dk/nrf52840 --sysbuild zephyr_sdk/app_purple_pedal

west build -b nrf52840dk/nrf52840 --sysbuild zephyr_sdk/app_purple_pedal -- -DEXTRA_CONF_FILE="overlay-shell.conf"

```

There are 2 possible options for firmware update:

1. update via USB CDC ACM: https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/subsys/mgmt/mcumgr/smp_svr

2. update via USB DFU: https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/subsys/usb/dfu


TODO: AD4130/1 might be a better choice.

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


## DFU process

### On Linux
```
sudo dfu-util --alt 0 --download build/app_purple_pedal/zephyr/zephyr.signed.bin --detach-delay 20

sudo dfu-util --alt 0 --download build/app_purple_pedal/zephyr/zephyr.signed.bin
```
### On windows:

Need to download Windows version of dfu-util:

https://dfu-util.sourceforge.net/

https://dfu-util.sourceforge.net/releases/

https://dfu-util.sourceforge.net/releases/dfu-util-0.11-binaries.tar.xz

Use Zadig tool to install WinUSB driver for the HID device:

https://github.com/libusb/libusb/wiki/Windows#How_to_use_libusb_on_Windows

Note that this Zadig WinUSB driver install needs to be done twice, the device shall disconnect once and connect with another VID/PID.

run from windows terminal:
```
.\dfu-util.exe -d 2fe3:0005 --alt 0 --download zephyr.signed.bin
```

To prevent DFU failure, it is recommended to turn off the program that is using the HID so that the USB bus is not busy and transmission is less likely to go wrong.

### LED status during mcuboot phase
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

## Existing Issues

nRF PWN LED cannot set the period >250ms. maybe STM32 can. we may need timer to control LEDs


# reference documents

HID specification:

https://www.usb.org/sites/default/files/hid1_11.pdf

HID usage table:

https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf