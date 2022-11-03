/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "board.h"
#include "api.h"

#include "mcu.h"
#include "i2c.h"

#include "vendor/em_cmu.h"  // For CMU_ClockFreqGet(), cmuClock_HFPER

/** Declare the interrupt handler */
void I2C_IRQHandler(void);

#if defined(USE_I2C1)
#define I2C_IRQn            I2C1_IRQn
#define I2C_DEV             I2C1
#define I2C_MODULE          1
#ifdef _SILICON_LABS_32B_SERIES_2
#define I2C_CLKEN0_BIT      CMU_CLKEN0_I2C1
#elif defined _SILICON_LABS_32B_SERIES_1
#define I2C_HFPERCLKEN0_BIT CMU_HFPERCLKEN0_I2C1
#endif // _SILICON_LABS_32B_SERIES_2
#elif defined(USE_I2C0)
#define I2C_IRQn            I2C0_IRQn
#define I2C_DEV             I2C0
#define I2C_MODULE          0
#ifdef _SILICON_LABS_32B_SERIES_2
#define I2C_CLKEN0_BIT      CMU_CLKEN0_I2C0
#elif defined _SILICON_LABS_32B_SERIES_1
#define I2C_HFPERCLKEN0_BIT CMU_HFPERCLKEN0_I2C0
#endif // _SILICON_LABS_32B_SERIES_2
#else
#error USE_I2C0 or USE_I2C1 must be defined
#endif

/** Internal transfer description */
typedef struct
{
    i2c_xfer_t *                client_xfer;    //< Transfer asked by client
    i2c_on_transfer_done_cb_f   cb;     //< Callback to call at end of transfer
    i2c_res_e                   res;            //< Result of I2C transfer
    uint8_t                     pos;            //< Read or write position
    bool                        free;           //< False if transfer ongoing
    bool                        write_done;     //< Is write before read done
    bool                        done;           //< Is transfer done
    bool                        mstop_detected; //< Is MSTOP interrupt detected
} internal_xfer_desc;

/** I2C state machine state */
typedef enum
{
    I2C_SM_ST_UNINITIALIZED = 0,
    I2C_SM_ST_IDLE,
    I2C_SM_ST_START,
    I2C_SM_ST_WRITE_ADDR,
    I2C_SM_ST_WRITE_DATA,
    I2C_SM_ST_READ,
    I2C_SM_ST_STOP
} i2c_sm_state_e;

/** I2C state machine commands */
typedef enum
{
    I2C_SM_CMD_ABORT = 0,
    I2C_SM_CMD_START,
    I2C_SM_CMD_NEXT
} i2c_sm_command_e;

/**
 * Time taken to transfer one bit on I2C with 50% margin. This value depends
 * on the bus speed.
 * This timeout is necessary because I2C peripheral doesn't generate any errors
 * and hangs when the line is low (disconnected slave, no pull-up one the line).
 * In asynchronous mode, use I2C_status to check if transfer is finished.
 */
static uint32_t m_i2c_bit_timeout_ns;

/** Is I2C module initialized */
static volatile i2c_sm_state_e m_state = I2C_SM_ST_UNINITIALIZED;

/** Current tansfer ongoing. Only one transfer supported at a time */
static volatile internal_xfer_desc m_current_xfer;

/**
 * \brief   Configure I2C gpios
 * \param   pullup
 *          Activate internal EFR32 pullup on SDA and SCL
 */
