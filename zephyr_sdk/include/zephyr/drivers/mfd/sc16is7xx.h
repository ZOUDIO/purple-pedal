#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_SC16IS7XX_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_SC16IS7XX_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>

/** General registers */
#define REG_RHR (0x00u)
#define REG_THR (0x00u)
#define REG_IER (0x01u)
#define REG_FCR (0x02u)
#define REG_IIR (0x02u)
#define REG_LCR (0x03u)
#define REG_MCR (0x04u)
#define REG_LSR (0x05u)
#define REG_MSR (0x06u)
#define REG_SPR (0x07u)
#define REG_TCR (0x06u)
#define REG_TLR (0x07u)
#define REG_TXLVL (0x08u)
#define REG_RXLVL (0x09u)
#define REG_IODIR (0x0Au)
#define REG_IOSTATE (0x0Bu)
#define REG_IOINTENA (0x0Cu)
#define REG_IOCONTROL (0x0Eu)
#define REG_EFCR (0x0Fu)

//Interrupt Enable Register details
#define REG_IER_CTS_INT_EN (BIT(7))
#define REG_IER_RTS_INT_EN (BIT(6))
#define REG_IER_XOFF_INT_EN (BIT(5))
#define REG_IER_SLEEP_MODE_EN (BIT(4))
#define REG_IER_MODEM_STAT_INT_EN (BIT(3))
#define REG_IER_RX_LINE_STAT_INT_EN (BIT(2))
#define REG_IER_THR_INT_EN (BIT(1))
#define REG_IER_RHR_INT_EN (BIT(0))

//FIFO control register details
#define REG_FCR_BIT_RX_TRIGGER (BIT(7)|BIT(6))
#define REG_FCR_BIT_TX_TRIGGER (BIT(5)|BIT(4))
#define REG_FCR_BIT_TX_FIFO_RESET (BIT(2))
#define REG_FCR_BIT_RX_FIFO_RESET (BIT(1))
#define REG_FCR_BIT_TX_FIFO_EN (BIT(0))

#define REG_FCR_RX_TRIGGER_8 (0x0u<<6u)
#define REG_FCR_RX_TRIGGER_16 (0x1u<<6u)
#define REG_FCR_RX_TRIGGER_32 (0x2u<<6u)
#define REG_FCR_RX_TRIGGER_56 (0x3u<<6u)

#define REG_FCR_TX_TRIGGER_8 (0x0u<<4u)
#define REG_FCR_TX_TRIGGER_16 (0x1u<<4u)
#define REG_FCR_TX_TRIGGER_32 (0x2u<<4u)
#define REG_FCR_TX_TRIGGER_56 (0x3u<<4u)

#define REG_FCR_TX_FIFO_RESET (0x1u<<2u)
#define REG_FCR_RX_FIFO_RESET (0x1u<<1u)
#define REG_FCR_FIFO_EN (0x1u)

//Interrupt Identification Register details
#define REG_IIR_BIT_INT_CODE (BIT(5)|BIT(4)|BIT(3)|BIT(2)|BIT(1)|BIT(0)) //note, table 21 includes BIT(0) so we also include it
#define REG_IIR_BIT_INT_STATUS (BIT(0))
#define REG_IIR_INT_CODE_RX_LINE_STAT_ERR (0x06)
#define REG_IIR_INT_CODE_RX_TIMEOUT (0x0C)
#define REG_IIR_INT_CODE_RHR (0x04)
#define REG_IIR_INT_CODE_THR (0x02)
#define REG_IIR_INT_CODE_MODEM (0x00)
#define REG_IIR_INT_CODE_INPUT_PIN (0x30)
#define REG_IIR_INT_CODE_XOFF (0x10)
#define REG_IIR_INT_CODE_CTS_RTS (0x20)

//Line Control Register details
#define REG_LCR_BIT_DIV_LCH_EN (BIT(7))
#define REG_LCR_BIT_PARITY ((BIT(5))|(BIT(4))|(BIT(3)))
#define REG_LCR_BIT_STOP_BITS (BIT(2))
#define REG_LCR_BIT_DATA_BITS ((BIT(1))|(BIT(0)))

#define REG_LCR_PARITY_NONE (0x0u<<3u)
#define REG_LCR_PARITY_ODD (0x1u<<3u)
#define REG_LCR_PARITY_EVEN (0x3u<<3u)
#define REG_LCR_PARITY_MARK (0x5u<<3u)
#define REG_LCR_PARITY_SPACE (0x7u<<3u)

#define REG_LCR_STOP_BITS_1 (0x0u<<2u)
#define REG_LCR_STOP_BITS_1_5 (0x1u<<2u)
#define REG_LCR_STOP_BITS_2 (0x1u<<2u)

#define REG_LCR_DATA_BITS_5 (0x00u)
#define REG_LCR_DATA_BITS_6 (0x01u)
#define REG_LCR_DATA_BITS_7 (0x02u)
#define REG_LCR_DATA_BITS_8 (0x03u)

//Modem Control Register details
#define REG_MCR_BIT_CLK_DIV (BIT(7))

