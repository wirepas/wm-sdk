/**
 * @file        acc_interface.h
 * @brief       Defined the accelerometer interface implemented by a 
 *              particular accelerometer wrapper
 * @copyright   Wirepas Ltd 2021
 */
#ifndef _ACC_INTERFACE_H_
#define _ACC_INTERFACE_H_

/*LIS12DH12*/
#if defined LIS2DH12_I2C || defined LIS2DH12_SPI
#include "lis2dh12_wrapper.h"
#endif

/*LIS12DW12*/
#if defined LIS2DW12_I2C || defined LIS2DW12_SPI
#include "lis2dw12_wrapper.h"
#endif

/**
 * @brief Structure containing acceleration measurement.
 */
typedef struct
{
    int32_t x; /**< Acceleration on X axis in mg */
    int32_t y; /**< Acceleration on Y axis in mg */
    int32_t z; /**< Acceleration on Z axis in mg */
} acc_measurement_t;

/**
 * @brief   Initialize the accelerometer and the wrapper
 * @return  true if successfully initialized, false if error occured
 */
bool ACC_init();

/**
 * @brief   Starts a measurement. 
 * @param   wait_ms The time in ms to wait before the measurement is ready. 
 *          @note ACC_readMeasurement shall be called only after the wait_ms ellapsed.
 *        
 * @return  true if started, false if error occured
 */
bool ACC_startMeasurement(uint32_t * wait_ms);

/**
 * @brief   Reads the accelerometer measurement ( @note measurement shall be started before)
 *          Once acceleration value is read the sensor is either: (a) placed in the lowest power 
 *          mode of (b) if motion monitoring was enabled before opperation will be resumed
 * @param   meas acceleration measurement.
 * @return  true if measurement valid, false if error occured.
 */
bool ACC_readMeasurement(acc_measurement_t * meas);

/**
 * @brief   Enable motion monitoring
 * @param   threshold_mg acceleration threshold value [mg]
 * @param   duration_ms duration for acceleration to be above threshold [ms]
 * @return  true if enabled, false if error occured.
 */
bool ACC_enableMonitoring(uint16_t threshold_mg, uint16_t duration_ms);

/**
 * @brief   Dissble motion monitoring
 * @return  true if dissabled, false if error occured.
 */
void ACC_disableMonitoring(void);
#endif /* _ACC_INTERFACE_T_ */
