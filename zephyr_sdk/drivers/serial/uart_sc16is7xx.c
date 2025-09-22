#define DT_DRV_COMPAT nxp_sc16is7xx_uart

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/mfd/sc16is7xx.h>
#include <zephyr/logging/log.h>


LOG_MODULE_REGISTER(uart_sc16is7xx, CONFIG_UART_LOG_LEVEL);

struct uart_sc16is7xx_config {
	const struct device *mfd_dev;
	const uint8_t channel;
};

struct uart_sc16is7xx_data {
	struct uart_config uart_config;
	struct k_spinlock lock;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	struct sc16is7xx_interrupt_callback callback;
	uint8_t iir_cache;	/**< cache of IIR since it clears when read */
	uart_irq_callback_user_data_t cb;  /**< Callback function pointer */
	void *cb_data;	/**< Callback function arg */
#endif
};


static int set_baudrate(const struct device *dev, uint32_t baudrate)
{
	struct uart_sc16is7xx_data * const data = dev->data;
	const struct uart_sc16is7xx_config * const config = dev->config;
	uint32_t clock_freq = mfd_sc16is7xx_get_clock_freq(config->mfd_dev);
	int ret;
	if((baudrate==0U)||(clock_freq==0U))
		return -EINVAL;
	
	// calculate the needed divisor
	// need to buffer the LCR register information
	uint32_t tgt_clk_freq = (baudrate << 4);
	uint32_t divisor = DIV_ROUND_CLOSEST(clock_freq, tgt_clk_freq);
	bool prescaler = false;
	if (divisor > BIT_MASK(18))
		return -EINVAL;
	if (divisor > BIT_MASK(16))
	{
		prescaler = true;
		divisor = (divisor >> 2);
	}
	if (prescaler)
	{
		uint8_t mcr = 0;
		uint8_t mcr_addr = REG_ADDR_I2C(REG_MCR, config->channel);
		ret = mfd_sc16is7xx_read_reg(config->mfd_dev, mcr_addr, &mcr);
		if (ret)
			return -EIO;
		mcr |= REG_MCR_BIT_CLK_DIV;
		// WRITE_BIT(mcr, REG_MCR_BIT_CLK_DIV, true);
		ret = mfd_sc16is7xx_write_reg(config->mfd_dev, mcr_addr, mcr);
		if (ret)
			return -EIO;
	}

	uint8_t lcr = 0;
	uint8_t lcr_addr = REG_ADDR_I2C(REG_LCR, config->channel);
	uint8_t dll_addr = REG_ADDR_I2C(REG_DLL, config->channel);
	uint8_t dlh_addr = REG_ADDR_I2C(REG_DLH, config->channel);

	ret = mfd_sc16is7xx_read_reg(config->mfd_dev, lcr_addr, &lcr);
	if (ret)
		return -EIO;
	// enable divisor latch registers
	// lcr |= REG_LCR_BIT_DIV_LCH_EN;
	// WRITE_BIT(lcr, REG_LCR_BIT_DIV_LCH_EN, true);
	ret = mfd_sc16is7xx_write_reg(config->mfd_dev, lcr_addr, REG_LCR_BIT_DIV_LCH_EN); // this can avoid the value happens to be 0xBF
	if (ret)
		return -EIO;

	ret = mfd_sc16is7xx_write_reg(config->mfd_dev, dll_addr, (divisor & BIT_MASK(8)));
	if (ret)
		return -EIO;

	ret = mfd_sc16is7xx_write_reg(config->mfd_dev, dlh_addr, ((divisor & BIT_MASK(8)) >> 8));
	if (ret)
		return -EIO;

	// disable divisor latch registers
	lcr &= !(REG_LCR_BIT_DIV_LCH_EN);
	// WRITE_BIT(lcr, REG_LCR_BIT_DIV_LCH_EN, false);
	ret = mfd_sc16is7xx_write_reg(config->mfd_dev, lcr_addr, lcr);
	if (ret)
		return -EIO;

	data->uart_config.baudrate = baudrate;
	return 0;
}