static void configure_gpios(bool pullup)
{
    //Configure SCL GPIO
    hal_gpio_set(BOARD_I2C_GPIO_PORT, BOARD_I2C_SCL_PIN);
    hal_gpio_set_mode(BOARD_I2C_GPIO_PORT,
                      BOARD_I2C_SCL_PIN,
                      pullup ? GPIO_MODE_OUT_OD_PU :
                               GPIO_MODE_OUT_OD_NOPULL);

    //Configure SDA GPIO
    hal_gpio_set(BOARD_I2C_GPIO_PORT, BOARD_I2C_SDA_PIN);
    hal_gpio_set_mode(BOARD_I2C_GPIO_PORT,
                      BOARD_I2C_SDA_PIN,
                      pullup ? GPIO_MODE_OUT_OD_PU :
                               GPIO_MODE_OUT_OD_NOPULL);

#ifdef _SILICON_LABS_32B_SERIES_2
    // Route GPIO pins to I2C module
    GPIO->I2CROUTE[I2C_MODULE].SDAROUTE = (GPIO->I2CROUTE[I2C_MODULE].SDAROUTE & ~_GPIO_I2C_SDAROUTE_MASK)
                                        | (BOARD_I2C_GPIO_PORT << _GPIO_I2C_SDAROUTE_PORT_SHIFT
                                        | (BOARD_I2C_SDA_PIN << _GPIO_I2C_SDAROUTE_PIN_SHIFT));
    GPIO->I2CROUTE[I2C_MODULE].SCLROUTE = (GPIO->I2CROUTE[I2C_MODULE].SCLROUTE & ~_GPIO_I2C_SCLROUTE_MASK)
                                        | (BOARD_I2C_GPIO_PORT << _GPIO_I2C_SCLROUTE_PORT_SHIFT
                                        | (BOARD_I2C_SCL_PIN << _GPIO_I2C_SCLROUTE_PIN_SHIFT));
    GPIO->I2CROUTE[I2C_MODULE].ROUTEEN = GPIO_I2C_ROUTEEN_SDAPEN | GPIO_I2C_ROUTEEN_SCLPEN;
#else
    // Choose routing of SDA and SCL GPIO to the I2C peripheral
    I2C_DEV->ROUTELOC0 = BOARD_I2C_ROUTELOC_SDALOC | BOARD_I2C_ROUTELOC_SCLLOC;

    // Enable routing of SDA and SCL to the I2C peripheral
    I2C_DEV->ROUTEPEN = I2C_ROUTEPEN_SDAPEN | I2C_ROUTEPEN_SCLPEN;
#endif
}

/**
 * \brief   Release I2C gpios (SDA and SCL)
 */
static void release_gpios(void)
{
    // Disable routing of SDA and SCL to the I2C peripheral
#ifdef _SILICON_LABS_32B_SERIES_2
    GPIO->I2CROUTE[I2C_MODULE].SDAROUTE = _GPIO_I2C_SDAROUTE_RESETVALUE;
    GPIO->I2CROUTE[I2C_MODULE].SCLROUTE = _GPIO_I2C_SCLROUTE_RESETVALUE;
    GPIO->I2CROUTE[I2C_MODULE].ROUTEEN = _GPIO_LETIMER_ROUTEEN_RESETVALUE;
#else
    I2C_DEV->ROUTEPEN = _I2C_ROUTEPEN_RESETVALUE;
    I2C_DEV->ROUTELOC0 = _I2C_ROUTELOC0_RESETVALUE;
#endif

    // Restore SDA GPIO
    hal_gpio_set_mode(BOARD_I2C_GPIO_PORT,
                      BOARD_I2C_SDA_PIN,
                      GPIO_MODE_DISABLED);
    hal_gpio_clear(BOARD_I2C_GPIO_PORT, BOARD_I2C_SDA_PIN);

    // Restore SCL GPIO
    hal_gpio_set_mode(BOARD_I2C_GPIO_PORT,
                      BOARD_I2C_SCL_PIN,
                      GPIO_MODE_DISABLED);
    hal_gpio_clear(BOARD_I2C_GPIO_PORT, BOARD_I2C_SCL_PIN);
}

/**
 * \brief   Enable or disable I2C peripheral clock
 * \param   enable
 *          True to enable clock, false to disable
 */
