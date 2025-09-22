#include "mfd_sc16is7xx.h"

static int mfd_sc16is7xx_i2c_read_data(const struct device *dev, const uint8_t reg_addr, uint8_t *val, const size_t len)
{
	const struct mfd_sc16is7xx_config *config = dev->config;
	return i2c_burst_read_dt(&config->i2c, reg_addr, val, len);
}

static int mfd_sc16is7xx_i2c_write_data(const struct device *dev, const uint8_t reg_addr, const uint8_t *val, const size_t len)
{
	const struct mfd_sc16is7xx_config *config = dev->config;
	return i2c_burst_write_dt(&config->i2c, reg_addr, val, len);
}

static int mfd_sc16is7xx_i2c_read_reg(const struct device *dev, const uint8_t reg_addr, uint8_t *val)
{
    const struct mfd_sc16is7xx_config *config = dev->config;
	return i2c_burst_read_dt(&config->i2c, reg_addr, val, 1);
}

static int mfd_sc16is7xx_i2c_write_reg(const struct device *dev, const uint8_t reg_addr, const uint8_t val)
{
	const struct mfd_sc16is7xx_config *config = dev->config;
	return i2c_burst_write_dt(&config->i2c, reg_addr, &val, sizeof(val));
}

static const struct mfd_sc16is7xx_transfer_function mfd_sc16is7xx_i2c_transfer_function = {
	.read_data = mfd_sc16is7xx_i2c_read_data,
	.write_data = mfd_sc16is7xx_i2c_write_data,
	.read_reg = mfd_sc16is7xx_i2c_read_reg,
	.write_reg = mfd_sc16is7xx_i2c_write_reg,
};

int mfd_sc16is7xx_i2c_init(const struct device *dev)
{
    const struct mfd_sc16is7xx_config *config = dev->config;
	struct mfd_sc16is7xx_data *data = dev->data;

	data->transfer_function = &mfd_sc16is7xx_i2c_transfer_function;

	if (!i2c_is_ready_dt(&config->i2c)) {
		return -ENODEV;
	}
    return 0;
}