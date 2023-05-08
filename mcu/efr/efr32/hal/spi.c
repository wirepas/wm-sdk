/* Copyright 2022 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "hal_api.h"
#include "board.h"
#include "api.h"
#include "spi.h"

#include "em_cmu.h"
#include "em_gpio.h"
#include "em_usart.h"

// References:
// [1] EFR32xG23 Wireless SoC Reference Manual 
// https://www.silabs.com/documents/public/reference-manuals/efr32xg23-rm.pdf

#ifndef BOARD_SPI
#error "Please define the required SPI definition in your board.h (BOARD_SPI,…)"
#endif

#define SPI_EXTFLASH_BAUDRATE       8000000

// Dummy byte for SPI transfer
#define DUMMY_BYTE                  0xFF

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

/** Is SPI module initialized */
static bool m_spi_initialized = false;

/***************************************************************************//**
 * @brief       Configure different SPI gpios (MOSI, MISO, SCLK and CS)
 * @param[in]   mode
 *              SPI chosen mode \ref spi_mode_e
 ******************************************************************************/
static void spidrv_configure_gpios(spi_mode_e mode)
{
    // Pin configuration: refer to doc [1] section 24.3.1 Pin Configuration
    // Setup the MOSI pin (TX), PUSHPULL mode
    hal_gpio_set_mode(BOARD_SPI_EXTFLASH_MOSI_PORT,
                      BOARD_SPI_EXTFLASH_MOSI_PIN,
                      GPIO_MODE_OUT_PP);
    hal_gpio_clear(BOARD_SPI_EXTFLASH_MOSI_PORT, BOARD_SPI_EXTFLASH_MOSI_PIN);

    // Setup the MISO pin (RX), INPUT mode
    hal_gpio_set_mode(BOARD_SPI_EXTFLASH_MISO_PORT,
                      BOARD_SPI_EXTFLASH_MISO_PIN,
                      GPIO_MODE_IN_OD_NOPULL);
    hal_gpio_clear(BOARD_SPI_EXTFLASH_MISO_PORT, BOARD_SPI_EXTFLASH_MISO_PIN);

    // Setup the SCLK pin, PUSHPULL mode:
    //    - serial mode 0 and 1 => SCLK idle low
    //    - serial mode 2 and 3 => SCLK idle high)
    hal_gpio_set_mode(BOARD_SPI_EXTFLASH_SCKL_PORT,
                      BOARD_SPI_EXTFLASH_SCKL_PIN,
                      GPIO_MODE_OUT_PP);
    if (SPI_MODE_LOW_FIRST == mode || SPI_MODE_LOW_SECOND == mode)
    {
        hal_gpio_clear(BOARD_SPI_EXTFLASH_SCKL_PORT,
                       BOARD_SPI_EXTFLASH_SCKL_PIN);
    }
    else
    {
        hal_gpio_set(BOARD_SPI_EXTFLASH_SCKL_PORT,
                     BOARD_SPI_EXTFLASH_SCKL_PIN);
    }
}

/***************************************************************************//**
 * @brief   Release different SPI GPIOs (MOSI, MISO, SCLK and CS)
 ******************************************************************************/
static void spidrv_release_gpios(void)
{
    hal_gpio_set_mode(BOARD_SPI_EXTFLASH_MOSI_PORT,
                      BOARD_SPI_EXTFLASH_MOSI_PIN,
                      GPIO_MODE_DISABLED);
    hal_gpio_set_mode(BOARD_SPI_EXTFLASH_MISO_PORT,
                      BOARD_SPI_EXTFLASH_MISO_PIN,
                      GPIO_MODE_DISABLED);
    hal_gpio_set_mode(BOARD_SPI_EXTFLASH_SCKL_PORT,
                      BOARD_SPI_EXTFLASH_SCKL_PIN,
                      GPIO_MODE_DISABLED);
}

/***************************************************************************//**
 * @brief       Convert configuration parameters
 * @param[in]   p_in
 *              SPI driver API configuration parameters \ref spi_conf_t
 * @param[out]  p_out
 *              EFR32 xG2x USART configuration parameters
 *              \ref USART_InitSync_TypeDef
 * @return      true if input parameters are valid, false otherwise
 ******************************************************************************/
