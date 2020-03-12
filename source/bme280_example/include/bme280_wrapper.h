/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \brief   Wrapper on top of BME280 driver from Bosh GitHub project
 *          It makes the glue between the bosh driver and Wirepas SPI driver
 *          and staticaly configure the sensor in a given mode
 */
#ifndef BME280_WRAPPER_H_
#define BME280_WRAPPER_H_

typedef struct
{
    uint32_t pressure;      //< Pressure
    int32_t  temperature;   //< Temperature
    uint32_t humidity;      //< Humidity
} bme280_wrapper_measurement_t;

/**
 * \brief   Initialize the BME280 wrapper library
 * \return  True if successfully initialized, false otherwise
 */
bool BME280_wrapper_init(void);

/**
 * \brief   Start a measurement
 * \return  The time in ms to wait before the measurement is ready
 */
uint32_t BME280_wrapper_start_measurement(void);

/**
 * \brief   Get a measurement previously asked by a start measurement
 * \param   measurement
 *          The measurement read out from the sensor
 */
bool BME280_wrapper_read_measurement(bme280_wrapper_measurement_t * measurement);


#endif /* BME280_WRAPPER_H_ */