static int set_databits_parity_stopbits(const struct device *dev, uint8_t data_bits, uint8_t parity, uint8_t stop_bits)
{
	struct uart_sc16is7xx_data * const data = dev->data;
	const struct uart_sc16is7xx_config * const config = dev->config;
	

	uint8_t data_bits_val = (data_bits == UART_CFG_DATA_BITS_5) ? REG_LCR_DATA_BITS_5 :
							(data_bits == UART_CFG_DATA_BITS_6) ? REG_LCR_DATA_BITS_6 :
							(data_bits == UART_CFG_DATA_BITS_7) ? REG_LCR_DATA_BITS_7 :
							(data_bits == UART_CFG_DATA_BITS_8) ? REG_LCR_DATA_BITS_8 : 0xFF;
	if(data_bits_val == 0xFF) return -EINVAL;

	uint8_t parity_val = (parity == UART_CFG_PARITY_NONE) ? REG_LCR_PARITY_NONE :
						(parity == UART_CFG_PARITY_ODD) ? REG_LCR_PARITY_ODD :
						(parity == UART_CFG_PARITY_EVEN) ? REG_LCR_PARITY_EVEN :
						(parity == UART_CFG_PARITY_MARK) ? REG_LCR_PARITY_MARK :
						(parity == UART_CFG_PARITY_SPACE) ? REG_LCR_PARITY_SPACE : 0xFF;
	if(parity_val == 0xFF) return -EINVAL;

	//according to datasheet, not all stop bits / data bits combinations are allowed
	bool stop_bit_1_allowed = (data_bits == UART_CFG_DATA_BITS_5) ||
							(data_bits == UART_CFG_DATA_BITS_6) ||
							(data_bits == UART_CFG_DATA_BITS_7) ||
							(data_bits == UART_CFG_DATA_BITS_8);

	bool stop_bit_1_5_allowed = (data_bits == UART_CFG_DATA_BITS_5);

	bool stop_bit_2_allowed = (data_bits == UART_CFG_DATA_BITS_6) ||
							(data_bits == UART_CFG_DATA_BITS_7) ||
							(data_bits == UART_CFG_DATA_BITS_8);

	uint8_t stop_bits_val = (stop_bits == UART_CFG_STOP_BITS_0_5) ? 0xFF :
							((stop_bits == UART_CFG_STOP_BITS_1)&&stop_bit_1_allowed) ? REG_LCR_STOP_BITS_1 :
							((stop_bits == UART_CFG_STOP_BITS_1_5)&&stop_bit_1_5_allowed) ? REG_LCR_STOP_BITS_1_5 :
							((stop_bits == UART_CFG_STOP_BITS_2)&&stop_bit_2_allowed) ? REG_LCR_STOP_BITS_2 : 0xFF;
	if(stop_bits_val == 0xff) return -EINVAL;

	int ret;
	uint8_t lcr = 0;
	uint8_t lcr_addr = REG_ADDR_I2C(REG_LCR, config->channel);
	ret = mfd_sc16is7xx_read_reg(config->mfd_dev, lcr_addr, &lcr);
	if(ret) return -EIO;
	
	lcr = (lcr & ~((REG_LCR_BIT_PARITY) | (REG_LCR_BIT_STOP_BITS) | (REG_LCR_BIT_DATA_BITS)))
			| (parity_val)|(stop_bits_val)|(data_bits_val);

	ret = mfd_sc16is7xx_write_reg(config->mfd_dev, lcr_addr, lcr);
	if(ret) return -EIO;

	data->uart_config.data_bits = data_bits;
	data->uart_config.parity = parity;
	data->uart_config.stop_bits = stop_bits;
	return 0;
}

