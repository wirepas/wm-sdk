/**
 * @brief       Motion module
 * @file        motion.c
 * @copyright   Wirepas Ltd 2021
 */

#ifndef _POSAPP_MOTION_H_
#define _POSAPP_MOTION_H_

#include "acc_interface.h"
#include "poslib.h"

/**
 * @brief Return codes of motion module
 */
typedef enum
{
    /**< Operation is a success. */
    POSAPP_MOTION_RET_OK = 0,
     /**< Operation failed. */
    POSAPP_MOTION_RET_ERROR = 1,
    /**< Incorrect parameter provided. */
    POSAPP_MOTION_RET_INVALID_PARAMETER = 2,
    /**< Motion is not supported. */
    POSAPP_MOTION_RET_NOT_SUPPORTED = 3,
    /**< Accelerometer initialization error */
    POSAPP_MOTION_RET_ACC_INIT_ERROR = 4,
    /**< GPIO registration error */
    POSAPP_MOTION_RET_GPIO_ERROR = 5
} posapp_motion_ret_e;

/**
 * @brief Status of motion module
 */
typedef enum
{
    /**< Motion monitoring is not supported. */
    POSAPP_MOTION_NOT_SUPPORTED = 0,
    /**< Motion monitoring was started. */
    POSAPP_MOTION_MONITORING = 1,
     /**< Acceleration sampling in progress. */
    POSAPP_MOTION_SAMPLING = 2,
    /**< Motion module waiting for requests. */
    POSAPP_MOTION_IDLE = 3,
} posapp_motion_status_e;

#define POSAPP_MOTION_THRESHOLD_MG_MIN 100
#define POSAPP_MOTION_THRESHOLD_MG_MAX 2000
#define POSAPP_MOTION_DURATION_MS_MIN 0
#define POSAPP_MOTION_DURATION_MS_MAX 2000

typedef void (*posapp_motion_mon_callback_f)(poslib_motion_mode_e mode);
typedef void (*posapp_motion_acc_callback_f)(acc_measurement_t * data);

typedef struct
{    
    uint16_t threshold_mg;
    uint16_t duration_ms;
    posapp_motion_mon_callback_f cb;
} posapp_motion_mon_settings_t;

/**
 * \brief   Initialize the motion module and the sensor. 
 *          Must be called before stack start.
 * \param   void 
 * \return  POSAPP_MOTION_RET_OK if success, POSAPP_MOTION_RET_ACC_INIT_ERROR if failure (\ref posapp_motion_ret_e)
 */
posapp_motion_ret_e PosAppMotion_init();

/**
 * \brief   Starts motion monitorig. PosAppMotion_init() shall be called before.
 *          If called when motion monitoring is already started it will update the settings
 * \param   void 
 * \return  POSAPP_MOTION_RET_OK if success, if failures specific codes from \ref posapp_motion_ret_e
 */
posapp_motion_ret_e PosAppMotion_startMonitoring(posapp_motion_mon_settings_t * cfg);

/**
 * \brief   Stops motion monitorig. 
 * \param   void 
 * \return  POSAPP_MOTION_RET_OK
 */
posapp_motion_ret_e PosAppMotion_stopMonitoring();

/**
 * \brief   Gets the status of the motion module. 
 * \param   void 
 * \return  status as \ref posapp_motion_status_e code
 */
posapp_motion_status_e PosAppMotion_getStatus();

/**
 * \brief   Samples the accelerometer and retuns the 3-axis acceleration through a callback.
 *          Is required that motion module was intialized. 
 *          If motion monitoring started it will continue once the acceleration sample is retrived. 
 * \param   void 
 * \return  POSAPP_MOTION_RET_OK if success, if failures specific codes from \ref posapp_motion_ret_e
 */
posapp_motion_ret_e PosAppMotion_getAcceleration(posapp_motion_acc_callback_f cb);

#endif /*_POSAPP_MOTION_H_*/