/**
    @file   app_config.h
    @brief  This module manages the application configuration.
    @copyright  Wirepas Oy 2019
*/
#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_

#include <stdint.h>
#include <stdbool.h>

/**
    @brief Structure containing the application configuration.
*/
typedef struct
{
    bool temperature_enable;    /**< Enable temperature sensor. */
    bool humidity_enable;       /**< Enable humidity sensor. */
    bool pressure_enable;       /**< Enable pressure sensor. */
    bool accel_x_enable;        /**< Enable X-axis acceleration sensor. */
    bool accel_y_enable;        /**< Enable Y-axis acceleration sensor. */
    bool accel_z_enable;        /**< Enable Z-axis acceleration sensor. */
    uint32_t sensors_period_ms; /**< Period in ms at wich rate sensor data are
                                                            sent to the Sink. */
} app_config_t;

/**
    @brief Function type config change callback.
*/
typedef void (*on_config_change_cb_f)(void);

/**
    @brief     Initialize the App Config module.
    @param[in] cb Pointer to a callback function. NULL if not used.
*/
void App_Config_init(on_config_change_cb_f cb);

/**
    @brief Returns a pointer to the configuration structure.
*/
const app_config_t * App_Config_get(void);

#endif /* APP_CONFIG_H_ */
