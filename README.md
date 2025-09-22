# Github repository location:
```
https://github.com/<Deleted link>/gm02s_stm32_board_test
```

# Pre-requisit
VSCode + Devcontainer plugin ( called 'Dev Containers', created by Microsoft )
WSL2

WSL shall be configured with SYSTEMD=true

Note that openOCD debugger works correctly after copying both jlink and openocd udev rules to WSL. Doing it in docker container is not enough.

#Install WSL
Option 1)
go to Microsoft Store
install Ubunut 22.04.05 LTS

Option 2)
wsl --list --online
wsl --install -d Ubuntu-22.04

WARNING FOR <Deleted Link> IT MANAGED SYSTEMS

If you receive an error like below during fetching the docker image
0.882 ERROR: cannot verify objects.githubusercontent.com's certificate, issued b
y ‘CN=Cisco Umbrella Secondary SubCA ams-SG,O=Cisco’:
0.882   Unable to locally verify the issuer's authority.

Please disable the Umbrella services
```
services.msc
->tab->Standard
->Stop Cisco Secure Client - Umbrella Agent
->Stop Cisco Secure Client - Umbrella SWG Agent
run docker image ( during the first run all software will be fetched)
->Start Cisco Secure Client - Umbrella Agent
->Start Cisco Secure Client - Umbrella SWG Agent
```
# Opening the projet
## clone the project to WSL folder and open (Recommended in windows)
This method is recommended because the disk access is fast and thus build speed is much faster.
On Linux, this is not applicable and use the method described in next section.

This method is inspired by: https://code.visualstudio.com/blogs/2020/07/01/containers-wsl

Follow the below steps to clone code in WSL and open it in VSCode:

1. set up git and ssh keys on WSL2.
2. git clone the project to ~/Documents folder. Note that do not use any folder mapped from your WIndows host to WSL, because this will result in slow disk access and slow build.
3. enter the folder and start vscode from the folder
```shell
cd ~/Documents/gm02s_stm32_board_test
code .
```
4. VSCode will open the folder, set up the docker machine. Then proceed do section "Initialize the zephyr workspace"

## clone the project to Windows local folder and open (Not recommended on windows, slow build)
This method is NOT recommended on Windows, because the disk access is slow and thus build speed is much slower.
On Linux, this method should be used,

if opening the project in VSCode results in decontainer error, try to install the devcontainer commandline and run the commandline:

https://code.visualstudio.com/docs/devcontainers/devcontainer-cli

You can quickly try out the CLI through the Dev Containers extension. Select the Dev Containers: Install devcontainer CLI command from the Command Palette

run the commandline from VSCode powershell:
```
devcontainer open
```

## Initialize the zephyr workspace
When first opening the project, after docker is created, run below command manually to initialize the zephyr workspace.
```shell
west init -l zephyr_sdk
west update
west blobs fetch hal_espressif
```

Note: after the initialization build of docker container, Debugger USB passthrough may not work out of the box. Just close VS Code and reopen it, the device will show up correctly and JLinkExe can also work.


Location of Zephyr SDK and openOCD
/opt/toolchains/zephyr-sdk-0.16.8/sysroots/x86_64-pokysdk-linux/usr/share/openocd/contrib/

# flash and debug the project