static int set_hw_flow_ctrl(const struct device *dev, uint8_t hw_flow_ctrl)
{
	struct uart_sc16is7xx_data * const data = dev->data;
	const struct uart_sc16is7xx_config * const config = dev->config;
	int ret;

	uint8_t lcr = 0;
	uint8_t lcr_addr = REG_ADDR_I2C(REG_LCR, config->channel);
	ret = mfd_sc16is7xx_read_reg(config->mfd_dev, lcr_addr, &lcr);
	if (ret)
		return -EIO;
	//enable EFR writing
	ret = mfd_sc16is7xx_write_reg(config->mfd_dev, lcr_addr, 0xBF);
	if (ret)
		return -EIO;

	// update EFR
	uint8_t efr = 0;
	uint8_t efr_adr = REG_ADDR_I2C(REG_EFR, config->channel);
	ret = mfd_sc16is7xx_read_reg(config->mfd_dev, efr_adr, &efr);
	if (ret)
		return -EIO;

	if(hw_flow_ctrl)
		efr |= (REG_EFR_BIT_AUTO_CTS|REG_EFR_BIT_AUTO_CTS);
	else
		efr &= ~(REG_EFR_BIT_AUTO_CTS|REG_EFR_BIT_AUTO_CTS);

	ret = mfd_sc16is7xx_write_reg(config->mfd_dev, efr_adr, efr);
	if (ret)
		return -EIO;

	//restore lcr to prevent EFR writing
	ret = mfd_sc16is7xx_write_reg(config->mfd_dev, lcr_addr, lcr);
	if (ret)
		return -EIO;

	data->uart_config.flow_ctrl = hw_flow_ctrl;
	return 0;
}

static int set_fifo(const struct device *dev)
{
	//struct uart_sc16is7xx_data * const data = dev->data;
	const struct uart_sc16is7xx_config * const config = dev->config;
	int ret;

	uint8_t fcr = (REG_FCR_RX_TRIGGER_32)|(REG_FCR_TX_TRIGGER_32)|
				(REG_FCR_TX_FIFO_RESET)|(REG_FCR_RX_FIFO_RESET)|(REG_FCR_FIFO_EN);
	uint8_t fcr_addr = REG_ADDR_I2C(REG_FCR, config->channel);
	ret = mfd_sc16is7xx_write_reg(config->mfd_dev, fcr_addr, fcr);
	if (ret)
		return -EIO;
	
	return 0;
}

static int set_interrupt(const struct device *dev, uint8_t ier_set_mask, uint8_t ier_clear_mask)
{
	//struct uart_sc16is7xx_data * const data = dev->data;
	const struct uart_sc16is7xx_config * const config = dev->config;
	int ret;

	uint8_t ier = 0;
	uint8_t ier_addr = REG_ADDR_I2C(REG_IER, config->channel);
	ret = mfd_sc16is7xx_read_reg(config->mfd_dev, ier_addr, &ier);
	if(ret)
		return -EIO;
	
	ier = (ier | ier_set_mask) & !ier_clear_mask;
	ret = mfd_sc16is7xx_write_reg(config->mfd_dev, ier_addr, ier);
	if(ret)
		return -EIO;
	
	return 0;
}

static int uart_sc16is7xx_poll_in(const struct device *dev, unsigned char *c)
{	
	//return 0 if a character arrived, -1 if the input buffer if empty.
	struct uart_sc16is7xx_data *data = dev->data;
	const struct uart_sc16is7xx_config * const config = dev->config;
	int ret;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	uint8_t lsr = 0;
	uint8_t lsr_addr = REG_ADDR_I2C(REG_LSR, config->channel);
	ret = mfd_sc16is7xx_read_reg(config->mfd_dev, lsr_addr, &lsr);
	if(ret){
		k_spin_unlock(&data->lock, key);
		return -1;
	}	

	if(!(lsr & REG_LSR_BIT_DATA_IN_RCV)){
		k_spin_unlock(&data->lock, key);
		return -1;
	}

	uint8_t rhr;
	uint8_t rhr_addr = REG_ADDR_I2C(REG_RHR, config->channel);
	ret = mfd_sc16is7xx_read_reg(config->mfd_dev, rhr_addr, &rhr);
	if (ret){
		k_spin_unlock(&data->lock, key);
		return -1;
	}
	*c = rhr;
	k_spin_unlock(&data->lock, key);
	return ret;
}

static void uart_sc16is7xx_poll_out(const struct device *dev, unsigned char c)
{
	struct uart_sc16is7xx_data *data = dev->data;
	const struct uart_sc16is7xx_config * const config = dev->config;
	int ret;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	uint8_t lsr = 0;
	uint8_t lsr_addr = REG_ADDR_I2C(REG_LSR, config->channel);
	do{
		ret = mfd_sc16is7xx_read_reg(config->mfd_dev, lsr_addr, &lsr);
		if(ret){
			k_spin_unlock(&data->lock, key);
			return;
		}	
	}while(!(lsr & REG_LSR_BIT_THR_EMPTY));

	uint8_t thr_addr = REG_ADDR_I2C(REG_THR, config->channel);;
	ret = mfd_sc16is7xx_write_reg(config->mfd_dev, thr_addr, c);

	k_spin_unlock(&data->lock, key);
}

