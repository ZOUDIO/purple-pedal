# Purple Pedal Application

## Build the application
```shell
west build -b purple_pedal --sysbuild zephyr_sdk/app_purple_pedal

west build -b nrf52840dk/nrf52840 --sysbuild zephyr_sdk/app_purple_pedal

west build -b nrf52840dk/nrf52840 --sysbuild zephyr_sdk/app_purple_pedal -- -DEXTRA_CONF_FILE="overlay-shell.conf"

```

There are 2 possible options for firmware update:

1. update via USB CDC ACM: https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/subsys/mgmt/mcumgr/smp_svr

2. update via USB DFU: https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/subsys/usb/dfu


TODO: AD4130/1 might be a better choice.

# reference documents

HID specification:

https://www.usb.org/sites/default/files/hid1_11.pdf

HID usage table:

https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf