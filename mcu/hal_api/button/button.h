/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file    button.h
 * \brief   Board-independent button functions
 */

#ifndef BUTTON_H_
#define BUTTON_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * \brief   Different events for a button
 */
typedef enum {
    BUTTON_PRESSED,
    BUTTON_RELEASED
} button_event_e;

/**
 * \brief   List of return code
 */
typedef enum
{
    /** Operation is successful */
    BUTTON_RES_OK = 0,
    /** Button id is invalid */
    BUTTON_RES_INVALID_ID = 1,
    /** Function parameter is invalid */
    BUTTON_RES_INVALID_PARAM = 2,
    /** Button initialization has not been performed */
    BUTTON_RES_UNINITIALIZED = 3
} button_res_e;

/**
 * \brief   Callback structure for a button event
 * \param   button_id
 *          Id of the button that is pressed
 * \param   event
 *          Event that generated this callback
 */
typedef void (*on_button_event_cb)(uint8_t button_id,
                                   button_event_e event);

/**
 * \brief   Initialize Button module
 */
void Button_init(void);

/**
 * \brief   Register for a button event
 * \param   button_id
 *          Id of the button
 * \param   event
 *          Event to monitor
 * \param   cb
 *          Callback to call when event happen
 * \return  Return code of operation
 */
button_res_e Button_register_for_event(uint8_t button_id,
                                       button_event_e event,
                                       on_button_event_cb cb);

/**
 * \brief   Get State of a given button
 * \param   button_id
 *          Id of the button
 * \param   state_p
 *          state of the button, true for pressed
 *          Valid only if return code is BUTTON_RES_OK
 * \return  Return code of operation
 */
button_res_e Button_getState(uint8_t button_id,
                             bool * state_p);

/**
 * \brief   Get number of buttons
 */
uint8_t Button_get_number(void);

#endif /* BUTTON_H_ */
