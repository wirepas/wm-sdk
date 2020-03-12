/**
    @file  bme280_wrapper.c
    @brief Wrapper on top of BME280 driver from Bosch GitHub.
    @copyright  Wirepas Oy 2019
*/
#include "bme280.h"
#include "board.h"
#include "hal_api.h"
#include "spi.h"
#include "api.h"
#include <string.h>
#include "bme280_wrapper.h"

/* Time to get one measurement with 50% margin. */
#define BME280_MEASUREMENT_TIME_MS (9.3*1.5)

/* Maximum SPI write transfer. */
#define MAX_WRITE_SIZE  32

/* Maximum spi read transfer. */
#define MAX_READ_SIZE  32

/* Reference to the bme280 driver. */
static struct bme280_dev m_bme280_dev;

/**
    @brief     Blocking wait for a given amount of time.
    @param[in] period Time to wait in ms.
    @note      Lib doesn't use it a lot and for only short period mainly at
               init stage executed from app_init.
*/
static void bme280_delay_ms(uint32_t period)
{
    app_lib_time_timestamp_hp_t end;
    end = lib_time->addUsToHpTimestamp(lib_time->getTimestampHp(),
                                       period * 1000);

    /* Active wait until period is elapsed */
    while (lib_time->isHpTimestampBefore(lib_time->getTimestampHp(),
                                         end));
}

/**
    @brief     Select or unselect BME280 with its chip select signal.
    @param[in] select True to select and false to unselect.
*/
void bme280_select_chip(bool select)
{
    if (select)
    {
        nrf_gpio_pin_clear(BOARD_SPI_BME280_CS_PIN);
    }
    else
    {
        nrf_gpio_pin_set(BOARD_SPI_BME280_CS_PIN);
    }
}

/**
    @brief     Read from spi (function required by Bosh Lib).
    @param[in] dev_id Device id (unused here).
    @param[in] reg_addr First register to read.
    @param[in] reg_data Pointer to store read registers.
    @param[in] len Number of registers (of 1 byte) to read.
*/
static int8_t bme280_spi_read(uint8_t dev_id,
                            uint8_t reg_addr,
                            uint8_t *reg_data,
                            uint16_t len)
{
    spi_res_e res;
    uint8_t tx[1];
    uint8_t rx[MAX_READ_SIZE];

    if (len > MAX_READ_SIZE)
    {
        return -1;
    }

    tx[0] = reg_addr;

    spi_xfer_t transfer;
    transfer.write_ptr = tx;
    transfer.write_size = 1;
    transfer.read_ptr = rx;
    transfer.read_size = len + 1;

    bme280_select_chip(true);
    /* Blocking read. */
    res = SPI_transfer(&transfer, NULL);
    bme280_select_chip(false);

    if (res != SPI_RES_OK)
    {
        return -1;
    }

    memcpy(reg_data, &rx[1], len);
    return 0;
}

/**
    @brief     Write with spi (function required by Bosh Lib).
    @param[in] dev_id Device id (unused here).
    @param[in] reg_addr First register to write.
    @param[in] reg_data Pointer to value to write.
    @param[in] len Number of registers (of 1 byte) to write.
*/
static int8_t bme280_spi_write(uint8_t dev_id,
                             uint8_t reg_addr,
                             uint8_t *reg_data,
                             uint16_t len)
{
    spi_res_e res;
    uint8_t tx[MAX_WRITE_SIZE + 1];

    if (len > MAX_WRITE_SIZE)
    {
        return -1;
    }

    tx[0] = reg_addr;
    memcpy(&tx[1], reg_data, len);

    spi_xfer_t transfer;
    transfer.write_ptr = tx;
    transfer.write_size = len + 1;
    transfer.read_ptr = NULL;
    transfer.read_size = 0;

    bme280_select_chip(true);
    /* Blocking write. */
    res = SPI_transfer(&transfer, NULL);
    bme280_select_chip(false);

    if (res != SPI_RES_OK)
    {
        return -1;
    }

    return 0;
}

bool BME280_wrapper_init(void)
{
    uint8_t settings_sel;

    /* Create device for BME280 driver with the adapted read/write functions. */
    m_bme280_dev.dev_id = 0;
    m_bme280_dev.intf = BME280_SPI_INTF;
    m_bme280_dev.read = bme280_spi_read;
    m_bme280_dev.write = bme280_spi_write;
    m_bme280_dev.delay_ms = bme280_delay_ms;

    if (bme280_init(&m_bme280_dev) != BME280_OK)
    {
        return false;
    }

    /*
     * Configure the mode of operation. It has a "Weather measurement" config
     * as described in RM.
     */
    m_bme280_dev.settings.osr_h = BME280_OVERSAMPLING_1X;
    m_bme280_dev.settings.osr_p = BME280_OVERSAMPLING_1X;
    m_bme280_dev.settings.osr_t = BME280_OVERSAMPLING_1X;
    m_bme280_dev.settings.filter = BME280_FILTER_COEFF_OFF;
    settings_sel = BME280_OSR_PRESS_SEL |
                   BME280_OSR_TEMP_SEL |
                   BME280_OSR_HUM_SEL |
                   BME280_FILTER_SEL;

    if (bme280_set_sensor_settings(settings_sel, &m_bme280_dev) != BME280_OK)
    {
        return false;
    }

    return true;
}

uint32_t BME280_wrapper_startMeasurement(void)
{
    int8_t rslt;

    /* Start a measurement. */
    rslt = bme280_set_sensor_mode(BME280_FORCED_MODE, &m_bme280_dev);
    (void) rslt;

    /*
     * According to our config, measurement max time is 9.3ms. Time can be
     * computed according to chapter 9.3 of BME280 RM.
     */
    return BME280_MEASUREMENT_TIME_MS;
}

bool BME280_wrapper_readMeasurement(bme280_wrapper_measurement_t * measurement)
{
    struct bme280_data comp_data;
    int8_t rslt;

    rslt = bme280_get_sensor_data(BME280_ALL, &comp_data, &m_bme280_dev);
    if (rslt != BME280_OK)
    {
        return false;
    }

    measurement->humidity = comp_data.humidity;
    measurement->pressure = comp_data.pressure;
    measurement->temperature = comp_data.temperature;

    return true;
}
