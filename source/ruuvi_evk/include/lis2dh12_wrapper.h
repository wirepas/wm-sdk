/**
    @file   lis2dh12_wrapper.h
    @brief  Wrapper on top of LIS2DH12 driver from STMicroelectronics GitHub.

    This file is the wrapper to expose a simple API for getting the acceleration
    from lis2dh12 device. It makes the glue between the ST driver and Wirepas
    SPI driver and staticaly configure the sensor.

    @copyright  Wirepas Oy 2019
*/
#ifndef LIS2DH12_WRAPPER_H_
#define LIS2DH12_WRAPPER_H_

/**
    @brief Structure containing acceleration measurements.
*/
typedef struct
{
    int32_t accel_x; /**< Acceleration on X axis in mg [-2g / +2g]. */
    int32_t accel_y; /**< Acceleration on Y axis in mg [-2g / +2g]. */
    int32_t accel_z; /**< Acceleration on Z axis in mg [-2g / +2g]. */
} lis2dh12_wrapper_measurement_t;

/**
    @brief  Initialize the LIS2DH12 wrapper library.
    @return True if successfully initialized, false otherwise.
*/
bool LIS2DH12_wrapper_init(void);

/**
    @brief  Start a measurement.
    @return The time in ms to wait before the measurement is ready.
*/
uint32_t LIS2DH12_wrapper_startMeasurement(void);

/**
    @brief     Get a measurement previously asked by a start measurement.
    @param[in] measurement The measurement read out from the sensor.
    @return    false if a problem occured, true otherwise.
*/
bool LIS2DH12_wrapper_readMeasurement(
                                lis2dh12_wrapper_measurement_t * measurement);

#endif /* LIS2DH12_WRAPPER_H_ */