## 1. (Windows host only) instal usbipd
Follow instructions in below link to install USBIPD and map USB device to docker:
(https://learn.microsoft.com/en-us/windows/wsl/connect-usb)[https://learn.microsoft.com/en-us/windows/wsl/connect-usb]

## 2. Flash and debug
```shell
west flash -r jlink
west flash --esp-device /dev/ttyUSBx
```

```shell
west debugserver -r jlink
west debugserver -r openocd
west debugserver -r openocd --serial xxx
```
Espressif also supports UART monitoring
```shell
west espressif monitor -p /dev/ttyUSB2
```
Further readings on flashing and debugging:
https://docs.zephyrproject.org/latest/develop/flash_debug/host-tools.html#flash-debug-host-tools

## 3. About ESP32 OpenOCD debug support

at this moment, Ubuntu22.04 jammy has openocd 0.11 version,
openocd-esp32 has 0.12 version but with broken support.
the one tested good is:
https://github.com/xpack-dev-tools/openocd-xpack/releases/tag/v0.12.0-4

So the docker container by default installs this version for ESP32 debug,
and all ESP32 boards are pointed to this debugger.

at this moment, there is a but in zephyr openocd runner, 
ESP32 should run
```
west debugserver --cmd-reset-halt "init; halt"
```

# Run test cases
Note that for running the test on actual board, --device-testing option is needed.

Note that for ESP32, same port is used for flashing and minotoring, 
so --flash-before option is needed.

```
west twister -p tn_gm02s_esp32/esp32/procpu --device-testing --device-serial /dev/ttyUSB2 --flash-before  -T zephyr_sdk/tests/lib/custom/
```
If only want to build the test, use --build-only (-b) option.

# Notes and references on Docker + Devcontainer + WSL
References and ideas of devcontainer + docker
https://github.com/cooked/vscode-zephyr-template/blob/master/.devcontainer/Dockerfile

https://github.com/<Deleted link>/amp-connectivity-nodes/tree/main
https://github.com/<Deleted link>/conplf-cnflasher
https://github.com/<Deleted link>/amp-connectivity-nodes-devcontainer
https://github.com/<Deleted link>/amp-connectivity-nodes/wiki/Deploying-and-Debugging
https://github.com/project-chip/connectedhomeip/blob/master/.vscode/launch.json

Solve the USB mapping and non-root access  issue:
user@dc8e37a113f0:/workspaces/zephyr_docker$ sudo dpkg -i ./JLink_Linux_x86_64.deb
(Reading database ... 64616 files and directories currently installed.)
Preparing to unpack ./JLink_Linux_x86_64.deb ...
Removing /opt/SEGGER/JLink ...
Unpacking jlink (8.10.0) over (8.10.0) ...
Setting up jlink (8.10.0) ...
Updating udev rules via udevadm...
OK
1-1: Failed to write 'remove' to '/sys/devices/platform/vhci_hcd.0/usb1/1-1/uevent': Read-only file system
1-1: Failed to write 'add' to '/sys/devices/platform/vhci_hcd.0/usb1/1-1/uevent': Read-only file system

Hint of solution:
https://askubuntu.com/questions/1408365/wsl-how-do-i-set-the-group-for-a-tty-device/1449282#1449282
https://learn.microsoft.com/en-us/windows/wsl/systemd
https://learn.microsoft.com/en-us/windows/wsl/wsl-config
https://devblogs.microsoft.com/commandline/systemd-support-is-now-available-in-wsl/

If USB passthrough cannot work we try this:
https://wiki.segger.com/WSL

According to https://docs.docker.com/reference/cli/docker/container/run/#privileged
Escalate container privileges (--privileged)
The --privileged flag gives the following capabilities to a container:
Enables all Linux kernel capabilities
Disables the default seccomp profile
Disables the default AppArmor profile
Disables the SELinux process label
Grants access to all host devices
Makes /sys read-write
Makes cgroups mounts read-write

So we should use priviledged mode.
Note: after the initialization build of docker container, things may not work out of the box.
Just close VS Code and reopen it, the device will show up correctly and JLinkExe can also work.

It is also mandatory to follow the below instruction to add systemd support to the docker container, so that the USB debugger can have non-root access.
https://learn.microsoft.com/en-us/windows/wsl/wsl-config#systemd-support


Current issue:
on devcontainer docker, below command gives error result that systemd is not started:
```shell
user@a613d189b38f:/workspaces/gm02s_stm32_board_test$ sudo systemctl status
System has not been booted with systemd as init system (PID 1). Can't operate.
Failed to connect to bus: Host is down
```
while on windows wsl command, below command gives the correct result.


# Update zephyr ,zephyr-sdk and docker

Zephyr version is in the west.yaml file
Zephyr SDK version should come as a stable version from https://github.com/zephyrproject-rtos/docker-image/releases


# LOG enable debug record:

CONFIG_LOG_BACKEND_UART=y can cause hanging problem, should be disabled.
with this option disabled, and NET_LOG disabled, shell can work.

turning on CONFIG_NET_LOG=y, shell will freeze again

turning on CONFIG_LOG_MODE_MINIMAL=y, shell will not freeze

# Debug MODEM PPP, IPCP, PAP
Options that turns on MODEM AT level log and PPP level log:

```
#this option prints out AT command level logs
CONFIG_MODEM_MODULES_LOG_LEVEL_DBG=y
#this option prints out IPCP, PAP level logs
CONFIG_NET_L2_PPP_LOG_LEVEL_DBG=y
```

When PPP level log is enabled, PPP packet transanctions are printed out too.
Below link gives good explanation on PPP fram format,

https://support.huawei.com/enterprise/en/doc/EDOC1100174721/a3a1ebd0/ppp

This helps to debug the MODEM PPP transanctions.

# Build and run for native_sim in docker

## to use the network:
Select the native_sim/native/64 target when building the example.
For the example to run and use internet, run below command:

```shell
#nat.conf enables dhcp and dnsmasq
cd tools/net-tools
./net-setup.sh --config nat.conf start
./net-setup.sh --config nat.conf stop

# run the zephyr code
build/zephyr/zephyr.exe
```
Note, if above command throws error on iptables/dnsmasq cannot be found, install these packages first

https://github.com/zephyrproject-rtos/zephyr/blob/c22233a1afcb97e55af2929c7d1416da77d96ca9/doc/connectivity/networking/native_sim_setup.rst

https://github.com/zephyrproject-rtos/net-tools/blob/865c0377f1cd19fec970703325a8adf55a99f03b/README%20NAT.md

## To connect to the UART0 terminal

```shell
# run the zephyr code
build/zephyr/zephyr.exe

#the bootup message will show which UART is used for UART0
uart connected to pseudotty: /dev/pts/8
*** Booting Zephyr OS build v3.7.1 ***

#open another bash terminal and run
screen /dev/pts/8

#when finished, press ctrl+A, K to close
```

## to debug native_sim application in the docker
Note that this requires "gdbserver" is installed on the docker.
If not installed, pull the latest docker or run "sudo apt install gdbserver"

To run the debug server, in commandline, run
```
west debugserver
```
in VSCode window, choose "Debug Linux GDB"

After GDB is attached the shell prompt will pop up



# STM32WB FUS download and flash

## FUS firmware version
The FUS version should match the version mentioned in below file:
<workdir>\modules\hal\stm32\lib\stm32wb\hci\README
For Zephyr v3.7.1, FUS v1.9.1 package should be used, 
FUS patch can be downloaded from here

https://www.st.com/en/embedded-software/stm32cubewb.html

Use below firmware lead to correct bootup

- stm32wb5x_FUS_fw.bin 0x080EC000
- stm32wb5x_BLE_HCILayer_fw.bin	0x080E0000

Other stm32wb5x_BLE_HCILayer_extended_fw.bin or stm32wb5x_BLE_Stack_full_fw.bin does not work according to my test.

## Flash FUS to STM32WB
ST-Link + STM32CubeProgrammer is used to program the FUS,

If any issue occurs, always try to do chip erase and erase the FUS firmware for a fresh start.

JLink should also work but i did not succeed.





# ==============================================================
# ===============old readme, to be removed======================
# To compile for Nucleo STM32 MCU using Nordic nRFConnect toolchain:

**Note** that Nordic SDK does NOT use the default west.yml file under zephyr source folder.
Instead, it uses C:\Users\310188132\ncs\v2.0.0/nrf/west.yml.
So that file shall be modified to add below

Open  C:\Users\310188132\ncs\v2.0.0/nrf/west.yml, and add below line:
```sh
        name-allowlist:
          - TraceRecorderSource
          - canopennode
          - chre
          - civetweb
          - cmsis
          - edtt
          - fatfs
          - fff
          - hal_nordic
          - hal_st
          - hal_stm32 #added by Shan Song, this line makes it possible to compile for Nucleo-F767ZI and other ST Nucleo boards.
```
then under the sample folder, call:
```
west build -p always -b nucleo_f767zi samples\basic\blinky
```
Add Jlink exe to the existing path:
```
set PATH=%PATH%;C:\Program Files (x86)\SEGGER\JLink

$env:Path += ";C:\Program Files (x86)\SEGGER\JLink"

```
To flash:
```
west flash -b nucleo_f767zi -r jlink
```
to debug the program:
```
west debug -r jlink
```

# Update ST-link to JLINK
https://www.segger.com/products/debug-probes/j-link/models/other-j-links/st-link-on-board/

# Steps to debug Nucleo + Zephyr using VScode + nRF SDK:
1. Install nRFConnect and nRFConnect for VS Code
2. Open  C:\Users\310188132\ncs\v2.0.0/nrf/west.yml, and add hal_stm32. run west update to download it.
```sh
        name-allowlist:
          - TraceRecorderSource
          - canopennode
          - chre
          - civetweb
          - cmsis
          - edtt
          - fatfs
          - fff
          - hal_nordic
          - hal_st
          - hal_stm32 #added by Shan Song, this line makes it possible to compile for Nucleo-F767ZI and other ST Nucleo boards.
```
3. create a new project in vs code, by copying existing example, e.g. sensor_shell
4. Compile the project from command line, using west build and give Nucleo board type.
5. Manually call west flash to flash the nucleo board (cannot be automated by pressing the flash button in VS code). Check from the UART console if the shell prompt pops up.
6. Modify settings.json to add the location of arm-none-eabi toolchain
```
{
    "workbench.colorTheme": "Visual Studio Light - C++",
    "cmake.configureOnOpen": true,
    "idf.gitPath": "C:/Users/310188132/.espressif/tools/idf-git/2.30.1/cmd/git.exe",
    "idf.espIdfPathWin": "C:/ShanSong/MyDevelop/esp-idf/",
    "idf.pythonBinPathWin": "C:\\Users\\310188132\\.espressif\\python_env\\idf4.4_py3.8_env\\Scripts\\python.exe",
    "idf.toolsPathWin": "C:\\Users\\310188132\\.espressif",
    "idf.customExtraPaths": "C:\\Users\\310188132\\.espressif\\tools\\xtensa-esp32-elf\\esp-2021r2-patch3-8.4.0\\xtensa-esp32-elf\\bin;C:\\Users\\310188132\\.espressif\\tools\\xtensa-esp32s2-elf\\esp-2021r2-patch3-8.4.0\\xtensa-esp32s2-elf\\bin;C:\\Users\\310188132\\.espressif\\tools\\xtensa-esp32s3-elf\\esp-2021r2-patch3-8.4.0\\xtensa-esp32s3-elf\\bin;C:\\Users\\310188132\\.espressif\\tools\\riscv32-esp-elf\\esp-2021r2-patch3-8.4.0\\riscv32-esp-elf\\bin;C:\\Users\\310188132\\.espressif\\tools\\esp32ulp-elf\\2.28.51-esp-20191205\\esp32ulp-elf-binutils\\bin;C:\\Users\\310188132\\.espressif\\tools\\esp32s2ulp-elf\\2.28.51-esp-20191205\\esp32s2ulp-elf-binutils\\bin;C:\\Users\\310188132\\.espressif\\tools\\cmake\\3.20.3\\bin;C:\\Users\\310188132\\.espressif\\tools\\openocd-esp32\\v0.11.0-esp32-20211220\\openocd-esp32\\bin;C:\\Users\\310188132\\.espressif\\tools\\ninja\\1.10.2;C:\\Users\\310188132\\.espressif\\tools\\idf-exe\\1.0.3;C:\\Users\\310188132\\.espressif\\tools\\ccache\\4.3\\ccache-4.3-windows-64;C:\\Users\\310188132\\.espressif\\tools\\dfu-util\\0.9\\dfu-util-0.9-win64",
    "idf.customExtraVars": "{\"OPENOCD_SCRIPTS\":\"C:\\\\Users\\\\310188132\\\\.espressif\\\\tools\\\\openocd-esp32\\\\v0.11.0-esp32-20211220/openocd-esp32/share/openocd/scripts\",\"IDF_CCACHE_ENABLE\":\"1\"}",
    "nrf-connect.welcome.showOnStartup": true,
    "nrf-connect.enableTelemetry": true,
    "nrf-connect.topdir": "${nrf-connect.sdk:1.9.1}",
    "nrf-connect.toolchain.path": "${nrf-connect.toolchain:1.9.1}",
    "nrf-connect.debugging.backend": "Cortex-Debug",
    "cortex-debug.armToolchainPath.windows": "C:\\ST\\STM32CubeIDE_1.9.0\\STM32CubeIDE\\plugins\\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.10.3-2021.10.win32_1.0.0.202111181127\\tools\\bin",
    "cortex-debug.armToolchainPrefix": "arm-none-eabi",
    "cortex-debug.JLinkGDBServerPath.windows": "C:\\Program Files (x86)\\SEGGER\\JLink\\JLinkGDBServerCL.exe"

}
```
8. go the vs_code "Run and Debug" button, create launch.json with below content:

```
{
// Use IntelliSense to learn about possible attributes.
// Hover to view descriptions of existing attributes.
// For more information, visit: https://go.microsoft.com/fwlink/?linkid=830387
"version": "0.2.0",
"configurations": [
	{
		"name": "Cortex Debug",
		"cwd": "${workspaceFolder}",
		"executable": "./build/zephyr/zephyr.elf",
		"request": "launch",
		"type": "cortex-debug",
		"servertype": "jlink",
		"device": "stm32f429zi",
		"interface": "swd",
		"runToEntryPoint": "main",
		"showDevDebugOutput": "raw"
	}
]
}
```

# SPI flash operations:
https://github.com/zephyrproject-rtos/zephyr/blob/51af614f88eae21dc4baa3cbed03e1de81b053df/samples/drivers/jesd216/src/main.c
seach for device spi_flash0 and its definition in device tree.

Example of flash configuration on single SPI:
https://github.com/zephyrproject-rtos/zephyr/blob/418d601d6d46d0e6e088cdff9c5cbef40694c4b9/boards/arm/adafruit_feather_stm32f405/adafruit_feather_stm32f405.dts

Flash shell commands are enabled. flash test commands are avaialble.
example flash command:
```
flash test s25fl256l@0 0 0x1000 4
flash read s25fl256l@0 0 0x40
flash erase s25fl256l@0 0 0x1000
flash read s25fl256l@0 0 40
```

To test config flash from Nucleo board
```
flash test s25fl064l@0 0 0x1000 4
flash read s25fl064l@0 0 0x40
flash erase s25fl064l@0 0 0x1000
flash read s25fl064l@0 0 40
```

# GPIO_SHELL

For GPIO command shell and how to use it, refer to:
https://github.com/zephyrproject-rtos/zephyr/issues/50232

GPIO shell is enabled. use below command example to toggle IO pin:
On Nucleo board, LD1 is PB0, LD2 is PB7, LD3 is PB14
```
gpio conf gpio@40020400 7 out
gpio blink gpio@40020400 7
gpio get gpio@40020400 7
gpio set gpio@40020400 7 1
```
On STM32+GM02S board, LEDs are PG0~3.
```
gpio conf gpio@40021800 0 out
gpio blink gpio@40021800 0
gpio get gpio@40021800 0
gpio set gpio@40021800 0 1
```

# UART echo test
```
demo uart_echo main_uart <test_string>
demo uart_echo debug_uart <test_string>
```

for the RTS flow control, consider using GPIO command to manually set it to high.
```
#MODEM UART0 RTS connects to PC8 pin
gpio conf gpio@40020800 8 out
gpio set gpio@40020800 8 1

#No need to set up the GPIO pin any more after enable the hardware flow control in device tree.
demo uart_echo modem_uart AT
```

# add ppp GSM modem support
check out the example of 
https://github.com/zephyrproject-rtos/zephyr/blob/main/samples/net/gsm_modem/src/main.c

See if ppp can be added to this board.

Since not all boards support GSM MODEM, GSM support shall be added to the board defconfig files not the top-level project configuration file.

## Socket offloading

many modems has socket related commands. 
socket offloading means to use the socket read/write AT commands instead of managing the socket on the host MCU.

https://docs.zephyrproject.org/3.2.0/connectivity/networking/api/sockets.html#socket-offloading
Socket offloading
Zephyr allows to register custom socket implementations (called offloaded sockets). This allows for seamless integration for devices which provide an external IP stack and expose socket-like API.

Socket offloading can be enabled with CONFIG_NET_SOCKETS_OFFLOAD option. A network driver that wants to register a new socket implementation should use NET_SOCKET_OFFLOAD_REGISTER macro. 

Driver can provide its own socket functions:
```
static const struct socket_op_vtable offload_socket_fd_op_vtable = {
	.fd_vtable = {
		.read	= offload_read,
		.write	= offload_write,
		.close	= offload_close,
		.ioctl	= offload_ioctl,
	},
	.bind		= NULL,
	.connect	= offload_connect,
	.sendto		= offload_sendto,
	.recvfrom	= offload_recvfrom,
	.listen		= NULL,
	.accept		= NULL,
	.sendmsg	= offload_sendmsg,
	.getsockopt	= NULL,
	.setsockopt	= NULL,
};
```

# additiona actions needed on TN_STM32WB55 board

## to build the project:
```
west build -p always -b tn_stm32wb55
```

https://docs.zephyrproject.org/latest/boards/arm/nucleo_wb55rg/doc/nucleo_wb55rg.html#bluetooth-and-compatibility-with-stm32wb-copro-wireless-binaries

To operate bluetooth on Nucleo WB55RG, Cortex-M0 core should be flashed with a valid STM32WB Coprocessor binaries (either ‘Full stack’ or ‘HCI Layer’). These binaries are delivered in STM32WB Cube packages, under Projects/STM32WB_Copro_Wireless_Binaries/STM32WB5x/ For compatibility information with the various versions of these binaries, please check modules/hal/stm32/lib/stm32wb/hci/README in the hal_stm32 repo. Note that since STM32WB Cube package V1.13.2, “full stack” binaries are not compatible anymore for a use in Zephyr and only “HCI Only” versions should be used on the M0 side.

There are 2 otions: 
Option 1. FUS binary flash can be done with STLink STM32CubeProgrammer.

Option 2. To flash the FUS with JLink, see: https://wiki.segger.com/ST_STM32WB

## FUS firmware version
The FUS version should match the version mentioned in below file:
C:\Users\310188132\ncs\<v2.3.0>\modules\hal\stm32\lib\stm32wb\hci\README

Here version v1.14.0 is mentioned.

Use STM32Cube to download the correct version, and find the load address for the binary.

According to below page
https://github.com/zephyrproject-rtos/zephyr/issues/47991
 STM32WB Cube package 13.2.0, "full" binaries are no more compatible with use in Zephyr. Instead, you should now use one of the following binaries:
stm32wb5x_BLE_HCILayer_extended_fw.bin
stm32wb5x_BLE_HCILayer_fw.bin

## Use Jlink to flash the Wireless binary
1. Optional, update FUS binary
```sh
JLink.exe -autoconnect 1 -device "STM32WB55CG - FUS" -if swd -speed 4000

loadfile C:\Users\310188132\STM32Cube\Repository\STM32Cube_FW_WB_V1.17.0\Projects\STM32WB_Copro_Wireless_Binaries\STM32WB5x\stm32wb5x_FUS_fw.bin, 0x080EC000

exit
```
2. Mandatory, update the BLE HCI radio binary:
```sh
JLink.exe -autoconnect 1 -device "STM32WB55CG - FUS" -if swd -speed 4000

loadfile C:\Users\310188132\STM32Cube\Repository\STM32Cube_FW_WB_V1.17.0\Projects\STM32WB_Copro_Wireless_Binaries\STM32WB5x\stm32wb5x_BLE_HCILayer_extended_fw.bin, 0x080DA000

(optional)
loadfile C:\Users\310188132\STM32Cube\Repository\STM32Cube_FW_WB_V1.17.0\Projects\STM32WB_Copro_Wireless_Binaries\STM32WB5x\stm32wb5x_BLE_HCILayer_fw.bin, 0x080E0000

(optional) 
loadfile C:\Users\310188132\STM32Cube\Repository\STM32Cube_FW_WB_V1.17.3\Projects\STM32WB_Copro_Wireless_Binaries\STM32WB5x\stm32wb5x_BLE_HCILayer_extended_fw.bin, 0x080DA000
(optional)
loadfile C:\Users\310188132\STM32Cube\Repository\STM32Cube_FW_WB_V1.17.3\Projects\STM32WB_Copro_Wireless_Binaries\STM32WB5x\stm32wb5x_BLE_HCILayer_fw.bin, 0x080E0000

perhaps see message:
FUS: Waiting for FUS to reply.

exit
```

1. After the exit command, **recycle the power.**

this operation order works. bt commands works after this.

# to update to new nRF-SDK version
1. use nRF-Connect tool, toolchain manager to install the latest nRF-SDK
2. Make change to west.yaml to add the STM32 HAL or Espressif HAL if needed.
3. Building for Espressif, also manually download the Espressif toolchain
4. change VS code project setting and workspace setting to point to the latest SDK
5. compile the code.
6. old toolchain installation can be removed by using nRF toolchain manager.


# Extend the Zephyr project to support ESP32

below command can check the toolchain settings
```
PS C:\ShanSong\Workdocs\CellularIoT\gm02s_stm32_board_test> [System.Environment]::GetEnvironmentVariable('ZEPHYR_SDK_INSTALL_DIR')

c:\Users\310188132\ncs\toolchains\v2.2.0\opt\zephyr-sdk

PS C:\ShanSong\Workdocs\CellularIoT\gm02s_stm32_board_test> [System.Environment]::GetEnvironmentVariable('ZEPHYR_TOOLCHAIN_VARIANT')

zephyr
```

Check the folder: C:\Users\310188132\ncs\toolchains\v2.2.0\opt\zephyr-sdk
there is a sdk_version file in this folder.
And it states sdk_version is: 0.15.1

So the Nordic toolchain is a partial installation of the COMPLETE zephyr SDK.
The zephyr SDK release can be found from:
https://github.com/zephyrproject-rtos/sdk-ng/releases
At the Asset section of each release, you can download the toolchain separatedly.
Here we only Download the 0.15.1 version Xtensa ESP32 toolchain from:
https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.15.1/toolchain_windows-x86_64_xtensa-espressif_esp32_zephyr-elf.zip
Unzip this file and put it under the folder: C:\Users\310188132\ncs\toolchains\v2.2.0\opt\zephyr-sdk


To debug the board, external open_ocd is needed. the hint is to follow the instruction is below page:
https://docs.zephyrproject.org/2.6.0/boards/xtensa/esp32/doc/index.html

To flash the board:
west flash --esp-device <Port name>

## configure the nRF Connect SDK to support ESP32-S3 
### 2.1 Add Espressif HAL repository to Zephyr workspace
After installing nRF toolchain, the toolchain executables and SDK code are under the folder:
```
toolchain folder:
C:\Users\<your user name>\ncs\toolchains\31f4403e35
SDK source code folder
C:\Users\<your user name>\ncs\v2.4.0
```
### normally below update is sufficient
Edit west configuration file at: C:\Users\<your user name>\ncs\v2.4.0\nrf\west.yml
add below line to bring in espressif HAL:
```yaml
        name-allowlist:
          - TraceRecorderSource
          - canopennode
          - chre
          - cmsis
          - edtt
          - fatfs
          - hal_espressif #add this line to support Espressif
          - hal_nordic
```
### for nRF SDK 2.4.0, ESP32S3 support is not fully ready:
we need to manually give the latest version of hal_espressif so that the project builds correctly.
So instead of adding hal_espressif as above, we should do this for nRF SDK v2.4.0:
```yaml
    #added by Shan to pull latest version of hal_espressif so that ESP32S3 WIFI works
    - name: hal_espressif
      revision: 623bfce170e74e80c83565c47d6f481e838b506e
      path: modules/hal/espressif
      west-commands: west/west-commands.yml
      groups:
        - hal
```
Note that Espressif HAL is frequently updated in recent weeks. always fetch the latest revision for new features.

Save the above file, go to "nRF toolchain", use "open the command prompt" to open the command line that includes all the toolchain environment variables, and run below command:
```sh
west update
```
Wait until above command is finished and the hal_espressif will be cloned to local repository.

### 2.2 add ESP32-S3 toolchain binary 
Next, manually download the ESP32-S3 toolchain and put it under the toolchain folder.
Check the file content of:
C:\Users\<your user name>\ncs\toolchains\31f4403e35\opt\zephyr-sdk\sdk_version
it has below text:
```
0.16.0
```
Go to Zephyr toolchain download page: https://github.com/zephyrproject-rtos/sdk-ng/releases/tag/v0.16.0
and download the windows toolchain for xtensa-espressif_esp32s3_zephyr-elf:
https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.16.0/toolchain_windows-x86_64_xtensa-espressif_esp32s3_zephyr-elf.7z
Unzip the download file into folder: C:\Users\<your user name>\ncs\toolchains\31f4403e35\opt\zephyr-sdk
Keep in mind that do not add additional folder layer during unzipping.
The folder structure after unzipping should be like:
```
zephyr-sdk
|
-xtensa-espressif_esp32s3_zephyr-elf
|	|
|	-bin, include, share...
-<other toolchain folders>
```
After finishing this section, the VS Code environment is ready to build Zephyr projects ESP32-S3.

### 2.3 fetch the blobs for Espressif:
Espressif includes blob library for WiFi BLE functions. To build/link Zephyr application correctly, these blob file shall exist in local Zephyr workspace.
run from nRF Connect Toolchain manager command prompt:
```
west blobs fetch hal_espressif
Fetching blob hal_espressif: C:\Users\310188132\ncs\v2.4.0\modules\hal\espressif\zephyr\blobs\lib\esp32c3\libbtdm_app.a
Fetching blob hal_espressif: C:\Users\310188132\ncs\v2.4.0\modules\hal\espressif\zephyr\blobs\lib\esp32s3\libbtdm_app.a
Fetching blob hal_espressif: C:\Users\310188132\ncs\v2.4.0\modules\hal\espressif\zephyr\blobs\lib\esp32\libbtdm_app.a
Fetching blob hal_espressif: C:\Users\310188132\ncs\v2.4.0\modules\hal\espressif\zephyr\blobs\lib\esp32\libnet80211.a
Fetching blob hal_espressif: C:\Users\310188132\ncs\v2.4.0\modules\hal\espressif\zephyr\blobs\lib\esp32\libpp.a
...
```
Wait for the command to finith.
This command will download all the Espressif MCU blob libraries.

# Build, flash and debug
## 1. Build project in VS Code
To build the code, it is recommended to do the initial build manually using command line:
in VS Code, nRF Connect tab, right click the project folder and select "start new terminal"
in the terminal, run the flash command below, note that the COM port number might change:
```sh
west build -p always -b esp32s3_devkitm
```
After the initial build, it is also possible to use the "build" button provided in the nRF Connect tab in VS Code.
## 2. flash ESP32S3 from VS Code
To flash the code, do not use the nRF Connect flash function. That flash function is for Nordic MCU and does not work for ESP32S3.
To flash the code on ESP32S3, use the west command from command line:
in VS Code, nRF Connect tab, right click the project folder and select "start new terminal"
in the terminal, run the flash command below, note that the COM port number might change:
```sh
west flash --esp-device COM8
```

## 3. Debug ESP32S3 from VS Code
Below steps are merged from these reference examples:
[https://gist.github.com/motla/ab7fdcf14303208996c40ca7fefe6f11](https://gist.github.com/motla/ab7fdcf14303208996c40ca7fefe6f11)
[https://github.com/espressif/vscode-esp-idf-extension/blob/master/docs/DEBUGGING.md](https://github.com/espressif/vscode-esp-idf-extension/blob/master/docs/DEBUGGING.md)

1.  Install 3 VSCode extensions:  [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools),  [Native Debug](https://marketplace.visualstudio.com/items?itemName=webfreak.debug),  [Task Explorer](https://marketplace.visualstudio.com/items?itemName=spmeesseman.vscode-taskexplorer)

2. Install the OpenOCD tool needed by ESP-IDF toolchain. run the below bat file from command line:
```sh
C:\Users\<your user name>\ncs\v2.4.0\modules\hal\espressif\install.bat
```
Note that OpenOCD is installed at:
```
C:\Users\<your user name>\.espressif\tools\openocd-esp32\v0.11.0-esp32-20220411\openocd-esp32
```
3. In the VS Code settings, Edit the lauch.json file:
```json
{
    // Use IntelliSense to learn about possible attributes.
    // Hover to view descriptions of existing attributes.
    // For more information, visit: https://go.microsoft.com/fwlink/?linkid=830387
    "version": "0.2.0",
    "configurations": [
      {
        "name": "ESP32S3 Working Debug",
        "type": "cppdbg",
        "request": "launch",
        "MIMode": "gdb",
        "miDebuggerPath": "C:\\Users\\<you_user_name>\\ncs\\toolchains\\31f4403e35\\opt\\zephyr-sdk\\xtensa-espressif_esp32s3_zephyr-elf\\bin\\xtensa-espressif_esp32s3_zephyr-elf-gdb.exe",
        "program": "${workspaceFolder}/build/${command:espIdf.getProjectName}.elf",
        "windows": {
          "program": "${workspaceFolder}\\build\\zephyr\\zephyr.elf"
        },
        "cwd": "${workspaceFolder}",
        "environment": [{ "name": "PATH", "value": "${config:idf.customExtraPaths}" }],
        "setupCommands": [
          { "text": "target remote :3333" },
          { "text": "set remote hardware-watchpoint-limit 2"},
          { "text": "mon reset halt" },
          { "text": "thb main" },
          { "text": "flushregs" }
        ],
        "externalConsole": false,
        "logging": {
          "engineLogging": true
        },
        "preLaunchTask": "OpenOCD"
      }
    ]
}
```
4. In VS Code settings, edit the task.json file:
```json
{
    // See https://go.microsoft.com/fwlink/?LinkId=733558
    // for the documentation about the tasks.json format
    "version": "2.0.0",
    "tasks": [
        {
            "label": "OpenOCD",
            "type": "shell",
            "command": ". $HOME/esp/esp-idf/export.sh && openocd -f interface/ftdi/esp32_devkitj_v1.cfg -f board/esp32-wrover.cfg",
            "windows": { "command": "C:\\Users\\<you_user_name>\\ncs\\v2.4.0\\modules\\hal\\espressif\\export.bat && C:\\Users\\<you_user_name>\\.espressif\\tools\\openocd-esp32\\v0.11.0-esp32-20220411\\openocd-esp32\\bin\\openocd -f board/esp32s3-builtin.cfg" },
            "isBackground": true,
            "problemMatcher": {
              "pattern": {
                "regexp": "^(Info |Warn ):(.*)$", // Ignore errors
                "severity": 1,
                "message": 2
              },
              "background": {
                "activeOnStart": true,
                "beginsPattern": ".",
                "endsPattern": "Info : Listening on port \\d+ for gdb connections"
              }
            }
          }
    ]
}
```
Note that all the json files <you_user_name> shall be replaced by the actual user name in your PC.

After this, the program debug shall work. Note that do not use the "debug" button in the nRF Connect tab, always use the "Run and Debug" tab from VS Code.

## ESP32S3 support UART over USB and JTAG over USB.
Note that this only applies for nRF SDK v2.4.1. ESP32S3 is newly added into Zephyr and support level will improve in the future.
Current Zephyr version comes with nRF SDK 2.4.1 does not have USB support by default. some manual modification is needed to enable this support.
In file <nrf sdk install folder>/ncs/v2.4.1/zephyr/dts/xtensa/espressif/esp32s3.dtsi
Add below modification to existing Zephyr code tree 2.4.1 to support USB JTAG debugging:
```
    //add USB serial support
		usb_serial: uart@60038000 {
			compatible = "espressif,esp32-usb-serial";
			reg = <0x60038000 DT_SIZE_K(4)>;
			status = "disabled";
			interrupts = <USB_SERIAL_JTAG_INTR_SOURCE>;
			interrupt-parent = <&intc>;
			clocks = <&rtc ESP32_USB_MODULE>;
		};
		
		timer0: counter@6001f000 {
			compatible = "espressif,esp32-timer";
			reg = <0x6001f000 DT_SIZE_K(4)>;
			group = <0>;
			index = <0>;
			interrupts = <TG0_T0_LEVEL_INTR_SOURCE>;
			interrupt-parent = <&intc>;
			status = "disabled";
		};
```

The in board board_defconfig file, enable the feature:
```
#this line makes the USB JTAG debug enabled
CONFIG_SERIAL_ESP32_USB=y
```

and in device tree file boards/xtensa/<board_name>/<board_name>.dts, enable the device:
```
&usb_serial {
	status = "okay";
};
```
