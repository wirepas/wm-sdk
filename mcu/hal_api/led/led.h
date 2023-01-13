/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file    led.h
 * \brief   Board-independent LED functions
 */

#ifndef LED_H_
#define LED_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * \brief   List of return code
 */
typedef enum
{
    /** Operation is successful */
    LED_RES_OK = 0,
    /** Led id is invalid */
    LED_RES_INVALID_ID = 1,
    /** LED iniatialization has not been performed */
    LED_RES_UNINITIALIZED = 2
} led_res_e;

/**
 * \brief Initialize Led module
 *
 * Example on use:
 * @code
 * void App_init(const app_global_functions_t * functions)
 * {
 *     ...
 *     // Set up LED GPIO
 *     Led_init();
 *     ...
 * }
 *
 * @endcode
 */
void Led_init(void);

/**
 * \brief   Turn the given LED on or off
 *
 * Example on use:
 * @code
 *
 * #define LED_NUMBER          0
 *
 * void App_init(const app_global_functions_t * functions)
 * {
 *     ...
 *     // Set up LED GPIO
 *     Led_init();
 *     Led_set(LED_NUMBER, true);
 *     ...
 * }
 * @endcode
 *
 * \param   led_id
 *          Id of the led
 * \param   state
 *          True to switch ON, False to switch OFF
 * \return  True if successful, false otherwise
 */
led_res_e Led_set(uint8_t led_id, bool state);

/**
 * \brief   Get the given LED current state
 *
 * \param   led_id
 *          Id of the led
 * \return  True if ON , false if OFF
 */
bool Led_get (uint8_t led_id);

/**
 * \brief   Toggle the given LED
 * \param   led_id
 *          Id of the led
 * \return  True if successful, false otherwise
 */
led_res_e Led_toggle(uint8_t led_id);

/**
 * \brief   Get number of leds available
 * \return  The number of leds available
 */
uint8_t Led_getNumber(void);

#endif /* LED_H_ */
