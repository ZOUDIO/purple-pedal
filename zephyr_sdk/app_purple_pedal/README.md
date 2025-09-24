# Purple Pedal Application

```shell
west build -b purple_pedal --sysbuild zephyr_sdk/app_purple_pedal

west build -b nrf52840dk/nrf52840 --sysbuild zephyr_sdk/app_purple_pedal
```

There are 2 possible options for firmware update:

1. update via USB CDC ACM: https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/subsys/mgmt/mcumgr/smp_svr

2. update via USB DFU: https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/subsys/usb/dfu