static void enable_clock(bool enable)
{
    if (enable)
    {
#ifdef _SILICON_LABS_32B_SERIES_2
        // I2C clock source gating available for xg22 only
        // Turn on clock to the I2C peripheral for xg22
        CMU->CLKEN0 |= I2C_CLKEN0_BIT;
#elif defined _SILICON_LABS_32B_SERIES_1
        // Turn on clock to the I2C peripheral for xg12/13
        CMU->HFPERCLKEN0 |= I2C_HFPERCLKEN0_BIT;
#endif
    }
    else
    {
#ifdef _SILICON_LABS_32B_SERIES_2
        // I2C clock source gating available for xg22 only
        // Turn off clock to the I2C peripheral for xg22
        CMU->CLKEN0 &= ~I2C_CLKEN0_BIT;
#elif defined _SILICON_LABS_32B_SERIES_1
        // Turn off clock to the I2C peripheral for xg12/13
        CMU->HFPERCLKEN0 &= ~I2C_HFPERCLKEN0_BIT;
#endif
    }
}

/**
 * \brief   Enable or disable I2C peripheral interrupts
 * \param   enable
 *          True to enable NVIC IRQ for I2C, false to disable
 * \param   clear
 *          True to clear pending I2C peripheral interrupts, false to disable
 * \note    IRQ can only be enabled when clocks are enabled
 */
static void enable_interrupts(bool enable, bool clear)
{
    if (clear)
    {
        // Clear all I2C interrupt flags
#ifdef _SILICON_LABS_32B_SERIES_2
        // For xg21/22
        I2C_DEV->IF_CLR = _I2C_IF_MASK;
#elif defined _SILICON_LABS_32B_SERIES_1
        // for xg12/13
        I2C_DEV->IFC = _I2C_IFC_MASK;
#endif
    }

    if (enable)
    {
        // Enable START, RXDATAV, ACK, NACK and MSTOP interrupts
        I2C_DEV->IEN = I2C_IEN_START | I2C_IEN_RXDATAV |
                       I2C_IEN_ACK | I2C_IEN_NACK | I2C_IEN_MSTOP;
    }
    else
    {
        // Disable all I2C interrupts
        I2C_DEV->IEN = _I2C_IEN_RESETVALUE;
    }
}

/**
 * \brief   Set the frequency of the I2C module
 * \param   freq
 *          Frequency requested in Hz
 * \return  True if successful set, false if not available
 */
static bool set_frequency(uint32_t f_scl)
{
    //Clock source for I2C interface
#ifdef _SILICON_LABS_32B_SERIES_2
#ifdef USE_I2C0
    uint32_t f_i2cclk = CMU_ClockFreqGet(cmuClock_LSPCLK);
#elif defined USE_I2C1
    uint32_t f_i2cclk = CMU_ClockFreqGet(cmuClock_PCLK);
#endif
#elif defined _SILICON_LABS_32B_SERIES_1
    // Read HFPERCLK frequency
    uint32_t f_i2cclk = CMU_ClockFreqGet(cmuClock_HFPER);
#endif

    // Calculate required divisor
    //
    // div = (f_i2cclk / f_scl - 8) / (n_low + n_high) - 1
    //
    // , where
    //      f_i2cclk: I2C clock source frequency, in Hz
    //      f_scl: Desired SCL frequency, in Hz
    //      n_low: low period counter, 4 for standard clock
    //      n_high: high period counter, 4 for standard clock
    if (f_scl == 0)
    {
        // DC SCL not supported
        return false;
    }
    uint32_t div = f_i2cclk / f_scl;
    if (div < (1 * 8 + 8))
    {
        // Requested SCL clock frequency too fast
        return false;
    }
    div = (div - 8) / 8 - 1;
    if (div > (_I2C_CLKDIV_DIV_MASK >> _I2C_CLKDIV_DIV_SHIFT))
    {
        // Requested SCL clock frequency too slow
        return false;
    }

    // Set I2C SCL frequency
    I2C_DEV->CLKDIV = div;

    return true;
}

/**
 * \brief   I2C state machine, also called from interrupts
 * \param   state
 *          New state to enter
 */
