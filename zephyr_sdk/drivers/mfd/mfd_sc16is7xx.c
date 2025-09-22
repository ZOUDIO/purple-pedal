#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mfd/sc16is7xx.h>
#include <zephyr/logging/log.h>

#include "mfd_sc16is7xx.h"

LOG_MODULE_REGISTER(mfd_sc16is7xx, CONFIG_MFD_LOG_LEVEL);

//read: https://docs.zephyrproject.org/latest/build/dts/bindings-syntax.html#child-binding
//read https://github.com/zephyrproject-rtos/zephyr/blob/main/samples/drivers/adc/adc_dt/boards/apollo3_evb.overlay
//read https://github.com/zephyrproject-rtos/zephyr/blob/e885c8c42f775f92176e7ad7eb71d8dcdc2bb4e0/dts/bindings/adc/adc-controller.yaml#L20C1-L20C14
//read https://github.com/zephyrproject-rtos/zephyr/blob/90919eb60b973ea09f61e60c8351b7b6d1a01a3a/dts/bindings/input/futaba%2Csbus.yaml

int mfd_sc16is7xx_read_data(const struct device *dev, const uint8_t reg_addr, uint8_t *val, const size_t len)
{
	struct mfd_sc16is7xx_data *data = dev->data;
	return data->transfer_function->read_data(dev, reg_addr, val, len);
}

int mfd_sc16is7xx_write_data(const struct device *dev, const uint8_t reg_addr, const uint8_t *val, const size_t len)
{
	struct mfd_sc16is7xx_data *data = dev->data;
	return data->transfer_function->write_data(dev, reg_addr, val, len);
}

int mfd_sc16is7xx_read_reg(const struct device *dev, const uint8_t reg_addr, uint8_t *val)
{
	struct mfd_sc16is7xx_data *data = dev->data;

	return data->transfer_function->read_reg(dev, reg_addr, val);
}

int mfd_sc16is7xx_write_reg(const struct device *dev, const uint8_t reg, const uint8_t val)
{
	struct mfd_sc16is7xx_data *data = dev->data;

	return data->transfer_function->write_reg(dev, reg, val);
}

int mfd_sc16is7xx_manage_interrupt_callback(const struct device *dev, struct sc16is7xx_interrupt_callback *callback, bool set)
{
	//add to
	struct mfd_sc16is7xx_data *data = dev->data;
	sys_slist_t *callbacks = &data->interrupt_callbacks;
	if (!sys_slist_is_empty(callbacks)) {
		if (!sys_slist_find_and_remove(callbacks, &callback->node)) {
			if (!set) {
				return -EINVAL;
			}
		}
	} else if (!set) {
		return -EINVAL;
	}

	if (set) {
		sys_slist_prepend(callbacks, &callback->node);
	}

	return 0;
}

uint32_t mfd_sc16is7xx_get_clock_freq(const struct device *dev)
{
	const struct mfd_sc16is7xx_config * const config = dev->config;
	return config->clock_freq;
}

static void sc16is7xx_interrupt_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	//determine the type of interrupt, and call the registered handler
	//there is only one interruption line on the chip, for multiple UART channels,
	//we should receive interrupt here and distribute it to each UART channel's driver
	//interrupt handling function should also consider GPIO ralated interruption
	struct mfd_sc16is7xx_data *data = CONTAINER_OF(cb, struct mfd_sc16is7xx_data, interrupt_cb_data);

	struct sc16is7xx_interrupt_callback *sc16is7xx_cb, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&data->interrupt_callbacks, sc16is7xx_cb, tmp, node) {
			__ASSERT(cb->handler, "No callback handler!");
			sc16is7xx_cb->handler(sc16is7xx_cb->dev);
	}
	//this implementation resembles gpio_fire_callbacks() in gpio_utils.h
}

static int mfd_sc16is7xx_init(const struct device *dev)
{
	//issue reset
	//setup interrupt callback
	//call the bus_init function
	const struct mfd_sc16is7xx_config *config = dev->config;
	struct mfd_sc16is7xx_data *data = dev->data;
	int ret = config->bus_init(dev);
	if(ret< 0) return ret;
	if (!gpio_is_ready_dt(&config->reset_gpio)) {
		return -ENODEV;
	}
#ifdef CONFIG_UART_INTERRUPT_DRIVEN //TODO: UART GPIO can all enable this. need to revise the condition here
	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("Error: int_gpio device %s is not ready\n", config->int_gpio.port->name);
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if(ret != 0){
		LOG_ERR("gpio_pin_configure_dt() returns %d", ret);
		return -EINVAL;
	}
	ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if(ret != 0){
		LOG_ERR("gpio_pin_interrupt_configure_dt() returns %d", ret);
		return -EINVAL;
	}
	gpio_init_callback(&data->interrupt_cb_data, sc16is7xx_interrupt_handler, BIT(config->int_gpio.pin));
	gpio_add_callback(config->int_gpio.port, &data->interrupt_cb_data);
#endif //CONFIG_UART_INTERRUPT_DRIVEN
	return 0;
}

#define SC16IS7XX_DEFINE_SPI(inst) \
	static struct mfd_sc16is7xx_config mfd_sc16is7xx_config_##inst = {\
		.i2c = I2C_DT_SPEC_INST_GET(inst),\
		.bus_init = mfd_sc16is7xx_spi_init,\
		.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, interrupt_gpios, {}),\
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {}),\
		.clock_freq = DT_INST_PROP(inst, clock_frequency),\
	};\
	DEVICE_DT_INST_DEFINE(inst,\
		&mfd_sc16is7xx_init,\
		NULL,\
		NULL,\
		&mfd_sc16is7xx_config_##inst,\
		POST_KERNEL,\
		CONFIG_MFD_SC16IS7XX_INIT_PRIORITY,\
		NULL);

#define SC16IS7XX_DEFINE_I2C(inst) \
	static struct mfd_sc16is7xx_config mfd_sc16is7xx_config_##inst = {\
		.i2c = I2C_DT_SPEC_INST_GET(inst),\
		.bus_init = mfd_sc16is7xx_i2c_init,\
	};\
	DEVICE_DT_INST_DEFINE(inst,\
		&mfd_sc16is7xx_init,\
		NULL,\
		NULL,\
		&mfd_sc16is7xx_config_##inst,\
		POST_KERNEL,\
		CONFIG_MFD_SC16IS7XX_INIT_PRIORITY,\
		NULL);

#define SC16IS7XX_DEFINE(inst) \
	COND_CODE_1(DT_INST_ON_BUS(inst, spi), (SC16IS7XX_DEFINE_SPI(inst)), (SC16IS7XX_DEFINE_I2C(inst)))

DT_INST_FOREACH_STATUS_OKAY(SC16IS7XX_DEFINE)