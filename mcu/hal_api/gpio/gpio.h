/* Copyright 2022 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file    gpio.h
 * \brief   Board-independent GPIO functions.
 */

#ifndef GPIO_H_
#define GPIO_H_

#include <stdint.h>

/** \brief GPIO identification number */
typedef uint8_t gpio_id_t;

/** \brief GPIO port number */
typedef uint8_t gpio_port_t;

/** \brief GPIO pin number */
typedef uint8_t gpio_pin_t;

/** \brief List of return code */
typedef enum
{
    /** Operation is successful */
    GPIO_RES_OK = 0,
    /** GPIO iniatialization has not been performed */
    GPIO_RES_UNINITIALIZED  = 1,
    /** GPIO HAL is not implemented. Weak definitions are used instead */
    GPIO_RES_NOT_IMPLEMENTED = 2,
    /** Invalid parameter(s) */
    GPIO_RES_INVALID_PARAM = 3,
    /** Invalid pin number */
    GPIO_RES_INVALID_PIN   = 4,
    /** Invalid GPIO direction */
    GPIO_RES_INVALID_DIRECTION   = 5,
    /** No free external interrupt: all of them are already in use. Note: Only used on SiLabs/EFR32 boards */
    GPIO_RES_NO_FREE_IT = 6
} gpio_res_e;

/** \brief GPIO logical level */
typedef enum
{
    /** The GPIO is low */
    GPIO_LEVEL_LOW,
    /** The GPIO is high */
    GPIO_LEVEL_HIGH
} gpio_level_e;

/**
 * \brief   GPIO pull configuration
 * \note    Used for input GPIOs
 */
typedef enum
{
    /** input disabled */
    GPIO_IN_DISABLED,
    /** No pull (floating if no external pull-up or pull-down) */
    GPIO_IN_PULL_NONE,
    /** Pull-down  */
    GPIO_IN_PULL_DOWN,
    /** Pull-up  */
    GPIO_IN_PULL_UP
} gpio_in_mode_cfg_e;

/**
 * \brief   GPIO operating mode configuration
 * \note    Used for output GPIOs
 */
typedef enum
{
    /** Push-pull  */
    GPIO_OUT_MODE_PUSH_PULL,
    /** Open-drain  */
    GPIO_OUT_MODE_OPEN_DRAIN,
    /** Open-drain with pull-up  */
    GPIO_OUT_MODE_OPEN_DRAIN_WITH_PULL_UP
} gpio_out_mode_cfg_e;

/**
 * \brief   GPIO event.
 * \note    Used for input GPIOs
 */
typedef enum
{
    /** No event  */
    GPIO_IN_EVENT_NONE = 0,
    /** Rising edge event */
    GPIO_IN_EVENT_RISING_EDGE = 1U << 0U,
    /** Falling edge event  */
    GPIO_IN_EVENT_FALLING_EDGE = 1U << 1U,
} gpio_in_event_e;

/** \brief Check if event has its rising edge bit set */
#define IS_RISING_EDGE(event) ((event & GPIO_IN_EVENT_RISING_EDGE) == GPIO_IN_EVENT_RISING_EDGE)
/** \brief Check if event has its falling edge bit set */
#define IS_FALLING_EDGE(event) ((event & GPIO_IN_EVENT_FALLING_EDGE) == GPIO_IN_EVENT_FALLING_EDGE)

/**
 * \brief   Callback structure for a GPIO event
 * \param   id
 *          Id of the GPIO which raised the event
 * \param   event
 *          Event raised
 */
typedef void (*gpio_in_event_cb_f)(gpio_id_t id, gpio_in_event_e event);

/** \brief GPIO input configuration */
typedef struct
{
    /** Callback called on GPIO events  */
    gpio_in_event_cb_f event_cb;
    /**
     * Event configuration.
     * Use | (OR) operator to detect both rising and falling edges.
     * e.g.: .event_cfg = (GPIO_IN_EVENT_RISING_EDGE | GPIO_IN_EVENT_FALLING_EDGE)
     */
    gpio_in_event_e event_cfg : 2;
    /** Pull configuration (e.g.: Pull-down)  */
    gpio_in_mode_cfg_e in_mode_cfg : 2;
} gpio_in_cfg_t;

/** \brief GPIO output configuration */
typedef struct
{
    /** Operating mode configuration (e.g.: Push-pull) */
    gpio_out_mode_cfg_e out_mode_cfg : 2;
    /** GPIO default logical level (e.g.: Low) */
    gpio_level_e level_default : 1;
} gpio_out_cfg_t;

/**
 * \brief Initialize GPIO module
 *
 * Example on use:
 * @code
 * void App_init(const app_global_functions_t * functions)
 * {
 *     ...
 *     // Set up GPIOs first
 *     Gpio_init();
 *     ...
 *     Gpio_inputSetCfg(GPIO_INPUT_ID, &in_cfg);
 *     ...
 *     Gpio_outputSetCfg(GPIO_OUTPUT_ID, &out_cfg);
 *     ...
 * }
 * @endcode
 */
gpio_res_e Gpio_init(void);

/**
 * \brief       Configure a GPIO as an input GPIO
 * \param       id
 *              Id of the GPIO
 * \param[in]   in_cfg
 *              GPIO input configuration
 * \return      Return code of operation
 */
gpio_res_e Gpio_inputSetCfg(gpio_id_t id, const gpio_in_cfg_t *in_cfg);

/**
 * \brief       Read the GPIO input level
 * \note        The GPIO should be configured as an input GPIO
 * \param       id
 *              Id of the GPIO
 * \param[out]  level
 *              Returned GPIO level (low or high)
 * \return      Return code of operation
 */
gpio_res_e Gpio_inputRead(gpio_id_t id, gpio_level_e *level);

/**
 * \brief       Configure a GPIO as an output GPIO
 * \param       id
 *              Id of the GPIO
 * \param[in]   out_cfg
 *              GPIO output configuration
 * \return      Return code of operation
 */
gpio_res_e Gpio_outputSetCfg(gpio_id_t id, const gpio_out_cfg_t *out_cfg);

/**
 * \brief   Write GPIO output level
 * \note    The GPIO should be configured as an output GPIO
 * \param   id
 *          Id of the GPIO
 * \param   level
 *          GPIO level (low or high) to write
 * \return  Return code of operation
 */
gpio_res_e Gpio_outputWrite(gpio_id_t id, gpio_level_e level);

/**
 * \brief   Toggle GPIO output level
 * \note    The GPIO should be configured as an output GPIO
 * \param   id
 *          Id of the GPIO
 * \return  Return code of operation
 */
gpio_res_e Gpio_outputToggle(gpio_id_t id);

/**
 * \brief       Read the GPIO output level
 * \note        The GPIO should be configured as an output GPIO
 * \param       id
 *              Id of the GPIO
 * \param[out]  level
 *              Returned GPIO level (low or high)
 * \return      Return code of operation
 */
gpio_res_e Gpio_outputRead(gpio_id_t id, gpio_level_e *level);

/**
 * \brief       Get the GPIO port and pin numbers of the given GPIO id
 * \param       id
 *              Id of the GPIO
 * \param[out]  port
 *              Returned GPIO port number
 * \param[out]  pin
 *              Returned GPIO pin number
 *
 * \return      Return code of operation
 */
gpio_res_e Gpio_getPin(gpio_id_t id, gpio_port_t *port, gpio_pin_t *pin);

/**
 * \brief       Get the number of GPIOs
 * \return      Number of GPIOs
 */
uint8_t Gpio_getNumber(void);

#endif /* GPIO_H_ */