static void i2c_state_machine(i2c_sm_command_e command)
{
    bool error = false;
    bool call_cb = false;
    i2c_xfer_t * client_xfer = m_current_xfer.client_xfer;
    uint8_t dev_state;

    // Turn on clock while in state machine
    enable_clock(true);

    dev_state = I2C_DEV->STATE & (_I2C_STATE_STATE_MASK |
                                  _I2C_STATE_NACKED_MASK);

    // Disable interrupts while in state machine
    enable_interrupts(false, false);

    if (command == I2C_SM_CMD_ABORT)
    {
        // Force transfer abort
        error = true;
    }
    else if ((command == I2C_SM_CMD_START) &&
             (m_state == I2C_SM_ST_IDLE) &&
             (dev_state == I2C_STATE_STATE_IDLE))
    {
        // Start transfer, transmit START condition

        // Update current state
        m_state = I2C_SM_ST_START;

        // Clear pending interrupts just in case
        enable_interrupts(false, true);

        // Transmit START condition
        I2C_DEV->CMD = I2C_CMD_START;
    }
    else if ((command == I2C_SM_CMD_NEXT) &&
             (m_state == I2C_SM_ST_START) &&
             (dev_state == I2C_STATE_STATE_START))
    {
        // Transmit address and R/W bit, or STOP condition if nothing to do

        uint8_t addr = client_xfer->address << 1;

        if (!m_current_xfer.write_done && (client_xfer->write_size > 0))
        {
            // Update current state
            m_state = I2C_SM_ST_WRITE_ADDR;

            // Write address and R/W bit = 0 to transmit buffer, to transmit it
            I2C_DEV->TXDATA = addr | 0x00;
        }
        else if (client_xfer->read_size > 0)
        {
            // Update current state
            m_state = I2C_SM_ST_READ;

            // Flush receive buffer and shift register
            while (I2C_DEV->STATUS & I2C_STATUS_RXDATAV)
            {
                I2C_DEV->RXDATA;
            }

            // Write address and R/W bit = 1 to transmit buffer, to transmit it
            I2C_DEV->TXDATA = addr | 0x01;
        }
        else
        {
            // Update current state
            m_state = I2C_SM_ST_STOP;

            // Nothing to transmit or receive, transmit a STOP condition
            I2C_DEV->CMD = I2C_CMD_STOP;
        }
    }
    else if ((command == I2C_SM_CMD_NEXT) && (m_state == I2C_SM_ST_WRITE_ADDR))
    {
        // Check address ACK, transmit first byte of data

        if (dev_state == I2C_STATE_STATE_ADDR)
        {
            // Address transmitted, wait for ACK or NACK
        }
        else if (dev_state == (I2C_STATE_STATE_ADDRACK | I2C_STATE_NACKED))
        {
            // Address not ACKed
            m_current_xfer.res = I2C_RES_ANACK;

            // Update current state
            m_state = I2C_SM_ST_STOP;

            // Transmit STOP condition
            I2C_DEV->CMD = I2C_CMD_STOP;
        }
        else if (dev_state == I2C_STATE_STATE_ADDRACK)
        {
            // Address ACKed, update current state
            m_state = I2C_SM_ST_WRITE_DATA;

            // Write first byte of data to transmit buffer
            I2C_DEV->TXDATA = client_xfer->write_ptr[m_current_xfer.pos++];
        }
        else
        {
            // Invalid state
            error = true;
        }
    }
    else if ((command == I2C_SM_CMD_NEXT) && (m_state == I2C_SM_ST_WRITE_DATA))
    {
        // Check data ACK, transmit next byte of data

        if (dev_state == I2C_STATE_STATE_DATA)
        {
            // Data transmitted, wait for ACK or NACK
        }
        else if (dev_state == (I2C_STATE_STATE_DATAACK | I2C_STATE_NACKED))
        {
            // Data not ACKed
            m_current_xfer.res = I2C_RES_DNACK;

            // Update current state
            m_state = I2C_SM_ST_STOP;

            // Transmit STOP condition
            I2C_DEV->CMD = I2C_CMD_STOP;
        }
        else if (dev_state == I2C_STATE_STATE_DATAACK)
        {
            // Data ACKed
            if (m_current_xfer.pos < client_xfer->write_size)
            {
                // Write next byte of data to transmit buffer
                I2C_DEV->TXDATA = client_xfer->write_ptr[m_current_xfer.pos++];
            }
            else if (client_xfer->read_size > 0)
            {
                // Mark write done and reset read position
                m_current_xfer.write_done = true;
                m_current_xfer.pos = 0;

                // Update current state
                m_state = I2C_SM_ST_START;

                // Some bytes to read, transmit repeated START condition
                I2C_DEV->CMD = I2C_CMD_START;
            }
            else
            {
                // No more bytes to write or read
                m_current_xfer.res = I2C_RES_OK;

                // Update current state
                m_state = I2C_SM_ST_STOP;

                // Transmit STOP condition
                I2C_DEV->CMD = I2C_CMD_STOP;
            }
        }
        else
        {
            // Invalid state
            error = true;
        }
    }
    else if ((command == I2C_SM_CMD_NEXT) && (m_state == I2C_SM_ST_READ))
    {
        // Check address ACK or read data

        if (dev_state == I2C_STATE_STATE_ADDR)
        {
            // Address transmitted, wait for ACK or NACK
        }
        else if (dev_state == (I2C_STATE_STATE_ADDRACK | I2C_STATE_NACKED))
        {
            // Address not ACKed
            m_current_xfer.res = I2C_RES_ANACK;

            // Update current state
            m_state = I2C_SM_ST_STOP;

            // Transmit STOP condition
            I2C_DEV->CMD = I2C_CMD_STOP;
        }
        else if (dev_state == I2C_STATE_STATE_ADDRACK)
        {
            // Address ACKed, perform a dummy read to start reception
            I2C_DEV->RXDATA;
        }
        else if (dev_state == I2C_STATE_STATE_DATA)
        {
            // Data received, transmit ACK or NACK
            I2C_DEV->CMD = ((client_xfer->read_size - m_current_xfer.pos) > 1) ?
                I2C_CMD_ACK : I2C_CMD_NACK;
        }
        else if (dev_state == I2C_STATE_STATE_DATAACK)
        {
            // Read received byte
            client_xfer->read_ptr[m_current_xfer.pos++] = I2C_DEV->RXDATA;

            // Data ACK or NACK sent
            if (m_current_xfer.pos == client_xfer->read_size)
            {
                // Last byte received
                m_current_xfer.res = I2C_RES_OK;

                // Update current state
                m_state = I2C_SM_ST_STOP;

                // Transmit STOP condition
                I2C_DEV->CMD = I2C_CMD_STOP;
            }
        }
        else
        {
            // Invalid state
            error = true;
        }
    }
    else if ((command == I2C_SM_CMD_NEXT) && (m_state == I2C_SM_ST_STOP))
    {
        // STOP has been served. Or is it? What is the opinion of HW?
        // Unfortunately cannot assume that dev_state would be IDLE when
        // MSTOP interrupt is generated. It is still quite often DATAACK.
        // No interrupts will occur after MSTOP, thus i2c bus would hang.
        // To overcome this problem mstop_detected flag is passed from
        // interrupt handler so that state machine can be set to IDLE
        // state and complete transfer successfully.
        if ((dev_state == I2C_STATE_STATE_IDLE) || (m_current_xfer.mstop_detected))
        {
            // I2C is idle again
            m_state = I2C_SM_ST_IDLE;

            // Call callback
            call_cb = true;
        }
        else
        {
            // Invalid state
            error = true;
        }
    }
    else
    {
        // Invalid state, abort transfer
        error = true;
    }

    if (error)
    {
        // I2C is idle again
        m_state = I2C_SM_ST_IDLE;

        if (!m_current_xfer.free)
        {
            // Error occurred, call callback with error code I2C_RES_BUS_HANG
            m_current_xfer.res = I2C_RES_BUS_HANG;
            call_cb = true;
        }

        // Something went wrong, abort transfer
        I2C_DEV->CMD = I2C_CMD_ABORT;
    }

    if (call_cb && !m_current_xfer.free)
    {
        // Free the transfer before calling client callback, to chain requests
        m_current_xfer.free = true;

        // If not blocking transfer, call the client callback
        // NOTE: This may set m_state != I2C_SM_ST_IDLE
        if (m_current_xfer.cb != NULL)
        {
            m_current_xfer.cb(m_current_xfer.res, client_xfer);
        }
        else
        {
            // If blocking call, set the transfer completed flag
            m_current_xfer.done = true;
        }
    }

    if (m_state == I2C_SM_ST_IDLE)
    {
        // I2C is idle, so turn off clock to the I2C peripheral
        enable_clock(false);
    }
    else
    {
        // I2C is running, enable interrupts and keep pending interrupt flags,
        // as the I2C operation may have completed already when we get here
        enable_interrupts(true, false);
    }
}

