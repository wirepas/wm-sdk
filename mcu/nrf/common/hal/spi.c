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
#include "spi.h"

#include "mcu.h"

/** Declare the interrupt handler */
void __attribute__((__interrupt__))     SPI_IRQHandler(void);


#if defined(USE_SPI0)
#define SPI_IRQn    SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQn
#define SPI_DEV     NRF_SPIM0
#elif defined(USE_SPI1)
#define SPI_IRQn    SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1_IRQn
#define SPI_DEV     NRF_SPIM1
#elif defined(USE_SPI2)
#define SPI_IRQn    SPIM2_SPIS2_SPI2_IRQn
#define SPI_DEV     NRF_SPIM2
#elif defined(USE_SPI3)
#if MCU_SUB == 832
#error SPI3 is not available on nrf52832
#endif
#define SPI_IRQn    SPIM3_IRQn
#define SPI_DEV     NRF_SPIM3
#else
#error You must specify either USE_SPI0, USE_SPI1, USE_SPI2 or USE_SPI3 (nrf52833 and nrf52840 only) in your board.h
#endif

#if MCU_SUB == 832
#define MAX_XFER_SIZE   UINT8_MAX
#else
#define MAX_XFER_SIZE   UINT16_MAX
#endif

/** Is SPI module initialized */
static bool m_initialized = false;

/** Internal transfer description */
typedef struct
{
    spi_xfer_t *          client_xfer;  //< Transfer asked by client
    spi_on_transfer_done_cb_f   cb;     //< Callback to call at end of transfer
    bool                  free;         //< False if transfer ongoing
    volatile bool         done;         //< Is transfer done
} internal_xfer_desc;

/** Current tansfer ongoing. Only one transfer supported in this implementation */
static internal_xfer_desc m_current_xfer;

/**
 * \brief   Configure different SPI gpios (SCK, MOSI and MISO)
 * \param   mode
 *          SPI mode chosen
 */
static void configure_gpios(spi_mode_e mode)
{
    // Configure clock pin (depending on mode)
    if (mode == SPI_MODE_LOW_FIRST || mode == SPI_MODE_LOW_SECOND)
    {
        nrf_gpio_pin_set(BOARD_SPI_SCK_PIN);
    }
    else
    {
        nrf_gpio_pin_clear(BOARD_SPI_SCK_PIN);
    }

    NRF_GPIO->PIN_CNF[BOARD_SPI_SCK_PIN] =
        (GPIO_PIN_CNF_DIR_Output        << GPIO_PIN_CNF_DIR_Pos)
      | (GPIO_PIN_CNF_INPUT_Connect     << GPIO_PIN_CNF_INPUT_Pos)
      | (GPIO_PIN_CNF_PULL_Disabled     << GPIO_PIN_CNF_PULL_Pos)
      | (GPIO_PIN_CNF_DRIVE_S0S1        << GPIO_PIN_CNF_DRIVE_Pos)
      | (GPIO_PIN_CNF_SENSE_Disabled    << GPIO_PIN_CNF_SENSE_Pos);


    // Configure MOSI
    nrf_gpio_pin_clear(BOARD_SPI_MOSI_PIN);
    nrf_gpio_cfg_output(BOARD_SPI_MOSI_PIN);

    // Configure MISO
    nrf_gpio_cfg_input(BOARD_SPI_MISO_PIN,
                       NRF_GPIO_PIN_NOPULL);
}

/**
 * \brief   Release different SPI gpios (SCK, MOSI and MISO)
 */
static void release_gpios()
{
    nrf_gpio_cfg_default(BOARD_SPI_SCK_PIN);
    nrf_gpio_cfg_default(BOARD_SPI_MOSI_PIN);
    nrf_gpio_cfg_default(BOARD_SPI_MISO_PIN);
}

/**
 * \brief   Set the frequency of the SPI module
 * \param   freq
 *          Frequency requested in Hz
 * \return  True if successful set, false if not available
 */
