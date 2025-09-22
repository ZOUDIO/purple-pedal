#include "mfd_sc16is7xx.h"

static int mfd_sc16is7xx_spi_raw_read(const struct device *dev, const uint8_t reg_addr, uint8_t *value, const uint8_t len)
{
    const struct mfd_sc16is7xx_config *config = dev->config;
    uint8_t write_reg_addr = reg_addr;
    REG_ADDR_SPI(write_reg_addr, false);

    uint8_t buffer_tx[2] = { write_reg_addr, 0 };
    const struct spi_buf tx_buf = {
        .buf = buffer_tx,
        .len = 2,
    };
    const struct spi_buf_set tx = {
        .buffers = &tx_buf,
        .count = 1};

    const struct spi_buf rx_buf[2] = {
        {
            .buf = NULL,
            .len = 1,
        },
        {
            .buf = value,
            .len = len,
        }};
    const struct spi_buf_set rx = {
        .buffers = rx_buf,
        .count = 2};

    if (spi_transceive_dt(&config->spi, &tx, &rx))
    {
        return -EIO;
    }

    return 0;
}

static int mfd_sc16is7xx_spi_raw_write(const struct device *dev, const uint8_t reg_addr, const uint8_t *value, const uint8_t len)
{
    const struct mfd_sc16is7xx_config *config = dev->config;
    uint8_t read_reg_addr = reg_addr;
    REG_ADDR_SPI(read_reg_addr, true);

    uint8_t buffer_tx[1] = { read_reg_addr };
	const struct spi_buf tx_buf[2] = {
		{
			.buf = buffer_tx,
			.len = 1,
		},
		{
			.buf = (void*)value,
			.len = len,
		}
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = 2
	};
    
	if (spi_write_dt(&config->spi, &tx)) {
		return -EIO;
	}

	return 0;    
}

static int mfd_sc16is7xx_spi_read_data(const struct device *dev, const uint8_t reg_addr, uint8_t *val, const size_t len)
{
	return mfd_sc16is7xx_spi_raw_read(dev, reg_addr, val, len);
}

static int mfd_sc16is7xx_spi_write_data(const struct device *dev, const uint8_t reg_addr, const uint8_t *val, const size_t len)
{
	return mfd_sc16is7xx_spi_raw_write(dev, reg_addr, val, len);
}

static int mfd_sc16is7xx_spi_read_reg(const struct device *dev, const uint8_t reg_addr, uint8_t *val)
{
	return mfd_sc16is7xx_spi_raw_read(dev, reg_addr, val, 1);
}

static int mfd_sc16is7xx_spi_write_reg(const struct device *dev, const uint8_t reg_addr, const uint8_t val)
{
	return mfd_sc16is7xx_spi_raw_write(dev, reg_addr, &val, sizeof(val));
}

static const struct mfd_sc16is7xx_transfer_function mfd_sc16is7xx_spi_transfer_function = {
	.read_data = mfd_sc16is7xx_spi_read_data,
	.write_data = mfd_sc16is7xx_spi_write_data,
	.read_reg = mfd_sc16is7xx_spi_read_reg,
	.write_reg = mfd_sc16is7xx_spi_write_reg,
};

int mfd_sc16is7xx_spi_init(const struct device *dev)
{
    const struct mfd_sc16is7xx_config *config = dev->config;
	struct mfd_sc16is7xx_data *data = dev->data;

	data->transfer_function = &mfd_sc16is7xx_spi_transfer_function;

	if (!spi_is_ready_dt(&config->spi)) {
		return -ENODEV;
	}
    return 0;
}