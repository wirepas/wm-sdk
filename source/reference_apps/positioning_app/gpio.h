/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    gpio.h
 * \brief   This file implements a simple GPIO interface
 *         
 */

#ifndef _POS_APP_GPIO_H_
#define _POS_APP_GPIO_H_

#include "api.h"

/* the number of pin supported for event monitoring
   each pin can be associated with a single event type and callback */

#define GPIO_MAX 8

typedef enum
{
    GPIO_RES_OK = 0, // Operation is successful
    GPIO_RES_FAIL = 1,
    GPIO_RES_NOT_INITIALIZED = 2
} gpio_res_e;

/**
 * \brief   Supported GPIO event types 
 */
typedef enum
{
    GPIO_EVENT_LH = 0,  // triggers on LOW->HIGH transition
    GPIO_EVENT_HL,      // triggers on HIGH->LOW transition
    GPIO_EVENT_ALL,     // triggers on LOW->HIGH | HIGH->LOW transition
    GPIO_EVENT_MAX
} gpio_event_e;

/**
 * \brief   Supported GPIO pull types 
 */
typedef enum
{
    GPIO_PULLDOWN = 0,
    GPIO_PULLUP,
    GPIO_NOPULL,
    GPIO_PULL_MAX
} gpio_pull_e;

/**
 * \brief   Callback structure for a GPIO event
 * \param   pin 
 *          pin number 
 * \param   event
 *          Event that generated this callback
 */
typedef void (*on_gpio_event_cb)(uint8_t pin,
                                   gpio_event_e event);

/**
 * \brief   Register for a GPIO event on a given pin
 * \param   pin 
 *          pin number  FixMe: !! to be made generic whe EFR32 support added
 * \param   pull 
 *          pull value :GPIO_PULLDOWN / GPIO_PULLUP / GPIO_NOPULL
 * \param   event
 *          Event that generated this callback
 * \param   cb 
 *          callback function to be called when event occured 
 * \return  GPIO_RES_OK if success, GPIO_RES_FAIL if failure (\ref gpio_res_e )
 */
gpio_res_e GPIO_register_for_event(uint8_t pin,
                                    gpio_pull_e pull,
                                    gpio_event_e event,
                                    uint8_t debounce_ms,
                                    on_gpio_event_cb cb);

/**
 * \brief   Deregister for GPIO event on a given pin
 * \param   pin 
 *          pin number 
 * \return  GPIO_RES_OK if success, GPIO_RES_FAIL if failure (\ref gpio_res_e )
 */
gpio_res_e GPIO_deregister_for_event(uint8_t pin);
#endif