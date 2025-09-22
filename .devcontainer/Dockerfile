FROM ghcr.io/zephyrproject-rtos/zephyr-build:v0.28.3

# ARG UID=1000
# ARG GID=1000

ENV ZEPHYR_SDK=/opt/toolchains/zephyr-sdk-${ZSDK_VERSION}

ENV ESPRESSIF_TOOLCHAIN_PATH=/opt/espressif
ENV INFINEON_TOOLCHAIN_PATH=/opt/infineon

USER root

RUN usermod -a -G dialout user && \
	usermod -a -G adm user && \
	usermod -a -G sudo user

# Running systemd is really needed for Jlink to work correctly
# refer to: https://learn.microsoft.com/en-us/windows/wsl/wsl-config#wslconfig
ADD .devcontainer/wsl.conf /etc/wsl.conf
RUN chmod 644 /etc/wsl.conf


# Install jlink and other useful tools
RUN apt-get -y update && \
	apt-get -y install \
	iptables \
	dnsmasq \
	gdbserver \
	udev \
	usbutils \
	screen \
	stlink-tools\
    libxkbcommon-x11-0 \
	libxcb-image0 \
	libxcb-keysyms1 \
	libxcb-icccm4 \
	libxcb-shape0 \
	libxcb-render-util0 \
	dotnet-sdk-8.0
RUN wget --post-data "accept_license_agreement=accepted" https://www.segger.com/downloads/jlink/JLink_Linux_x86_64.deb && \
	dpkg --unpack ./JLink_Linux_x86_64.deb && \
	rm -f /var/lib/dpkg/info/jlink.postinst && \
	dpkg --configure jlink && \
	apt install -yf && \
	rm ./JLink_Linux_x86_64.deb && \
	exit 0

# install openocd-esp32
RUN wget https://github.com/xpack-dev-tools/openocd-xpack/releases/download/v0.12.0-4/xpack-openocd-0.12.0-4-linux-x64.tar.gz && \
	mkdir ${ESPRESSIF_TOOLCHAIN_PATH} && \	
	tar -xvzf xpack-openocd-0.12.0-4-linux-x64.tar.gz -C ${ESPRESSIF_TOOLCHAIN_PATH} && \
	rm ./xpack-openocd-0.12.0-4-linux-x64.tar.gz && \
	wget https://github.com/espressif/openocd-esp32/releases/download/v0.12.0-esp32-20241016/openocd-esp32-linux-amd64-0.12.0-esp32-20241016.tar.gz && \
	tar -xvzf openocd-esp32-linux-amd64-0.12.0-esp32-20241016.tar.gz -C ${ESPRESSIF_TOOLCHAIN_PATH} && \
	rm ./openocd-esp32-linux-amd64-0.12.0-esp32-20241016.tar.gz

# install infineon openocd
RUN	wget https://github.com/Infineon/openocd/releases/download/release-v5.8.0/openocd-5.8.0.3960-linux.tar.gz && \
	mkdir ${INFINEON_TOOLCHAIN_PATH} && \		
	tar -xvzf openocd-5.8.0.3960-linux.tar.gz -C ${INFINEON_TOOLCHAIN_PATH} && \
	rm ./openocd-5.8.0.3960-linux.tar.gz && \
	cp /opt/infineon/openocd/udev_rules/66-wiced-JTAG.rules /etc/udev/rules.d/66-wiced-JTAG.rules

# below modification is not really needed for Jlink to work
# Hack udev to run inside a container
# https://stackoverflow.com/questions/62060604/why-udev-init-script-default-disable-container-support-while-in-fact-it-works
# RUN sed -i.bak -e '/if \[ ! -w \/sys \]/,+3 s/^/#/' /etc/init.d/udev

#setup openocd rules
WORKDIR "${ZEPHYR_SDK}"
RUN cp ./sysroots/x86_64-pokysdk-linux/usr/share/openocd/contrib/60-openocd.rules /etc/udev/rules.d/60-openocd.rules

#install strictdoc tool for requirement management
RUN pip3 install strictdoc

USER user

ENV ESPRESSIF_OPENOCD_PATH=${ESPRESSIF_TOOLCHAIN_PATH}/xpack-openocd-0.12.0-4

ENV CONNECTIVITY_NODE_SDK_BASE=/workdir/connectivity_node_sdk

# ENV ZEPHYR_BASE=/workspaces/zephyr