static bool spidrv_convert_parameters(spi_conf_t * in_p,
                                      USART_InitSync_TypeDef * out_p)
{
    switch (in_p->mode)
    {
        //< Low Polarity, First Clock edge
        case SPI_MODE_LOW_FIRST:
            out_p->clockMode = usartClockMode0;
            break;
        //< Low Polarity, Second Clock edge
        case SPI_MODE_LOW_SECOND:
            out_p->clockMode = usartClockMode1;
            break;
        //< High Polarity, First Clock edge
        case SPI_MODE_HIGH_FIRST:
            out_p->clockMode = usartClockMode2;
           break;
        //< High Polarity, Second Clock edge
        case SPI_MODE_HIGH_SECOND:
            out_p->clockMode = usartClockMode3;
            break;
        default:
            return false;
    }

    switch (in_p->bit_order)
    {
        //< Most Significant Bit first
        case SPI_ORDER_MSB:
            out_p->msbf = true;
            break;
        //< Less Significant Bit first
        case SPI_ORDER_LSB:
            out_p->msbf = false;
            break;
        default:
           return false;
    }

    if (! in_p->clock)
    {
        return false;
    }
    out_p->baudrate = in_p->clock;

    return true;
}

/***************************************************************************//**
 * @brief       Enable USART clock
 * @param[in]   p_in
 *              SPI driver API configuration parameters \ref spi_conf_t
 * @param[out]  p_out
 *              EFR32 xG2x USART configuration parameters
 *              \ref USART_InitSync_TypeDef
 * @return      true if input parameters are valid, false otherwise
 ******************************************************************************/
static void enable_usart_clock(void)
{
    DS_Disable(DS_SOURCE_USART);
    CMU->CLKEN0_SET = CMU_CLKEN0_USART0;
}

/***************************************************************************//**
 * @brief       Disable USART clock
 * @param[in]   p_in
 *              SPI driver API configuration parameters \ref spi_conf_t
 * @param[out]  p_out
 *              EFR32 xG2x USART configuration parameters
 *              \ref USART_InitSync_TypeDef
 * @return      true if input parameters are valid, false otherwise
 ******************************************************************************/
static void disable_usart_clock(void)
{
    CMU->CLKEN0_CLR = CMU_CLKEN0_USART0;
    DS_Enable(DS_SOURCE_USART);
}

/***************************************************************************//**
 * @brief
 *   Initialize SPI i.e. USART block for synchronous mode.
 *
 * @details
 *   This function will configure basic settings to operate in
 *   synchronous mode.
 *
 * @param[in] conf_p
 *   A pointer to the SPI configuration parameters \ref spi_conf_t
 *
 * @return
 *   \ref SPI_RES_OK if initialization is ok, an error code otherwise
 *   Refer to \ref spi_res_e for other result codes
 ******************************************************************************/
spi_res_e SPI_init(spi_conf_t * conf_p)
{
    if (m_spi_initialized)
    {
        return SPI_RES_ALREADY_INITIALIZED;
    }

    // Check and convert input parameters
    USART_InitSync_TypeDef USART_init = USART_INITSYNC_DEFAULT;
    USART_init.enable = usartDisable;
    if (! conf_p || false == spidrv_convert_parameters(conf_p, &USART_init))
    {
        return SPI_RES_INVALID_CONFIG;
    }

    // Enable GPIO clock
    CMU->CLKEN0_SET = CMU_CLKEN0_GPIO;

    // Configure GPIO
    spidrv_configure_gpios(conf_p->mode);

    // Enable USART clock during the configuration
    enable_usart_clock();

    // Configure UART in sync mode
    USART_InitSync(BOARD_SPI, &USART_init);

    // Set UART routes
    // Refer to doc [1], sections:
    //  - 24.6.192 GPIO_USART0_TXROUTE - TX Port/Pin Select
    //  - 24.6.190 GPIO_USART0_RXROUTE - RX Port/Pin Select
    //  - 24.6.191 GPIO_USART0_CLKROUTE - SCLK Port/Pin Select
    BOARD_SPIROUTE.TXROUTE = 
        (BOARD_SPI_EXTFLASH_MOSI_PORT << _GPIO_USART_TXROUTE_PORT_SHIFT) |
        (BOARD_SPI_EXTFLASH_MOSI_PIN << _GPIO_USART_TXROUTE_PIN_SHIFT);

    BOARD_SPIROUTE.RXROUTE = 
        (BOARD_SPI_EXTFLASH_MISO_PORT << _GPIO_USART_RXROUTE_PORT_SHIFT) |
        (BOARD_SPI_EXTFLASH_MISO_PIN << _GPIO_USART_RXROUTE_PIN_SHIFT);

    BOARD_SPIROUTE.CLKROUTE = 
        (BOARD_SPI_EXTFLASH_SCKL_PORT << _GPIO_USART_CLKROUTE_PORT_SHIFT) |
        (BOARD_SPI_EXTFLASH_SCKL_PIN << _GPIO_USART_CLKROUTE_PIN_SHIFT);

    // Enable the routes
    // Refer to doc [1] 24.6.186 GPIO_USART0_ROUTEEN - USART0 Pin Enable
    BOARD_SPIROUTE.ROUTEEN = GPIO_USART_ROUTEEN_TXPEN |
                               GPIO_USART_ROUTEEN_RXPEN |
                               GPIO_USART_ROUTEEN_CLKPEN;


    // Enable data transmission and reception
    // Refer to doc [1] section 20.5.3 USART_CTRL - Control Register
    BOARD_SPI->CMD = USART_CMD_RXEN | USART_CMD_TXEN;

    // Configuration done: disable USART clock
    disable_usart_clock();

    // Initialize the transfer structure
    memset(&m_current_xfer, 0x00, sizeof(m_current_xfer));
    m_current_xfer.free = true;

    m_spi_initialized = true;

    return SPI_RES_OK;
}