i2c_res_e I2C_init(i2c_conf_t * conf_p)
{
    if (m_state != I2C_SM_ST_UNINITIALIZED)
    {
        return I2C_RES_ALREADY_INITIALIZED;
    }

    // Mark I2C driver as initialized
    m_state = I2C_SM_ST_IDLE;

    // Turn on clock to the I2C peripheral during configuration
    enable_clock(true);

    // Disable I2C peripheral interrupts, clear pending interrupt flags
    enable_interrupts(false, true);

    // Enable I2C IRQ
    Sys_clearFastAppIrq(I2C_IRQn);
    Sys_enableFastAppIrq(I2C_IRQn,
                         APP_LIB_SYSTEM_IRQ_PRIO_HI,
                         I2C_IRQHandler);

    // Enable I2C peripheral: arbitration disabled, 4:4 clock periods
#ifdef _SILICON_LABS_32B_SERIES_2
    I2C_DEV->CTRL_SET = I2C_CTRL_ARBDIS |
                        I2C_CTRL_CLHR_STANDARD | I2C_CTRL_TXBIL_EMPTY;
    I2C_DEV->EN_SET = I2C_EN_EN;
#else
    I2C_DEV->CTRL = I2C_CTRL_EN | I2C_CTRL_ARBDIS |
                    I2C_CTRL_CLHR_STANDARD | I2C_CTRL_TXBIL_EMPTY;
#endif

    // Configure SCL frequency
    if (!set_frequency(conf_p->clock))
    {
        // Invalid SCL frequency, return peripheral to the reset state
        I2C_close();
        return I2C_RES_INVALID_CONFIG;
    }
    // Issue an ABORT command to make sure the peripheral is in a known state
    I2C_DEV->CMD = I2C_CMD_ABORT;

    // Configure the GPIOs
    configure_gpios(conf_p->pullup);

    // Configuration done, turn off clock to the I2C peripheral
    enable_clock(false);

    /* Add 50% margin to timeout calculation */
    m_i2c_bit_timeout_ns = 1500000000 / conf_p->clock;

    m_current_xfer.free = true;

    return I2C_RES_OK;
}

