zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/uart.h)

# out of tree driver is amended to zephyr's existing drivers/serial library
zephyr_library_amend()

zephyr_library_sources_ifdef(CONFIG_UART_SC16IS7XX uart_sc16is7xx.c)