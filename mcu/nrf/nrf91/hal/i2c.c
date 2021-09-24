/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
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

/** Declare the interrupt handler */
void __attribute__((__interrupt__))     I2C_IRQHandler(void);

#if defined(USE_I2C0)
#define I2C_IRQn    UARTE0_SPIM0_SPIS0_TWIM0_TWIS0_IRQn
#define I2C_DEV     NRF_TWIM0
#elif defined(USE_I2C1)
#define I2C_IRQn    UARTE1_SPIM1_SPIS1_TWIM1_TWIS1_IRQn
#define I2C_DEV     NRF_TWIM1
#elif defined(USE_I2C2)
#define I2C_IRQn    UARTE2_SPIM2_SPIS2_TWIM2_TWIS2_IRQn
#define I2C_DEV     NRF_TWIM2
#elif defined(USE_I2C3)
#define I2C_IRQn    UARTE3_SPIM3_SPIS3_TWIM3_TWIS3_IRQn
#define I2C_DEV     NRF_TWIM3
#else
#error USE_I2Cx (x=0-3) must be defined
#endif

#define HP_TIME_OPPERATION_MARGIN_US 32
static uint32_t m_i2c_bit_ns;

/** Is I2C module initialized */
static bool m_initialized = false;

/** Internal transfer description */
typedef struct
{
    i2c_xfer_t *          client_xfer;  //< Transfer asked by client
    i2c_on_transfer_done_cb_f   cb;     //< Callback to call at end of transfer
    bool                  free;         //< False if transfer ongoing
    volatile bool         done;         //< Is transfer done
} internal_xfer_desc;

/** Current tansfer ongoing. Only one transfer supported at a time */
static internal_xfer_desc m_current_xfer;

static i2c_res_e m_xfer_res = I2C_RES_OK;

/**
 * \brief   Configure I2C gpios
 * \param   pullup
 *          Activate internal NRF52 pullup on SDA and SCL
 */
static void configure_gpios(bool pullup)
{
    //Configure SDA
    NRF_GPIO->PIN_CNF[BOARD_I2C_SDA_PIN] =
        (GPIO_PIN_CNF_DIR_Input        << GPIO_PIN_CNF_DIR_Pos)
      | (GPIO_PIN_CNF_INPUT_Connect    << GPIO_PIN_CNF_INPUT_Pos)
      | (GPIO_PIN_CNF_PULL_Disabled    << GPIO_PIN_CNF_PULL_Pos)
      | (GPIO_PIN_CNF_DRIVE_S0D1       << GPIO_PIN_CNF_DRIVE_Pos)
      | (GPIO_PIN_CNF_SENSE_Disabled   << GPIO_PIN_CNF_SENSE_Pos);

    //Configure SCL
    NRF_GPIO->PIN_CNF[BOARD_I2C_SCL_PIN] =
        (GPIO_PIN_CNF_DIR_Input      << GPIO_PIN_CNF_DIR_Pos)
      | (GPIO_PIN_CNF_INPUT_Connect  << GPIO_PIN_CNF_INPUT_Pos)
      | (GPIO_PIN_CNF_PULL_Disabled  << GPIO_PIN_CNF_PULL_Pos)
      | (GPIO_PIN_CNF_DRIVE_S0D1     << GPIO_PIN_CNF_DRIVE_Pos)
      | (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);

    if(pullup)
    {
        NRF_GPIO->PIN_CNF[BOARD_I2C_SDA_PIN] |=
            GPIO_PIN_CNF_PULL_Pullup  << GPIO_PIN_CNF_PULL_Pos;

        NRF_GPIO->PIN_CNF[BOARD_I2C_SCL_PIN] |=
            GPIO_PIN_CNF_PULL_Pullup  << GPIO_PIN_CNF_PULL_Pos;
    }
}

/**
 * \brief   Release I2C gpios (SDA and SCL)
 */
static void release_gpios(void)
{
    nrf_gpio_cfg_default(BOARD_I2C_SCL_PIN);
    nrf_gpio_cfg_default(BOARD_I2C_SDA_PIN);
}

/**
 * \brief   Set the frequency of the I2C module
 * \param   freq
 *          Frequency requested in Hz
 * \return  True if successful set, false if not available
 */
static bool set_frequency(uint32_t freq)
{
    uint32_t reg;
    switch (freq)
    {
        case 100000:
            reg = TWIM_FREQUENCY_FREQUENCY_K100;
            break;
        case 250000:
            reg = TWIM_FREQUENCY_FREQUENCY_K250;
            break;
        case 400000:
            reg = TWIM_FREQUENCY_FREQUENCY_K400;
            break;
        default:
            return false;
    }
    I2C_DEV->FREQUENCY = reg;
    return true;
}