static int uart_sc16is7xx_err_check(const struct device *dev)
{
	//return one of UART_ERROR_OVERRUN, UART_ERROR_PARITY, 
	//UART_ERROR_FRAMING, UART_BREAK if an error was detected, 0 otherwise.
	struct uart_sc16is7xx_data *data = dev->data;
	const struct uart_sc16is7xx_config * const config = dev->config;
	int ret;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	uint8_t lsr = 0;
	uint8_t lsr_addr = REG_ADDR_I2C(REG_LSR, config->channel);
	ret = mfd_sc16is7xx_read_reg(config->mfd_dev, lsr_addr, &lsr);
	if(ret){
		k_spin_unlock(&data->lock, key);
		return -EIO;
	}

	int err_check = lsr & ((REG_LSR_BIT_BREAKK_INT) | (REG_LSR_BIT_FRAME_ERR) | 
					(REG_LSR_BIT_PARITY_ERR) | (REG_LSR_BIT_OVERRUN_ERR));

	k_spin_unlock(&data->lock, key);

	return err_check;
}

static int uart_sc16is7xx_configure(const struct device *dev,const struct uart_config *cfg)
{
	struct uart_sc16is7xx_data * const data = dev->data;
	//const struct uart_sc16is7xx_config * const config = dev->config;

	k_spinlock_key_t key = k_spin_lock(&data->lock);


	//set baudrate
	int ret = set_baudrate(dev, cfg->baudrate);
	if(ret){
		return ret;
	}
	//set databits, parity and stopbits together, they are in the same register
	ret = set_databits_parity_stopbits(dev, cfg->data_bits, cfg->parity, cfg->stop_bits);
	if(ret){
		return ret;
	}

	//set hw_flow_ctrl
	ret = set_hw_flow_ctrl(dev, cfg->flow_ctrl);
	if(ret){
		return ret;
	}

	ret = set_fifo(dev);
	if(ret){
		return ret;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	data->iir_cache = 0U;
	//set up fifo operation, fifo does NOT need to be coupled with interrupt?
	//FCR register sets both fifo level and fifo enable

	//set up interrupt
	//IER register sets interrupt

	//this part shall be done in init function, AFTER configure() call
#endif

	//disable all interrrupt
	ret = set_interrupt(dev, 0, 0xff);
	if(ret){
		return ret;
	}
	k_spin_unlock(&data->lock, key);
	return 0;
}

static int uart_sc16is7xx_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_sc16is7xx_data *data = dev->data;

	cfg->baudrate = data->uart_config.baudrate;
	cfg->parity = data->uart_config.parity;
	cfg->stop_bits = data->uart_config.stop_bits;
	cfg->data_bits = data->uart_config.data_bits;
	cfg->flow_ctrl = data->uart_config.flow_ctrl;

	return 0;
}

static int uart_sc16is7xx_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	//write data to fifo
	struct uart_sc16is7xx_data *data = dev->data;
	const struct uart_sc16is7xx_config * const config = dev->config;
	int ret;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	//get number of empty slots in fifo
	//write out bytes shall not exceed num of empty slots
	uint8_t txlvl = 0;
	uint8_t txlvl_addr = REG_ADDR_I2C(REG_TXLVL, config->channel);	
	ret = mfd_sc16is7xx_read_reg(config->mfd_dev, txlvl_addr, &txlvl);
	if(ret){
		k_spin_unlock(&data->lock, key);
		return 0;
	}

	size_t len_to_write = MIN((size_t)size, (size_t)txlvl);
	uint8_t thr_addr = REG_ADDR_I2C(REG_THR, config->channel);
	ret = mfd_sc16is7xx_write_data(dev, thr_addr, tx_data, len_to_write);
	if(ret){
		k_spin_unlock(&data->lock, key);
		return 0;
	}
	k_spin_unlock(&data->lock, key);
	return len_to_write;
}

