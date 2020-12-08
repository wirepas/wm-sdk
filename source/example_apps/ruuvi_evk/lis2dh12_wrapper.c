/**
    @file  lis2dh12_wrapper.c
    @brief Wrapper on top of LIS2DH12 driver from STMicroelectronics GitHub.
    @copyright  Wirepas Oy 2019
*/
#include "lis2dh12_reg.h"
#include "board.h"
#include "hal_api.h"
#include "spi.h"
#include "api.h"
#include <string.h>
#include "lis2dh12_wrapper.h"

/* Time to get first measure from POWER DOWN @1Hz, with 50% margin. */
#define LIS2DH12_WAKE_UP_TIME_MS (7*1.5)

/* Constant to convert raw acceleration data to mg. */
#define RAW_ACCEL_TO_MG 16

/* Maximum SPI write transfer. */
#define MAX_WRITE_SIZE 16

/* Maximum SPI read transfer. */
#define MAX_READ_SIZE 16

/* Reference to the lis2dh12 driver. */
static lis2dh12_ctx_t m_lis2dh12_dev;

/**
    @brief     Blocking wait for a given amount of time.
    @param[in] period Time to wait in ms.
*/
static void lis2dh12_delay_ms(uint32_t period)
{
    app_lib_time_timestamp_hp_t end;
    end = lib_time->addUsToHpTimestamp(lib_time->getTimestampHp(),
                                       period * 1000);

    /* Active wait until period is elapsed */
    while (lib_time->isHpTimestampBefore(lib_time->getTimestampHp(),
                                         end));
}

/**
    @brief     Select or unselect LIS2DH12 with its chip select signal.
    @param[in] select True to select and false to unselect.
 */
void lis2dh12_select_chip(bool select)
{
    if (select)
    {
        nrf_gpio_pin_clear(BOARD_SPI_LIS2DH12_CS_PIN);
    }
    else
    {
        nrf_gpio_pin_set(BOARD_SPI_LIS2DH12_CS_PIN);
    }
}

/**
    @brief     Read from SPI (function required by STMicroelectronics lib).
    @param[in] handle SPI driver id (unused here).
    @param[in] reg First register to read.
    @param[in] bufp Pointer to store read registers.
    @param[in] len Number of registers (of 1 byte) to read.
*/
static int32_t lis2dh12_spi_read(void * handle,
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

    tx[0] = reg | 0xC0;

    spi_xfer_t transfer;
    transfer.write_ptr = tx;
    transfer.write_size = 1;
    transfer.read_ptr = rx;
    transfer.read_size = len + 1;

    lis2dh12_select_chip(true);
    /* Blocking read. */
    res = SPI_transfer(&transfer, NULL);
    lis2dh12_select_chip(false);

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
static int32_t lis2dh12_spi_write(void *handle,
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

    tx[0] = reg | 0x40;
    memcpy(&tx[1], bufp, len);

    spi_xfer_t transfer;
    transfer.write_ptr = tx;
    transfer.write_size = len + 1;
    transfer.read_ptr = NULL;
    transfer.read_size = 0;

    lis2dh12_select_chip(true);
    /* Blocking write */
    res = SPI_transfer(&transfer, NULL);
    lis2dh12_select_chip(false);

    if (res != SPI_RES_OK)
    {
        return -1;
    }

    return 0;
}

bool LIS2DH12_wrapper_init(void)
{
    uint8_t whoamI;

    /*
     * Create device for LIS2DH12 driver with the adapted read/write functions.
     */
    m_lis2dh12_dev.write_reg = lis2dh12_spi_write;
    m_lis2dh12_dev.read_reg = lis2dh12_spi_read;
    m_lis2dh12_dev.handle = NULL;

    /* Set to Power Down. */
    lis2dh12_data_rate_set(&m_lis2dh12_dev, LIS2DH12_POWER_DOWN);
    lis2dh12_delay_ms(10);

    /* Check the device ID. */
    lis2dh12_device_id_get(&m_lis2dh12_dev, &whoamI);
    if (whoamI != LIS2DH12_ID)
    {
        /* Device not found or did not respond. */
        return false;
    }

    /* Reload memory. */
    lis2dh12_boot_set(&m_lis2dh12_dev, 1);
    lis2dh12_delay_ms(10);

    /* Enable Block Data Update. */
    lis2dh12_block_data_update_set(&m_lis2dh12_dev, PROPERTY_ENABLE);

    /* Set full scale to 2g. */
    lis2dh12_full_scale_set(&m_lis2dh12_dev, LIS2DH12_2g);

    /* Set device resolution to 12 bits. */
    lis2dh12_operating_mode_set(&m_lis2dh12_dev, LIS2DH12_HR_12bit);

    return true;
}

uint32_t LIS2DH12_wrapper_startMeasurement(void)
{
    lis2dh12_data_rate_set(&m_lis2dh12_dev, LIS2DH12_ODR_1Hz);

    /*
     * According to datasheet, wake up time from power down to high resolution
     * @1Hz is 7ms.
     */
    return LIS2DH12_WAKE_UP_TIME_MS;
}

bool LIS2DH12_wrapper_readMeasurement(
                                lis2dh12_wrapper_measurement_t * measurement)
{
    axis3bit16_t data_raw_acceleration;

    /* Read accelerometer data. */
    memset(data_raw_acceleration.u8bit, 0x00, 3*sizeof(int16_t));
    lis2dh12_acceleration_raw_get(&m_lis2dh12_dev, data_raw_acceleration.u8bit);

    /* Put the accelerometer back to sleep. */
    lis2dh12_data_rate_set(&m_lis2dh12_dev, LIS2DH12_POWER_DOWN);

    measurement->accel_x = data_raw_acceleration.i16bit[0] / RAW_ACCEL_TO_MG;
    measurement->accel_y = data_raw_acceleration.i16bit[1] / RAW_ACCEL_TO_MG;
    measurement->accel_z = data_raw_acceleration.i16bit[2] / RAW_ACCEL_TO_MG;

    return true;
}
