/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef _STACK_STATE_H_
#define _STACK_STATE_H_

#include "api.h"

/**
 * \brief   Event to register
 */
typedef enum
{
    /** Stack is enterring stopped state and will reboot soon */
    STACK_STATE_STOPPED_EVENT = 0,
    /** Stack is enterring start state */
    STACK_STATE_STARTED_EVENT = 1,
} stack_state_event_e;

typedef void
    (*stack_state_event_cb_f)(stack_state_event_e event);


/**
 * @brief   Initialize the stack state library.
 * @note    If stack state library is used in application, the
 *          @ref app_lib_system_set_shutdown_cb_f "lib_system->setShutdownCb()",
 *          function offered by system library MUST NOT be used outside of this module.
 *          Neither @ref app_lib_state_start_stack_f "lib_state->startStack()" nor
 *           @ref app_lib_state_stop_stack_f "lib_state->stopStack()"
 * @return  \ref APP_RES_OK.
 */
app_res_e Stack_State_init(void);

/**
 * @brief   Start the stack
 * @note    Starting the stack with this function instead of directly calling
 *          @ref app_lib_state_start_stack_f "lib_state->startStack()" allows the
 *          library to generate the stack started event.
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
 * @return  APP_RES_OK if ok. See \ref app_res_e for
 *          other result codes.
 */
app_res_e Stack_State_addEventCb(stack_state_event_cb_f callback);

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
