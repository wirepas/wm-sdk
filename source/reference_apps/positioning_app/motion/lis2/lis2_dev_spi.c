/**
 * @file  lis2_spi.c
 * @brief Defines read/write functions for STMeme device.
 * @copyright  Wirepas Ltd 2021
 */

#define DEBUG_LOG_MODULE_NAME "LIS2SPI"
#ifdef DEBUG_POSLIB_LOG_MAX_LEVEL
#define DEBUG_LOG_MAX_LEVEL DEBUG_POSLIB_LOG_MAX_LEVEL
#else
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#endif

#include "string.h"
#include "board.h"
#include "hal_api.h"
#include "debug_log.h"
#include "spi.h"
#include "api.h"
#include "lis2_dev.h"

#ifdef LIS2DH12_SPI
#define TX_MASK_SPI_READ_MODE 0xC0
#define TX_MASK_SPI_WRITE_MODE 0x40
#elif defined LIS2DW12_SPI
#define TX_MASK_SPI_READ_MODE 0x80
#define TX_MASK_SPI_WRITE_MODE 0x00
#endif

/** Maximum SPI write transfer */
#define MAX_WRITE_SIZE             16

/** Maximum SPI read transfer */
#define MAX_READ_SIZE              16

static spi_conf_t m_spi_conf = {
    .bit_order = SPI_ORDER_MSB,
    .clock = 4000000,
    .mode = SPI_MODE_HIGH_FIRST,
};

/**
    @brief     Select or unselect LIS2DH12 with its chip select signal.
    @param[in] select True to select and False to unselect.
 */
void lis2_select_chip(bool select)
{
    if (select)
    {
        nrf_gpio_pin_clear(BOARD_SPI_LIS2_CS_PIN);
    }
    else
    {
        nrf_gpio_pin_set(BOARD_SPI_LIS2_CS_PIN);
    }
}

/**
    @brief     Read from SPI (function required by STMicroelectronics lib).
    @param[in] handle SPI driver id (unused here).
    @param[in] reg First register to read.
    @param[in] bufp Pointer to store read registers.
    @param[in] len Number of registers (of 1 byte) to read.
*/
static int32_t lis2_spi_read(void * handle,
                                 uint8_t reg,
                                 uint8_t * bufp,
                                 uint16_t len)
{
    spi_res_e res;
    uint8_t tx[1];
    uint8_t rx[MAX_READ_SIZE + 1];

    if (len > MAX_READ_SIZE)
    {
        return -1;
    }

    res = SPI_init(&m_spi_conf);

    tx[0] = reg | TX_MASK_SPI_READ_MODE;

    spi_xfer_t transfer;
    transfer.write_ptr = tx;
    transfer.write_size = 1;
    transfer.read_ptr = rx;
    transfer.read_size = len + 1;

    lis2_select_chip(true);
    // Blocking read
    res = SPI_transfer(&transfer, NULL);
    lis2_select_chip(false);

    if (res != SPI_RES_OK)
    {
        return -1;
    }

    memcpy(bufp, &rx[1], len);

    return 0;
}

/**
    @brief      Write with SPI (function required by STMicroelectronics lib).
    @param[in]  handle SPI driver id (unused here).
    @param[in]  reg First register to write to.
    @param[out] bufp Pointer in RAM to the data to be written.
    @param[in]  len Number of registers (of 1 byte) to write.
*/
static int32_t lis2_spi_write(void *handle,
                               uint8_t reg,
                               uint8_t *bufp,
                               uint16_t len)
{
    spi_res_e res;
    uint8_t tx[MAX_WRITE_SIZE + 1];

    if (len > MAX_WRITE_SIZE)
    {
        return -1;
    }

    tx[0] = reg | TX_MASK_SPI_WRITE_MODE;
    memcpy(&tx[1], bufp, len);

    spi_xfer_t transfer;
    transfer.write_ptr = tx;
    transfer.write_size = len + 1;
    transfer.read_ptr = NULL;
    transfer.read_size = 0;

    lis2_select_chip(true);
    // Blocking write
    res = SPI_transfer(&transfer, NULL);
    lis2_select_chip(false);

    if (res != SPI_RES_OK)
    {
        return -1;
    }

    return 0;
}

void LIS2_dev_init(stmdev_ctx_t * dev)
{
    nrf_gpio_pin_dir_set(BOARD_SPI_LIS2_CS_PIN, NRF_GPIO_PIN_DIR_OUTPUT);
    nrf_gpio_cfg_output(BOARD_SPI_LIS2_CS_PIN);
    nrf_gpio_pin_set(BOARD_SPI_LIS2_CS_PIN);
    LOG(LVL_DEBUG, "BOARD_SPI_LIS2_CS_PIN: %u", BOARD_SPI_LIS2_CS_PIN);
    dev->handle = NULL;
    dev->write_reg = lis2_spi_write;
    dev->read_reg = lis2_spi_read;
}