/***************************************************************************//**
 * @brief
 *  Deinitialize SPI
 *
 * @return
 *   \ref SPI_RES_OK if initialization is ok, an error code otherwise
 *   Refer to \ref spi_res_e for other result codes
 ******************************************************************************/
 spi_res_e SPI_close(void)
{
    if (! m_spi_initialized)
    {
        return SPI_RES_NOT_INITIALIZED;
    }

    // Enable USART clock
    enable_usart_clock();

    // Reser USART registers to their default configuration
    USART_Reset(BOARD_SPI);

    // Disable USART clock
    disable_usart_clock();

    // Disable the routes
    BOARD_SPIROUTE.RXROUTE  = _GPIO_USART_RXROUTE_RESETVALUE;
    BOARD_SPIROUTE.TXROUTE  = _GPIO_USART_TXROUTE_RESETVALUE;
    BOARD_SPIROUTE.CLKROUTE = _GPIO_USART_CLKROUTE_RESETVALUE;
    BOARD_SPIROUTE.ROUTEEN  = _GPIO_USART_ROUTEEN_RESETVALUE;

    // Set all GPIOs as default configuration
    spidrv_release_gpios();

    m_spi_initialized = false;

    return SPI_RES_OK;
}

/***************************************************************************//**
 * @brief
 *   SPI_transfer
 *
 * @details
 *
 * @param[in] xfer_p
 *   A pointer to the structure describing a SPI transfer \ref spi_xfer_t
 * @param[in] cb
 *   Callback to be called when the transfer is done
 *
 * @return
 *   \ref SPI_RES_OK if the transfer is ok, an error code otherwise
 *   Refer to \ref spi_res_e for other result codes
 ******************************************************************************/
spi_res_e SPI_transfer(spi_xfer_t * xfer_p,
                       spi_on_transfer_done_cb_f cb)
{
    uint32_t i;

    if (! m_spi_initialized)
    {
        return SPI_RES_NOT_INITIALIZED;
    }

    // Check if a transfer is already ongoing
    if (! m_current_xfer.free)
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

    // We just support synchronous transfers
    if (NULL != cb)
    {
        return SPI_RES_INVALID_XFER;
    }

    // Enable USART clock
    enable_usart_clock();

    // Setup the transfer
    m_current_xfer.free = false;
    m_current_xfer.client_xfer = xfer_p;
    m_current_xfer.done = false;
    m_current_xfer.cb = NULL;

    // Send the command
    for (i = 0; i < m_current_xfer.client_xfer->write_size; i++)
    {
        uint8_t rsp;
        rsp = USART_SpiTransfer(BOARD_SPI,
                                m_current_xfer.client_xfer->write_ptr[i]);
        if (i < m_current_xfer.client_xfer->read_size)
        {
            m_current_xfer.client_xfer->read_ptr[i] = rsp;
        }
    }

    // Read the data if requested
    if (m_current_xfer.client_xfer->read_size > 
                                        m_current_xfer.client_xfer->write_size)
    {
        for (i = m_current_xfer.client_xfer->write_size;
             i <  m_current_xfer.client_xfer->read_size; i++)
        {
            m_current_xfer.client_xfer->read_ptr[i] = 
                                    USART_SpiTransfer(BOARD_SPI, DUMMY_BYTE);
        }
    }

    m_current_xfer.done = true;
    m_current_xfer.free = true;

    // Disable clocks
    disable_usart_clock();

    return SPI_RES_OK;
}