static int uart_sc16is7xx_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	//Read data from FIFO
	struct uart_sc16is7xx_data *data = dev->data;
	const struct uart_sc16is7xx_config * const config = dev->config;
	int ret;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	//get number of received bytes in fifo
	//read bytes shall not exceed given buffer size
	uint8_t rxlvl = 0;
	uint8_t rxlvl_addr = REG_ADDR_I2C(REG_RXLVL, config->channel);	
	ret = mfd_sc16is7xx_read_reg(config->mfd_dev, rxlvl_addr, &rxlvl);
	if(ret){
		k_spin_unlock(&data->lock, key);
		return 0;
	}

	size_t len_to_read = MIN((size_t)size, (size_t)rxlvl);
	uint8_t thr_addr = REG_ADDR_I2C(REG_THR, config->channel);
	ret = mfd_sc16is7xx_write_data(dev, thr_addr, rx_data, len_to_read);
	if(ret){
		k_spin_unlock(&data->lock, key);
		return 0;
	}
	k_spin_unlock(&data->lock, key);
	return len_to_read;
}

static void uart_sc16is7xx_irq_tx_enable(const struct device *dev)
{
	//enable TX interrupt in IER
	struct uart_sc16is7xx_data *data = dev->data;
	//const struct uart_sc16is7xx_config * const config = dev->config;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	int ret = set_interrupt(dev, REG_IER_THR_INT_EN, 0);
	if(ret){
		LOG_ERR("set_interrupt() returns %d",ret);
	}

	k_spin_unlock(&data->lock, key);
}

static void uart_sc16is7xx_irq_tx_disable(const struct device *dev)
{
	//Disable TX interrupt in IER
	struct uart_sc16is7xx_data *data = dev->data;
	//const struct uart_sc16is7xx_config * const config = dev->config;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int ret = set_interrupt(dev, 0, REG_IER_THR_INT_EN);
	if(ret){
		LOG_ERR("set_interrupt() returns %d",ret);
	}
	k_spin_unlock(&data->lock, key);
}

static int uart_sc16is7xx_irq_tx_ready(const struct device *dev)
{
	//Check if Tx IRQ has been raised
	struct uart_sc16is7xx_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	int ret = ((data->iir_cache & REG_IIR_BIT_INT_CODE) == REG_IIR_INT_CODE_THR) ? 1 : 0;

	k_spin_unlock(&data->lock, key);

	return ret;
}

static int uart_ns16550_irq_tx_complete(const struct device *dev)
{
	//Check if nothing remains to be transmitted
	//return 1 if nothing remains to be transmitted, 0 otherwise
	struct uart_sc16is7xx_data *data = dev->data;
	const struct uart_sc16is7xx_config * const config = dev->config;
	int ret;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	uint8_t lsr = 0;
	uint8_t lsr_addr = REG_ADDR_I2C(REG_LSR, config->channel);
	ret = mfd_sc16is7xx_read_reg(config->mfd_dev, lsr_addr, &lsr);
	if(ret){
		k_spin_unlock(&data->lock, key);
		return -1;
	}	

	ret = (lsr & (REG_LSR_BIT_THR_TSR_EMPTY | REG_LSR_BIT_THR_EMPTY)) ? 1:0;

	k_spin_unlock(&data->lock, key);

	return ret;
}

static void uart_sc16is7xx_irq_rx_enable(const struct device *dev)
{
	//enable RX interrupt in IER
	struct uart_sc16is7xx_data *data = dev->data;
	//const struct uart_sc16is7xx_config * const config = dev->config;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	int ret = set_interrupt(dev, REG_IER_RHR_INT_EN, 0);
	if(ret){
		LOG_ERR("set_interrupt() returns %d",ret);
	}

	k_spin_unlock(&data->lock, key);
}

static void uart_sc16is7xx_irq_rx_disable(const struct device *dev)
{
	//Disable RX interrupt in IER
	struct uart_sc16is7xx_data *data = dev->data;
	//const struct uart_sc16is7xx_config * const config = dev->config;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int ret = set_interrupt(dev, 0, REG_IER_RHR_INT_EN);
	if(ret){
		LOG_ERR("set_interrupt() returns %d",ret);
	}
	k_spin_unlock(&data->lock, key);
}

