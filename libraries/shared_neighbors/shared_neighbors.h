/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#ifndef _SHARED_NEIGHBORS_H_
#define _SHARED_NEIGHBORS_H_

#include "api.h"

/**
 * @brief   Initialize the shared neighbors library.
 * @note    If shared state library is used in application, the
 *          @ref app_lib_state_set_on_beacon_cb_f "lib_state->setOnBeaconCb()",
 *          @ref app_lib_state_set_on_scan_nbors_with_type_cb_f "lib_state->setOnScanNborsCb()",
 *          functions offered by state library MUST NOT be used outside of this module.
 * @return  \ref APP_RES_OK.
 */
app_res_e Shared_Neighbors_init(void);

/**
 * @brief   Add a new callback about beacon received.
 * @param   cb_scanned_neighbor
 *          New callback to set
 * @param   cb_id
 *          id to be used with @ref Shared_State_removeBeaconCb.
 *          Set only if return code is APP_RES_OK.
 * @return  APP_RES_OK if ok. See \ref app_res_e for
 *          other result codes.
 */
app_res_e Shared_Neighbors_addOnBeaconCb(app_lib_state_on_beacon_cb_f cb_scanned_neighbor,
                                         uint16_t * cb_id);

/**
 * @brief   Remove a received beacon item from the list.
 *          Removed item fields are all set to 0.
 * @param   cb_id
 *          item to remove.
 */
app_res_e Shared_Neighbors_removeBeaconCb(uint16_t cb_id);

#endif //_SHARED_NEIGHBORS_H_
