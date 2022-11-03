/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef _STACK_STATE_H_
#define _STACK_STATE_H_

#include "api.h"

#define STACK_STATE_ALL_EVENTS_BITFIELDS    (uint32_t)(-1)

typedef void
    (*stack_state_event_cb_f)(app_lib_stack_event_e event, void * param);

/**
 * @brief   Initialize the stack state library.
 * @return  \ref APP_RES_OK.
 */
app_res_e Stack_State_init(void);

/**
 * @brief   Start the stack
 * @return  Return code of the operation @ref app_res_e
 */
app_res_e Stack_State_startStack();

/**
 * @brief   Stopt the stack
 * @return  Return code of the operation @ref app_res_e
 */
app_res_e Stack_State_stopStack();

/**
 * @brief   Is the stack currently started
 * @return  True if stack is started, False otherwise
 */
bool Stack_State_isStarted();

/**
 * @brief   Add a new callback about event callback.
 * @param   callback
 *          New callback
 * @param   event_bitfield
 *          Bitfields of event to register. To be notified of event n, bit n must be set
 *          For example, to register only for APP_LIB_STATE_STACK_EVENT_SCAN_STARTED and
 *          APP_LIB_STATE_STACK_EVENT_SCAN_STOPPED, bit field is as followed:
 *          uint32_t bitfield = 1 << APP_LIB_STATE_STACK_EVENT_SCAN_STARTED | 1 << APP_LIB_STATE_STACK_EVENT_SCAN_STOPPED
 * @return  APP_RES_OK if ok. See \ref app_res_e for
 *          other result codes.
 */
app_res_e Stack_State_addEventCb(stack_state_event_cb_f callback, uint32_t event_bitfield);

/**
 * @brief   Remove an event callback from the list.
 *          Removed item fields are all set to 0.
 * @param   callback
 *          callback to remove.
 * @return  APP_RES_OK if ok. See \ref app_res_e for
 *          other result codes.
 */
app_res_e Stack_State_removeEventCb(stack_state_event_cb_f callback);

#endif //_STACK_STATE_H_