static int uart_ns16550_irq_rx_ready(const struct device *dev)
{
	//Check if Rx IRQ has been raised
	struct uart_sc16is7xx_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	//When the UART receives a number of characters and these data are not enough to set off the receive interrupt
	// (because they do not reach the receive trigger level), the UART will generate a time-out interrupt instead, 4
	// character times after the last character is received. The time-out counter will be reset at the center of each stop
	// bit received or each time the receive FIFO is read.

	//int ret = ((data->iir_cache & REG_IIR_BIT_INT_CODE) == REG_IIR_INT_CODE_RHR) ? 1 : 0;
	uint8_t interrupt_code = (data->iir_cache & REG_IIR_BIT_INT_CODE);
	int ret = (interrupt_code == REG_IIR_INT_CODE_RHR)||(interrupt_code == REG_IIR_INT_CODE_RX_TIMEOUT) ? 1 : 0;

	k_spin_unlock(&data->lock, key);
	return ret;
}

static void uart_sc16is7xx_irq_err_enable(const struct device *dev)
{
	//enable ERROR interrupt in IER
	struct uart_sc16is7xx_data *data = dev->data;
	//const struct uart_sc16is7xx_config * const config = dev->config;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	int ret = set_interrupt(dev, REG_IER_RX_LINE_STAT_INT_EN, 0);
	if(ret){
		LOG_ERR("set_interrupt() returns %d",ret);
	}

	k_spin_unlock(&data->lock, key);
}

static void uart_sc16is7xx_irq_err_disable(const struct device *dev)
{
	//Disable ERROR interrupt in IER
	struct uart_sc16is7xx_data *data = dev->data;
	//const struct uart_sc16is7xx_config * const config = dev->config;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int ret = set_interrupt(dev, 0, REG_IER_RX_LINE_STAT_INT_EN);
	if(ret){
		LOG_ERR("set_interrupt() returns %d",ret);
	}
	k_spin_unlock(&data->lock, key);
}

static int uart_sc16is7xx_irq_is_pending(const struct device *dev)
{
	//Check if any IRQ is pending
	//return 1 if an IRQ is pending, 0 otherwise
	struct uart_sc16is7xx_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	//logic 0 = an interrupt is pending
	//logic 1 = no interrupt is pending
	int ret = (!(data->iir_cache & REG_IIR_BIT_INT_STATUS)) ? 1 : 0;

	k_spin_unlock(&data->lock, key);
	return ret;
}

static int uart_sc16is7xx_irq_update(const struct device *dev)
{
	// Update cached contents of IIR
	// return Always 1
	struct uart_sc16is7xx_data *data = dev->data;
	const struct uart_sc16is7xx_config * const config = dev->config;
	int ret;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	uint8_t iir_addr = REG_ADDR_I2C(REG_IIR, config->channel);
	ret = mfd_sc16is7xx_read_reg(config->mfd_dev, iir_addr, &data->iir_cache);
	if (ret){
		LOG_ERR("mfd_sc16is7xx_read_reg() returns %d", ret);
	}

	k_spin_unlock(&data->lock, key);

	return 1;
}

static void uart_sc16is7xx_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb, void *cb_data)
{
	struct uart_sc16is7xx_data * const data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->cb = cb;
	data->cb_data = cb_data;

	k_spin_unlock(&data->lock, key);
}