i2c_res_e I2C_close(void)
{
    if (m_state == I2C_SM_ST_UNINITIALIZED)
    {
        return I2C_RES_NOT_INITIALIZED;
    }

    m_state = I2C_SM_ST_UNINITIALIZED;

    // Turn on clock to the I2C peripheral during configuration
    enable_clock(true);

    // Disable I2C peripheral interrupts, clear pending interrupt flags
    enable_interrupts(false, true);

    // Disable I2C IRQ
    Sys_disableAppIrq(I2C_IRQn);

    // Issue an ABORT command to make sure the peripheral is in a known state
    I2C_DEV->CMD = I2C_CMD_ABORT;

    // Set all gpios as default configuration
    release_gpios();

    // Disable I2C peripheral
#ifdef _SILICON_LABS_32B_SERIES_2
    I2C_DEV->EN_CLR = I2C_EN_EN;
#endif
    I2C_DEV->CTRL = _I2C_CTRL_RESETVALUE;

    // Configuration done, turn off clock to the I2C peripheral
    enable_clock(false);

    return I2C_RES_OK;
}

i2c_res_e I2C_transfer(i2c_xfer_t * xfer_p, i2c_on_transfer_done_cb_f cb)
{
    if (m_state == I2C_SM_ST_UNINITIALIZED)
    {
        return I2C_RES_NOT_INITIALIZED;
    }

    // Check if a transfer is already ongoing
    if (!m_current_xfer.free)
    {
        return I2C_RES_BUSY;
    }

    // Check transfer
    if (((xfer_p->read_ptr != NULL) && (xfer_p->read_size == 0)) ||
        ((xfer_p->read_ptr == NULL) && (xfer_p->read_size != 0)) ||
        ((xfer_p->write_ptr != NULL) && (xfer_p->write_size == 0)) ||
        ((xfer_p->write_ptr == NULL) && (xfer_p->write_size != 0)))
    {
        return I2C_RES_INVALID_XFER;
    }

    // Setup the transfer
    m_current_xfer.client_xfer = xfer_p;
    m_current_xfer.cb = cb;
    m_current_xfer.res = I2C_RES_OK;
    m_current_xfer.pos = 0;
    m_current_xfer.free = false;
    m_current_xfer.write_done = false;
    m_current_xfer.done = false;
    m_current_xfer.mstop_detected = false;

    // Start the transfer
    i2c_state_machine(I2C_SM_CMD_START);

    // Is it a blocking call
    if (m_current_xfer.cb == NULL)
    {
        app_lib_time_timestamp_hp_t end;

        /* Timeout calculation, (timeout is in us)/
         * - Start Address R/W bit is 10bits
         * - For each byte read or written add 1 ack bit so 9bits
         */
        uint32_t timeout = ((10 + xfer_p->read_size*9 + xfer_p->write_size*9) *
                            m_i2c_bit_timeout_ns) / 1000;

        end = lib_time->addUsToHpTimestamp(lib_time->getTimestampHp(),
                                           timeout);

        // Active wait until end of transfer or timeout
        while (!m_current_xfer.done &&
               lib_time->isHpTimestampBefore(lib_time->getTimestampHp(),end) );

        m_current_xfer.free = true;

        if(!m_current_xfer.done)
        {
            return I2C_RES_BUS_HANG;
        }
        else
        {
            return m_current_xfer.res;
        }
    }

    return I2C_RES_OK;
}

i2c_res_e I2C_status(void)
{
    if (m_state == I2C_SM_ST_UNINITIALIZED)
    {
        return I2C_RES_NOT_INITIALIZED;
    }
    else if (!m_current_xfer.free)
    {
        return I2C_RES_BUSY;
    }
    else
    {
        return I2C_RES_OK;
    }
}

/**
 * \brief   Function to handle the I2C Interrupt.
 */
void __attribute__((__interrupt__)) I2C_IRQHandler(void)
{
    if (I2C_DEV->IF && _I2C_IF_MSTOP_MASK)
    {
        m_current_xfer.mstop_detected = true;
    } else {
        m_current_xfer.mstop_detected = false;
    }

    // Clear all I2C interrupt flags
#ifdef _SILICON_LABS_32B_SERIES_2
        // For xg21/22
        I2C_DEV->IF_CLR = _I2C_IF_MASK;
#else
        // for xg12/13
        I2C_DEV->IFC = _I2C_IFC_MASK;
#endif

    // Run the I2C state machine
    i2c_state_machine(I2C_SM_CMD_NEXT);
}
