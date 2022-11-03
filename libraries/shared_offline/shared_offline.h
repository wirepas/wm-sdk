/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file shared_offline.h
 *
 * This library allows multiple application modules to control the NRLS feature
 * of the stack at the same time without conflicts.
 * Ie, when the stack can freely disconnect from the network and enter offline
 * mode.
 * The purpose of this mode is to save energy.
 *
 * @note    Switching too often between online and offline mode may have the
 *          opposite desired effect. In fact, each time a node will be back
 *          in online mode, it will trigger a scan and consume additional power.
 * @note    This library can only be used if node role is non-router. Otherwise
 *          it will have no effect.
 */

#ifndef _SHARED_OFFLINE_H_
#define _SHARED_OFFLINE_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * \brief   List of return code
 */
typedef enum
{
    /** Operation is successful */
    SHARED_OFFLINE_RES_OK = 0,
    /** No more tasks available */
    SHARED_OFFLINE_RES_NO_MORE_ROOM = 1,
    /** Wrong id */
    SHARED_OFFLINE_RES_WRONG_ID = 2,
    /* Module is not initialized */
    SHARED_OFFLINE_RES_UNINITIALIZED = 3,
    /* Stack is already online */
    SHARED_OFFLINE_RES_ALREADY_ONLINE = 4,
    /* Role is wrong to enter offline */
    SHARED_OFFLINE_RES_WRONG_ROLE = 5
} shared_offline_res_e;

/**
 * \brief   Offline state
 */
typedef enum
{
    /** Operation is successful */
    SHARED_OFFLINE_STATUS_OFFLINE = 0,
    /** No more tasks available */
    SHARED_OFFLINE_STATUS_ONLINE = 1,
    /* Module is not initialized */
    SHARED_OFFLINE_STATUS_UNINITIALIZED = 3,
} shared_offline_status_e;

#define SHARED_OFFLINE_INFINITE_DELAY   (uint32_t)(-1)

/**
 * \brief   Offline callback called when the node enter offline state
 *          It means that all the module are ready to enter offline mode
 * \param   delay_s
 *          Delay the node will stay offline if none of the registered module
 *          ask to enter online state asynchronously
 */
typedef void (*offline_cb_f)(uint32_t delay_s);

/**
 * \brief   Online callback called when the node enter online state
 * \param   delay_from_deadline_s
 *          Delay in second per module relative to the initially requested time.
 *          If two registered modules M1 and M2 ask at the same time to stay
 *          offline for 3600 and 4000 seconds respectively, this variable will
 *          be set at 0 for M1 and 400 for M2. In fact online event happens 400s
 *          before initial M2 online request time.
 * \note    It is the responsibility of each registered module to ask for
 *          entering in offline state again by calling
 *          @ref Shared_Offline_enter_offline_state. The call can be made
 *          directly from this callback.
 * \note    For power consumption optimization, each module should try
 *          (if possible) to move forward their network activity
 *          (like sending a status) if their deadline is close,
 *          instead of asking to go offline for very short period.
 * \note    @ref on_online_event will be called at the latest when
 *          the deadline of the module to be online is reached. It can be
 *          called earlier if someone else has requested the online state
 *          earlier but it will never be called later.
 */
typedef void (*online_cb_f)(uint32_t delay_from_deadline_s);

typedef struct {
    offline_cb_f on_offline_event;
    online_cb_f on_online_event;
} offline_setting_conf_t;

/**
 * \brief   Initialize shared offline module
 *
 * \note    If this library is used in application, the nrls stack library
 *          MUST NOT be used outside of this module
 */
shared_offline_res_e Shared_Offline_init(void);

/**
 * \brief   Register to offline share lib
 *          It is required to take part of the offline arbitration.
 * \param   id_p
 *          Pointer to store the id assigned to the module
 * \param   cbs
 *          Callbacks for module event
 * \return  Return code of the operation
 */
shared_offline_res_e Shared_Offline_register(uint8_t * id_p,
                                             offline_setting_conf_t cbs);

/**
 * \brief   Unregister from offline share lib
 *          Once done, you will not be notified anymore for online/offline event
 * \param   id_p
 *          Pointer to store the id assigned to the module
 * \return  Return code of the operation
 */
shared_offline_res_e Shared_Offline_unregister(uint8_t id_p);

/**
 * \brief   Call to asynchronously enter in offline state
 * \param   id
 *          Id of the module asking for offline state
 * \param   delay_s
 *          Maximum delay in s to stay offline
 * \return  Return code of the operation
 */
shared_offline_res_e Shared_Offline_enter_offline_state(uint8_t id,
                                                        uint32_t delay_s);

/**
 * \brief   Call to asynchronously enter in online state
 * \param   id
 *          Id of the module asking for online state
 * \return  Return code of the operation
 */
shared_offline_res_e Shared_Offline_enter_online_state(uint8_t id);

/**
 * \brief   Get the current status
 * \param   elapsed_s_p
 *          Pointer to get the time in s already elapsed in the current state
 *          Can be NULL if caller is not interested by the info.
 * \param   remaining_s_p
 *          Pointer to get the time in s remaining in this state if no other
 *          event happens. Can be NULL if caller is not interested by the info.
 * \return  Return code of the operation
 */
shared_offline_status_e Shared_Offline_get_status(uint32_t * elapsed_s_p,
                                                  uint32_t * remaining_s_p);


#endif //_SHARED_OFFLINE_H_