static bool set_frequency(uint32_t freq)
{
    uint32_t reg;
    switch (freq)
    {
        case 125000:
            reg = SPIM_FREQUENCY_FREQUENCY_K125;
            break;
        case 250000:
            reg = SPIM_FREQUENCY_FREQUENCY_K250;
            break;
        case 500000:
            reg = SPIM_FREQUENCY_FREQUENCY_K500;
            break;
        case 1000000:
            reg = SPIM_FREQUENCY_FREQUENCY_M1;
            break;
        case 2000000:
            reg = SPIM_FREQUENCY_FREQUENCY_M2;
            break;
        case 4000000:
            reg = SPIM_FREQUENCY_FREQUENCY_M4;
            break;
        case 8000000:
            reg = SPIM_FREQUENCY_FREQUENCY_M8;
            break;
        default:
            return false;
    }
    SPI_DEV->FREQUENCY = reg;
    return true;
}

/**
 * \brief   Set the SPI Mode of operation
 * \param   mode
 *          SPI mode of operation
 * \param   bit_order
 *          Bit order of SPI transfers
 * \return  True if successful, false otherwise
 */
static bool set_mode(spi_mode_e mode, spi_bit_order_e bit_order)
{
    uint32_t config = 0;
    if (bit_order == SPI_ORDER_LSB)
    {
        config |= (SPIM_CONFIG_ORDER_LsbFirst << SPIM_CONFIG_ORDER_Pos);
    }
    else
    {
        config |= (SPIM_CONFIG_ORDER_MsbFirst << SPIM_CONFIG_ORDER_Pos);
    }

    switch(mode)
    {
        case SPI_MODE_LOW_FIRST:
            config |= (SPIM_CONFIG_CPOL_ActiveLow  << SPIM_CONFIG_CPOL_Pos) |
                      (SPIM_CONFIG_CPHA_Leading    << SPIM_CONFIG_CPHA_Pos);
            break;
        case SPI_MODE_LOW_SECOND:
            config |= (SPIM_CONFIG_CPOL_ActiveLow  << SPIM_CONFIG_CPOL_Pos) |
                      (SPIM_CONFIG_CPHA_Trailing   << SPIM_CONFIG_CPHA_Pos);
            break;
        case SPI_MODE_HIGH_FIRST:
            config |= (SPIM_CONFIG_CPOL_ActiveHigh << SPIM_CONFIG_CPOL_Pos) |
                      (SPIM_CONFIG_CPHA_Leading    << SPIM_CONFIG_CPHA_Pos);
            break;
        case SPI_MODE_HIGH_SECOND:
            config |= (SPIM_CONFIG_CPOL_ActiveHigh << SPIM_CONFIG_CPOL_Pos) |
                      (SPIM_CONFIG_CPHA_Trailing   << SPIM_CONFIG_CPHA_Pos);
            break;
    }
    SPI_DEV->CONFIG = config;
    return true;
}

/**
 * \brief   Clear the SPI end event
 */
static void clear_spi_end_event()
{
    SPI_DEV->EVENTS_END = 0x0;
    // Read it back to ensure it is written (needed on cortex m4)
    volatile uint32_t dummy = SPI_DEV->EVENTS_END;
    (void) dummy;

}

/**
 * \brief   Disable all SPI interrupts
 */
static void disable_all_interrupts()
{
    SPI_DEV->INTENCLR = SPIM_INTENCLR_STARTED_Msk |
                        SPIM_INTENCLR_ENDTX_Msk |
                        SPIM_INTENCLR_END_Msk |
                        SPIM_INTENCLR_ENDRX_Msk |
                        SPIM_INTENCLR_STOPPED_Msk;
}

/**
 * \brief   Reset DMA Rx and Tx registers to their default values
 */
static void reset_xfer_registers()
{
    SPI_DEV->TXD.PTR = 0;
    SPI_DEV->TXD.MAXCNT = 0;
    SPI_DEV->RXD.PTR = 0;
    SPI_DEV->RXD.MAXCNT = 0;
}

spi_res_e SPI_init(spi_conf_t * conf_p)
{
    if (m_initialized)
    {
        return SPI_RES_ALREADY_INITIALIZED;
    }

    // Configure the gpios
    configure_gpios(conf_p->mode);

    // Configure the SPIM module
    SPI_DEV->PSEL.SCK  = BOARD_SPI_SCK_PIN;
    SPI_DEV->PSEL.MOSI = BOARD_SPI_MOSI_PIN;
    SPI_DEV->PSEL.MISO = BOARD_SPI_MISO_PIN;

    // Configure frequency
    if (!set_frequency(conf_p->clock))
    {
        return SPI_RES_INVALID_CONFIG;
    }

    // Configure mode
    if (!set_mode(conf_p->mode, conf_p->bit_order))
    {
        return SPI_RES_INVALID_CONFIG;
    }

    SPI_DEV->ORC = 0xff;

    disable_all_interrupts();

    // Enable SPI IRQ
    Sys_clearFastAppIrq(SPI_IRQn);
    Sys_enableFastAppIrq(SPI_IRQn,
                         APP_LIB_SYSTEM_IRQ_PRIO_HI,
                         SPI_IRQHandler);

    m_current_xfer.free = true;
    m_initialized = true;

    return SPI_RES_OK;
}

