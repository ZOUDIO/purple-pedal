#ifndef ZEPHYR_DRIVERS_MFD_SC16IS7XX_H_
#define ZEPHYR_DRIVERS_MFD_SC16IS7XX_H_

#ifdef __cplusplus
extern "C" {
#endif

#define DT_DRV_COMPAT nxp_sc16is7xx

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#include <zephyr/drivers/mfd/sc16is7xx.h>

// #define AD559X_GPIO_READBACK_EN         BIT(10)
// #define AD559X_LDAC_READBACK_EN         BIT(6)
// #define AD559X_REG_SOFTWARE_RESET       0x0FU
// #define AD559X_SOFTWARE_RESET_MAGIC_VAL 0x5AC
// #define AD559X_REG_VAL_MASK             0x3FF
// #define AD559X_REG_RESET_VAL_MASK       0x7FF
// #define AD559X_REG_SHIFT_VAL            11
// #define AD559X_REG_READBACK_SHIFT_VAL   2


struct mfd_sc16is7xx_transfer_function {
	int (*read_data)(const struct device *dev, const uint8_t reg_addr, uint8_t *val, const size_t len);
	int (*write_data)(const struct device *dev, const uint8_t reg_addr, const uint8_t *val, const size_t len);
	int (*read_reg)(const struct device *dev, const uint8_t reg_addr, uint8_t *val);
	int (*write_reg)(const struct device *dev, const uint8_t reg_addr, const uint8_t val);
};

struct mfd_sc16is7xx_config {
	int (*bus_init)(const struct device *dev);
	const union {
		#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
			struct i2c_dt_spec i2c;
		#endif
		
		#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
			struct spi_dt_spec spi;
		#endif
	};
	struct gpio_dt_spec int_gpio;
	struct gpio_dt_spec reset_gpio;
    uint32_t clock_freq;
};

struct mfd_sc16is7xx_data {
	const struct mfd_sc16is7xx_transfer_function *transfer_function;
    struct gpio_callback interrupt_cb_data;
    sys_slist_t interrupt_callbacks;
};

int mfd_sc16is7xx_i2c_init(const struct device *dev);
int mfd_sc16is7xx_spi_init(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MFD_SC16IS7XX_H_*/