//each UART device should have this driver SPI
//TODO: zephyr v3.7.1 cannot use DEVICE_API() macro yet
static const struct uart_driver_api uart_sc16is7xx_driver_api = {
	.poll_in = uart_sc16is7xx_poll_in,
	.poll_out = uart_sc16is7xx_poll_out,
	.err_check = uart_sc16is7xx_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_sc16is7xx_configure,
	.config_get = uart_sc16is7xx_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_sc16is7xx_fifo_fill,
	.fifo_read = uart_sc16is7xx_fifo_read,
	.irq_tx_enable = uart_sc16is7xx_irq_tx_enable,
	.irq_tx_disable = uart_sc16is7xx_irq_tx_disable,
	.irq_tx_ready = uart_sc16is7xx_irq_tx_ready,
	.irq_tx_complete = uart_ns16550_irq_tx_complete,
	.irq_rx_enable = uart_sc16is7xx_irq_rx_enable,
	.irq_rx_disable = uart_sc16is7xx_irq_rx_disable,
	.irq_rx_ready = uart_ns16550_irq_rx_ready,
	.irq_err_enable = uart_sc16is7xx_irq_err_enable,
	.irq_err_disable = uart_sc16is7xx_irq_err_disable,
	.irq_is_pending = uart_sc16is7xx_irq_is_pending,
	.irq_update = uart_sc16is7xx_irq_update,
	.irq_callback_set = uart_sc16is7xx_irq_callback_set,
#endif

//this chip does not support ASYNC API.
#ifdef CONFIG_UART_ASYNC_API
	.callback_set = NULL,
	.tx = NULL,
	.tx_abort = NULL,
	.rx_enable = NULL,
	.rx_disable = NULL,
	.rx_buf_rsp = NULL,
#endif

//no need to implment line control yet
#ifdef CONFIG_UART_NS16550_LINE_CTRL
	.line_ctrl_set = NULL,
#endif

//no need to implment drive command yet
#ifdef CONFIG_UART_NS16550_DRV_CMD
	.drv_cmd = NULL,
#endif
};

static void uart_sc16is7xx_isr(const struct device *dev)
{
	struct uart_sc16is7xx_data * const data = dev->data;
	//const struct uart_sc16is7xx_config * const cfg = dev->config;

	//TODO: if this function is called from ISR, the actual work should use workitem and submit to system workq.
	//here we need to read the I2C register to check if the current interrupt applies to this driver
	//and if yes, call the cb function.
	if (data->cb) {
		data->cb(dev, data->cb_data);
	}
	
}

static int uart_sc16is7xx_init(const struct device *dev)
{
	//need to register isr func uart_sc16is7xx_isr()
	const struct uart_sc16is7xx_config *config = dev->config;
	struct uart_sc16is7xx_data *data = dev->data;
	int ret;

	ret = uart_sc16is7xx_configure(dev, &data->uart_config);
	if (ret != 0) {
		return ret;
	}

	//set up the baudrate, parity, stop_bits, etc
	ret = uart_sc16is7xx_configure(dev, &data->uart_config);
	if (ret) {
		return ret;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	sc16is7xx_init_interrupt_callback(&data->callback, dev, uart_sc16is7xx_isr);
	ret = mfd_sc16is7xx_manage_interrupt_callback(config->mfd_dev, &data->callback, true);
	if(ret){
		//LOG_ERR("mfd_sc16is7xx_manage_interrupt_callback() returns %d", ret);
		return ret;
	}
#endif //CONFIG_UART_INTERRUPT_DRIVEN

	return 0;
}

#define UART_SC16IS7XX_DEFINE(inst) \
    static const struct uart_sc16is7xx_config uart_sc16is7xx_config_##inst = {\
        .mfd_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),\
		.channel = DT_REG_ADDR(DT_DRV_INST(inst)),\
    };\
	static struct uart_sc16is7xx_data uart_sc16is7xx_data_##inst = {\
        .uart_config.baudrate = DT_INST_PROP_OR(inst, current_speed, 115200),\
		.uart_config.parity = DT_INST_ENUM_IDX_OR(inst, parity, UART_CFG_PARITY_NONE),\
		.uart_config.stop_bits = DT_INST_PROP_OR(inst, stop_bits, UART_CFG_STOP_BITS_1),\
		.uart_config.data_bits = DT_INST_PROP_OR(inst, data_bits, UART_CFG_DATA_BITS_8),\
		.uart_config.flow_ctrl = DT_INST_PROP_OR(inst, hw_flow_control, UART_CFG_FLOW_CTRL_NONE),\
    };\
	DEVICE_DT_INST_DEFINE(inst,\
		&uart_sc16is7xx_init,\
		NULL,\
		&uart_sc16is7xx_data_##inst,\
		&uart_sc16is7xx_config_##inst,\
		POST_KERNEL,\
		CONFIG_MFD_SC16IS7XX_INIT_PRIORITY,\
		&uart_sc16is7xx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_SC16IS7XX_DEFINE)