spi_res_e SPI_close()
{
    if (!m_initialized)
    {
        return SPI_RES_NOT_INITIALIZED;
    }

    m_initialized = false;

    // Disable SPIM module
    SPI_DEV->ENABLE =
        (SPIM_ENABLE_ENABLE_Disabled << SPIM_ENABLE_ENABLE_Pos);

    Sys_disableAppIrq(SPI_IRQn);

    disable_all_interrupts();

    // Set all gpios as default configuration
    release_gpios();

    return SPI_RES_OK;
}

spi_res_e SPI_transfer(spi_xfer_t * xfer_p,
                       spi_on_transfer_done_cb_f cb)
{
    if (!m_initialized)
    {
        return SPI_RES_NOT_INITIALIZED;
    }

    // Check if a transfer is already ongoing
    if (!m_current_xfer.free)
    {
        return SPI_RES_BUSY;
    }

    // Check transfer
    if (((xfer_p->write_ptr != NULL) && (xfer_p->write_size == 0)) ||
        ((xfer_p->write_ptr == NULL) && (xfer_p->write_size != 0)) ||
        ((xfer_p->read_ptr != NULL) && (xfer_p->read_size == 0)) ||
        ((xfer_p->read_ptr == NULL) && (xfer_p->read_size != 0)))
    {
        return SPI_RES_INVALID_XFER;
    }

    // Check transfer size
    if ((xfer_p->write_size > MAX_XFER_SIZE) || (xfer_p->read_size > MAX_XFER_SIZE))
    {
        return SPI_RES_INVALID_XFER;
    }


    // Enable SPIM module
    SPI_DEV->ENABLE =
        (SPIM_ENABLE_ENABLE_Enabled << SPIM_ENABLE_ENABLE_Pos);

    // Setup the transfer
    m_current_xfer.free = false;
    m_current_xfer.client_xfer = xfer_p;
    m_current_xfer.done = false;
    m_current_xfer.cb = cb;

    reset_xfer_registers();

    if (m_current_xfer.client_xfer->write_ptr != NULL)
    {
        SPI_DEV->TXD.PTR = (uint32_t) m_current_xfer.client_xfer->write_ptr;
        SPI_DEV->TXD.MAXCNT = m_current_xfer.client_xfer->write_size;
    }

    if (m_current_xfer.client_xfer->read_ptr != NULL)
    {
        SPI_DEV->RXD.PTR = (uint32_t) m_current_xfer.client_xfer->read_ptr;
        SPI_DEV->RXD.MAXCNT = m_current_xfer.client_xfer->read_size;
    }

    // Prepare the interrupt part
    disable_all_interrupts();
    clear_spi_end_event();

    // Enable interrupts we are interested
    SPI_DEV->INTENSET = SPIM_INTENSET_END_Msk;

    // Start the transfer
    SPI_DEV->TASKS_START = 0x1UL;

    // Is it a blocking call
    if (m_current_xfer.cb == NULL)
    {
        // Active wait on end of transfer
        while (!m_current_xfer.done);
        m_current_xfer.free = true;
    }

    return SPI_RES_OK;
}

/**
 * \brief   Function to handle the SPI Interrupt.
 */
void __attribute__((__interrupt__)) SPI_IRQHandler(void)
{
    if (SPI_DEV->EVENTS_END != 0)
    {
        clear_spi_end_event();
        // Requested transfer is done
        m_current_xfer.done = true;

        // Disable SPIM module
        SPI_DEV->ENABLE =
            (SPIM_ENABLE_ENABLE_Disabled << SPIM_ENABLE_ENABLE_Pos);

        // If not blocking transfer, then call client cb
        if (m_current_xfer.cb != NULL)
        {
            // Free the transfer before calling cb to chain requests
            m_current_xfer.free = true;
            m_current_xfer.cb(SPI_RES_OK,
                              m_current_xfer.client_xfer);
        }
    }
}