//Line status register details
#define REG_LSR_BIT_FIFO_DATA_ERR (0x1u<<7u)
#define REG_LSR_BIT_THR_TSR_EMPTY (0x1u<<6u)
#define REG_LSR_BIT_THR_EMPTY (0x1u<<5u)
#define REG_LSR_BIT_BREAKK_INT (0x1u<<4u)
#define REG_LSR_BIT_FRAME_ERR (0x1u<<3u)
#define REG_LSR_BIT_PARITY_ERR (0x1u<<2u)
#define REG_LSR_BIT_OVERRUN_ERR (0x1u<<1u)
#define REG_LSR_BIT_DATA_IN_RCV (0x1u<<0u)

// #define COMBINE_PARITY_STOP_BITS_DATA_BITS(p,s,d) (((p<<3)&REG_LCR_BIT_PARITY)|((s<<2)&REG_LCR_BIT_STOP_BITS)|(d&REG_LCR_BIT_DATA_BITS))

/** special registers */
#define REG_DLL (0x00u)
#define REG_DLH (0x01u)

/** enhanced registers */
#define REG_EFR (0x02u)
#define REG_XON1 (0x04u)
#define REG_XON2 (0x05u)
#define REG_XOFF1 (0x06u)
#define REG_XOFF2 (0x07u)

#define REG_EFR_BIT_AUTO_CTS (BIT(7))
#define REG_EFR_BIT_AUTO_RTS (BIT(6))

#define REG_ADDR_I2C(reg, ch) ((reg & BIT_MASK(4))<<3 | (ch & BIT_MASK(2)<<1))
//rw=0 means write, rw=1 means read
#define REG_ADDR_SPI(i2c_addr, rw) WRITE_BIT(i2c_addr, 7, rw)

/**
 * @typedef sc16is7xx_interrupt_handler_t
 * @brief Define the application callback handler function signature
 *
 * @param 
 *
 * Note: cb pointer can be used to retrieve private data through
 * CONTAINER_OF() if original struct gpio_callback is stored in
 * another private structure.
 */
typedef void (*sc16is7xx_interrupt_handler_t)(const struct device *dev);

/**
 * @brief sc16is7xx interrupt callback structure
 *
 * Used to register a callback in the driver instance callback list.
 * As many callbacks as needed can be added as long as each of them
 * are unique pointers of struct sc16is7xx_interrupt_callback.
 * Beware such structure should not be allocated on stack.
 *
 * Note: To help setting it, see gpio_init_callback() below
 */
struct sc16is7xx_interrupt_callback {
	/** This is meant to be used in the driver and the user should not
	 * mess with it (see drivers/gpio/gpio_utils.h)
	 */
	sys_snode_t node;
	/** the device that registers the callback */
	const struct device *dev;
	/** Actual callback function being called when relevant. */
	sc16is7xx_interrupt_handler_t handler;
};

static inline void sc16is7xx_init_interrupt_callback(struct sc16is7xx_interrupt_callback *callback, const struct device *dev,
    sc16is7xx_interrupt_handler_t handler)
{
    __ASSERT(callback, "Callback pointer should not be NULL");
    __ASSERT(handler, "Callback handler pointer should not be NULL");

    callback->handler = handler;
	callback->dev = dev;
    //callback->iir_mask = iir_mask;
}

/**
 * @brief sc16is7xx interrupt manage function
 * add or remove callback
 * 
 * @param dev sc16is7xx device
 * @param callback the callback structure, initialized by sc16is7xx_init_interrupt_callback
 * @param set true to add the callback, false to remove the callback
 * Note: To help setting it, see gpio_init_callback() below
 */
int mfd_sc16is7xx_manage_interrupt_callback(const struct device *dev, struct sc16is7xx_interrupt_callback *callback, bool set);

/**
 * @brief Get the clock frequency of sc16is7xx
 *
 * @param[in] dev Pointer to MFD device
 *
 * @retval clock frequency in Hz
 */
uint32_t mfd_sc16is7xx_get_clock_freq(const struct device *dev);

/**
 * @brief Read data from provided register
 *
 * @param[in] dev Pointer to MFD device
 * @param[in] reg Register to be read
 * @param[in] val Pointer to data buffer
 *
 * @retval 0 if success
 * @retval negative errno if failure
 */
int mfd_sc16is7xx_read_reg(const struct device *dev, const uint8_t reg_addr, uint8_t *val);

/**
 * @brief Write data to provided register
 *
 * @param[in] dev Pointer to MFD device
 * @param[in] reg Register to be written
 * @param[in] val Data to be written
 *
 * @retval 0 if success
 * @retval negative errno if failure
 */
int mfd_sc16is7xx_write_reg(const struct device *dev, const uint8_t reg_addr, const uint8_t val);

/**
 * @brief Read multi-byte data from provided register
 *
 * @param[in] dev Pointer to MFD device
 * @param[in] reg Register to be written
 * @param[in] val Data to be written
 * * @param[in] len length
 *
 * @retval 0 if success
 * @retval negative errno if failure
 */
int mfd_sc16is7xx_read_data(const struct device *dev, const uint8_t reg_addr, uint8_t *val, const size_t len);

/**
 * @brief Write multi-byte data to provided register
 *
 * @param[in] dev Pointer to MFD device
 * @param[in] reg Register to be written
 * @param[in] val Data to be written
 * * @param[in] len length
 *
 * @retval 0 if success
 * @retval negative errno if failure
 */
int mfd_sc16is7xx_write_data(const struct device *dev, const uint8_t reg_addr, const uint8_t *val, const size_t len);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_SC16IS7XX_H_ */