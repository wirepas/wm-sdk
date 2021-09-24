/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file local_provisioning.h
 *
 * Local provisioning library.
 */

#ifndef _LOCAL_PROVISIONING_H_
#define _LOCAL_PROVISIONING_H_

typedef enum {
    LOCAL_PROVISIONING_RES_SUCCESS = 0, /**< Operation is a success. */
    LOCAL_PROVISIONING_RES_UNINTIALLIZED = 1,
    LOCAL_PROVISIONING_RES_WRONG_STATE = 2,
    LOCAL_PROVISIONING_RES_INTERNAL_ERROR = 3,
    LOCAL_PROVISIONING_RES_UNPROVISIONED = 4,
    LOCAL_PROVISIONING_RES_ALREADY_PROVISIONED = 5,
} Local_provisioning_res_e;


/**
 * \brief   Selects which joining beacon to connect to at the end of a scan.
 * \note    Joining network selection is up to the application. Any algorithm
 *          can be implemented for example by looking at the content of the
 *          beacon payload, or the network (address and channel) the node
 *          sending the beacons is connected to.
 * \param   beacons
 *          A buffer of joining beacons.
 * \return  The selected beacon.
 */
typedef const app_lib_joining_received_beacon_t *
                    (*local_provisioning_joining_beacon_selection_f)
                            (const app_lib_joining_received_beacon_t * beacons);

/**
 * \brief   Callback to be notified when proxy is enabled
 *          It is an information for app. It can be used to blink a led for example.
 * \param   enabled
 *          True if joining beacons are currently sent, false otherwise
 */
typedef void (*local_provisioning_proxy_enabled_cb)(bool enabled);

/**
 * \brief   Initialize the Local provisioning library
 * \param   psk
 *          Pre Shared Key used between joining node and neighbor node.
 *          If set to NULL, secure method is still used but with a default
 *          key.
 * \return  Result code of the operation
 * \note    If psk is not set, even if data exchange is encrypted, anyone could decrypt it
 *          thanks to the default key available on GitHub. It is highly recommended to define
 *          a personal secret key that will be the same in all the nodes that can interact.
 */
Local_provisioning_res_e Local_provisioning_init(const uint8_t psk[32],
                                                 local_provisioning_proxy_enabled_cb on_proxy_enabled_cb);

/**
 * \brief   Is the node provisioned
 *          A node is considered provisioned if it has a valid config with network security
 *          keys set.
 * \note    Once provisioned, a node will automatically wait on app config specific order to
 *          start sending its joining beacons
 * \return  True if node is provisioned, false otherwise.
 */
bool Local_provisioning_is_provisioned();

/**
 * \brief   Start a joining session
 *          Node will start a joining request.
 * \param   cb
 *          Callback to be called after a beacon scan to select the node to do the joining.
 *          If set to NULL, the beacon with the highest RSSI is selected
 * \return  Result code of the operation
 */
Local_provisioning_res_e Local_provisioning_start_joining(local_provisioning_joining_beacon_selection_f cb);

/**
 * \brief   Reset all node settings
 * \note    This call will genrate a reboot of node and will never return
 */
void Local_provisioning_reset_node();

#endif //_LOCAL_PROVISIONING_H_