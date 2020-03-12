/**
    @file   bme280_wrapper.h
    @brief  Wrapper on top of BME280 driver from Bosh GitHub project.

    This file is the wrapper to expose a simple API for getting the temperature,
    pressure and humidity out of device for a "weather application"
    configuration. It makes the glue between the Bosh driver and Wirepas SPI
    driver and staticaly configure the sensor in a given mode.

    @copyright  Wirepas Oy 2019
*/
#ifndef BME280_WRAPPER_H_
#define BME280_WRAPPER_H_

/**
    @brief Structure containing sensor measurements.
*/
typedef struct
{
    int32_t  temperature;    /**< Temperature in 0.01°C. */
    uint32_t humidity;       /**< Humidity in 1/1024 % relative humidity
                                                                (1% is 1024). */
    uint32_t pressure;       /**< Pressure in 0.01 Pascal. */
} bme280_wrapper_measurement_t;

/**
    @brief  Initialize the BME280 wrapper library.
    @return True if successfully initialized, false otherwise.
*/
bool BME280_wrapper_init(void);

/**
    @brief  Start a measurement.
    @return The time in ms to wait before the measurement is ready.
*/
uint32_t BME280_wrapper_startMeasurement(void);

/**
    @brief     Get a measurement previously asked by a start measurement.
    @param[in] measurement The measurement read out from the sensor.
    @return    false if a problem occured, true otherwise.
 */
bool BME280_wrapper_readMeasurement(bme280_wrapper_measurement_t * measurement);

#endif /* BME280_WRAPPER_H_ */