/**
 * \brief   Clear all the I2C events
 */
static void clear_i2c_events()
{
    volatile uint32_t dummy;

    // Each event is read back to ensure it is written (needed on cortex m4)
    I2C_DEV->EVENTS_STOPPED = 0x0;
    dummy = I2C_DEV->EVENTS_STOPPED;


    I2C_DEV->EVENTS_ERROR = 0x0;
    dummy = I2C_DEV->EVENTS_ERROR;

    // Other events are not used.
    // I2C_DEV->EVENTS_RXSTARTED
    // I2C_DEV->EVENTS_TXSTARTED
    // I2C_DEV->EVENTS_LASTRX
    // I2C_DEV->EVENTS_LASTRX

    (void) dummy;
}

/**
 * \brief   Disable all I2C interrupts
 */
static void disable_all_interrupts()
{
    I2C_DEV->INTENCLR = TWIM_INTENCLR_STOPPED_Msk |
                        TWIM_INTENCLR_ERROR_Msk |
                        TWIM_INTENCLR_RXSTARTED_Msk |
                        TWIM_INTENCLR_TXSTARTED_Msk |
                        TWIM_INTENCLR_LASTRX_Msk |
                        TWIM_INTENCLR_LASTTX_Msk;
}

/**
 * \brief   Reset DMA Rx and Tx registers to their default values
 */
static void reset_xfer_registers()
{
    I2C_DEV->TXD.PTR = 0;
    I2C_DEV->TXD.MAXCNT = 0;
    I2C_DEV->RXD.PTR = 0;
    I2C_DEV->RXD.MAXCNT = 0;
}

i2c_res_e I2C_init(i2c_conf_t * conf_p)
{
    if (m_initialized)
    {
        return I2C_RES_ALREADY_INITIALIZED;
    }

    // Configure the gpios
    configure_gpios(conf_p->pullup);

    // Make sure periphal is disabled before configuring
    I2C_DEV->ENABLE = (TWIM_ENABLE_ENABLE_Disabled << TWIM_ENABLE_ENABLE_Pos);

    // Configure the I2CM module
    I2C_DEV->PSEL.SCL  = BOARD_I2C_SCL_PIN;
    I2C_DEV->PSEL.SDA = BOARD_I2C_SDA_PIN;

    // Configure frequency
    if (!set_frequency(conf_p->clock))
    {
        return I2C_RES_INVALID_CONFIG;
    }

    // 50% margin added to cover potential I2C clock frequency deltas
    m_i2c_bit_ns = 1500000000 / conf_p->clock;

    disable_all_interrupts();

    // Enable I2C IRQ
    Sys_clearFastAppIrq(I2C_IRQn);
    Sys_enableFastAppIrq(I2C_IRQn,
                         APP_LIB_SYSTEM_IRQ_PRIO_HI,
                         I2C_IRQHandler);

    m_current_xfer.free = true;
    m_initialized = true;

    return I2C_RES_OK;
}

i2c_res_e I2C_close()
{
    if (!m_initialized)
    {
        return I2C_RES_NOT_INITIALIZED;
    }

    m_initialized = false;

    // Disable I2CM module
    I2C_DEV->ENABLE = (TWIM_ENABLE_ENABLE_Disabled << TWIM_ENABLE_ENABLE_Pos);

    Sys_disableAppIrq(I2C_IRQn);

    disable_all_interrupts();

    // Set all gpios as default configuration
    release_gpios();

    return I2C_RES_OK;
}

