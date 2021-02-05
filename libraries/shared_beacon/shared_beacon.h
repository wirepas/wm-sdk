/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef _SHARED_BEACON_H_
#define _SHARED_BEACON_H_

#include "api.h"

/**
 * \brief   List of return code
 */
typedef enum
{
    /** Operation is successful */
    SHARED_BEACON_RES_OK = 0,
    /** Initialization is not done */
    SHARED_BEACON_INIT_NOT_DONE = 1,
    /** No free index in shared_beacon or in lib_beacon_tx library */
    SHARED_BEACON_INDEX_NOT_AVAILABLE = 2,
    /** No active index in in lib_beacon_tx library */
    SHARED_BEACON_INDEX_NOT_ACTIVE = 3,
    /** Given parameter(s) invalid */
    SHARED_BEACON_INVALID_PARAM = 4,
} shared_beacon_res_e;

/**
 * @brief   Initialize the Shared_Beacon library.
 * @note    If Shared_Beacon library is used in application, functions offered
 *          by lib_beacon_tx library MUST NOT be used outside of this module.
 * @return  \ref shared_beacon_res_e.
 */
shared_beacon_res_e Shared_Beacon_init(void);

/**
 * @brief   Starts sending of beacon content defined in the parameters.
 *          There is possibility to start multiple beacons after another beacons
 *          started. The limitation is that there cannot be set individual beacon
 *          sending interval for each beacons. The smallest interval defined
 *          for beacons is selected as common interval for all beacons.
 * @param   interval_ms
 *          beacon sending interval in ms
 * @param   power
 *          rf power level in dBm
 * @param   channel_mask
 *          channels to be used for defined beacon
 * @param   content
 *          data to be sent out
 * @param   length
 *          length of content to be sent out
 * @param   shared_beacon_index
 *          returns index which is used when Shared_Beacon_stopBeacon used,
 *          used indexes: (0 .. \ref APP_LIB_BEACONTX_MAX_INDEX)
 * @return  \ref shared_beacon_res_e.
 */
shared_beacon_res_e Shared_Beacon_startBeacon(uint16_t interval_ms,
                                              int8_t * power,
                                              app_lib_beacon_tx_channels_mask_e
                                              channels_mask,
                                              const uint8_t * content,
                                              uint8_t length,
                                              uint8_t * shared_beacon_index);

/**
 * @brief   Stops beacon with the given index. Shared beacon used interval is
 *          checked and selected smaller interval from active beacons after
 *          shared_beacon_index is set as not active.
 * @param   shared_beacon_index
 *          The index received in Shared_Beacon_startBeacon
 * @return  \ref shared_beacon_res_e.
 */
shared_beacon_res_e Shared_Beacon_stopBeacon(uint8_t shared_beacon_index);

#endif //_SHARED_BEACON_H_
