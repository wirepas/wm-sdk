/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef _SHARED_SHUTDOWN_H_
#define _SHARED_SHUTDOWN_H_

#include "api.h"

/**
 * @brief   Initialize the shared Shutdown library.
 * @note    If shared shutdown library is used in application, the
 *          @ref app_lib_system_set_shutdown_cb_f "lib_system->setShutdownCb()",
 *          function offered by system library MUST NOT be used outside of this module.
 * @return  \ref APP_RES_OK.
 */
app_res_e Shared_Shutdown_init(void);

/**
 * @brief   Add a new callback about shutdown cb.
 * @param   callback
 *          New callback
 * @param   cb_id
 *          Id to be used with @ref Shared_Shutdown_removeShutDownCb.
 *          Set only if return code is APP_RES_OK.
 * @return  APP_RES_OK if ok. See \ref app_res_e for
 *          other result codes.
 */
app_res_e Shared_Shutdown_addShutDownCb(app_lib_system_shutdown_cb_f callback,
                                        uint16_t * cb_id);

/**
 * @brief   Remove a shutdown cb item from the list.
 *          Removed item fields are all set to 0.
 * @param   cb_id
 *          item to remove.
 */
app_res_e Shared_Shutdown_removeShutDownCb(uint16_t cb_id);

#endif //_SHARED_SHUTDOWN_H_