i2c_res_e I2C_transfer(i2c_xfer_t * xfer_p, i2c_on_transfer_done_cb_f cb)
{
    if (!m_initialized)
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
    m_current_xfer.free = false;
    m_current_xfer.client_xfer = xfer_p;
    m_current_xfer.done = false;
    m_current_xfer.cb = cb;

    // Enable I2CM module
    I2C_DEV->ENABLE = (TWIM_ENABLE_ENABLE_Enabled << TWIM_ENABLE_ENABLE_Pos);

    // Prepare the interrupt part
    disable_all_interrupts();
    clear_i2c_events();

    reset_xfer_registers();

    if (m_current_xfer.client_xfer->write_ptr != NULL)
    {
        I2C_DEV->TXD.PTR = (uint32_t) m_current_xfer.client_xfer->write_ptr;
        I2C_DEV->TXD.MAXCNT = m_current_xfer.client_xfer->write_size;
    }

    if (m_current_xfer.client_xfer->read_ptr != NULL)
    {
        I2C_DEV->RXD.PTR = (uint32_t) m_current_xfer.client_xfer->read_ptr;
        I2C_DEV->RXD.MAXCNT = m_current_xfer.client_xfer->read_size;
    }

    // Set address
    I2C_DEV->ADDRESS = xfer_p->address;

    if (m_current_xfer.client_xfer->write_ptr != NULL)
    {
        if (m_current_xfer.client_xfer->read_ptr != NULL)
        {
            // Tx and Rx : short LASTTX_STARTRX and LASTRX_STOP
            I2C_DEV->SHORTS =
            TWIM_SHORTS_LASTTX_STARTRX_Msk | TWIM_SHORTS_LASTRX_STOP_Msk;
        }
        else
        {
            // Tx Only : short LASTTX_STOP
            I2C_DEV->SHORTS = TWIM_SHORTS_LASTTX_STOP_Msk;
        }
    }
    else
    {
        // Rx Only : short LASTRX_STOP
        I2C_DEV->SHORTS = TWIM_SHORTS_LASTRX_STOP_Msk;
    }

    // Enable interrupts we are interested in (STOPPED and ERROR)
    I2C_DEV->INTENSET = TWIM_INTEN_ERROR_Msk | TWIM_INTEN_STOPPED_Msk;

    // Start the transfer
    if (m_current_xfer.client_xfer->write_ptr != NULL)
    {
        I2C_DEV->TASKS_STARTTX = 0x1UL;
    }
    else
    {
        I2C_DEV->TASKS_STARTRX = 0x1UL;
    }

    // Is it a blocking call
    if (m_current_xfer.cb == NULL)
    {
        app_lib_time_timestamp_hp_t end;

        /* Timeout calculation, (timeout is in us)/
         * - Start + Address + R/W bit + stop is 11 bits
         * - For each written byte there are 8 bits of data + 1 ack
         * - For each read opperation there are 10 bits for restart + address + R/W
         *   and for every byte there are 8 bits + 1 ack  (note that
         *   imediate read is covered)
         */

        uint16_t cycles = 11 + xfer_p->write_size*9;

        if (xfer_p->read_size > 0)
        {
            cycles += xfer_p->read_size*9 + 10;
        }

        // Note that HP_TIME_OPPERATION_MARGIN_US is added to mitigate the 30uS resolution of the hp timer
        uint32_t timeout_us = (cycles * m_i2c_bit_ns)/1000 + HP_TIME_OPPERATION_MARGIN_US;

        end = lib_time->addUsToHpTimestamp(lib_time->getTimestampHp(),
                                           timeout_us);

        // Active wait until end of transfer or timeout
        while (!m_current_xfer.done &&
               lib_time->isHpTimestampBefore(lib_time->getTimestampHp(),end) );

        m_current_xfer.free = true;

        // Disable I2CM module
        I2C_DEV->ENABLE =
          (TWIM_ENABLE_ENABLE_Disabled << TWIM_ENABLE_ENABLE_Pos);

        if(!m_current_xfer.done)
        {
            return I2C_RES_BUS_HANG;
        }
        else
        {
            return m_xfer_res;
        }

    }
    else
    {
        return I2C_RES_OK;
    }
}

i2c_res_e I2C_status()
{
    if (!m_initialized)
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
    if (I2C_DEV->EVENTS_ERROR != 0 || //Error
        I2C_DEV->EVENTS_STOPPED != 0) // or transfer finished
    {
        uint32_t error_source;
        volatile uint32_t dummy;

        // Requested transfer is done
        m_current_xfer.done = true;

        clear_i2c_events();

        // Update transfer result with ERROSRC register
        error_source = I2C_DEV->ERRORSRC;

        // Clear register
        I2C_DEV->ERRORSRC = 0x07;
        dummy = I2C_DEV->ERRORSRC;
        (void) dummy;

        if(error_source & TWIM_ERRORSRC_ANACK_Msk)
        {
          m_xfer_res = I2C_RES_ANACK;
        }
        else if(error_source & TWIM_ERRORSRC_DNACK_Msk)
        {
          m_xfer_res = I2C_RES_DNACK;
        }
        else
        {
          m_xfer_res = I2C_RES_OK;
        }

        // Disable I2CM module
        I2C_DEV->ENABLE =
          (TWIM_ENABLE_ENABLE_Disabled << TWIM_ENABLE_ENABLE_Pos);

        // If not blocking transfer, then call client cb
        if (m_current_xfer.cb != NULL)
        {
          // Free the transfer before calling cb to chain requests
          m_current_xfer.free = true;

          m_current_xfer.cb(m_xfer_res,  m_current_xfer.client_xfer);
        }
    }
}
