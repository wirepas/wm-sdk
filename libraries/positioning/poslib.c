/** Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 *  See file LICENSE.txt for full license details.
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include "api.h"
#include "poslib.h"
#include "poslib_control.h"
#include "poslib_ble_beacon.h"

poslib_ret_e PosLib_setConfig(poslib_settings_t * settings)
{
    return PosLibCtrl_setConf(settings);
}

poslib_ret_e PosLib_getConfig(poslib_settings_t * settings)
{
    return PosLibCtrl_getConfig(settings);
}

poslib_ret_e PosLib_start(void)
{
    PosLibCtrl_init();
    return PosLibCtrl_start();
}

poslib_ret_e PosLib_stop(void)
{
    PosLibCtrl_stop(true);
    return POS_RET_OK;
}

poslib_ret_e PosLib_oneshot(void)
{
    return PosLibCtrl_oneshot();
}

poslib_ret_e PosLib_motion(poslib_motion_mode_e mode)
{
    return PosLibCtrl_motion(mode);
}

poslib_status_e PosLib_status(void)
{
    return PosLibCtrl_status();
}

poslib_ret_e PosLib_BLE(poslib_ble_mode_e pos_ble_mode)
{
    if (pos_ble_mode == POSLIB_BLE_START)
    {
        PosLibBle_set();
    }
    else if (pos_ble_mode == POSLIB_BLE_STOP)
    {
        PosLibBle_disable();
    }

    return POS_RET_OK;
}

void PosLib_event_setCb(poslib_events_e requested_event,
                        poslib_events_listen_info_f callback)
{
    PosLibCtrl_setEventCb(requested_event, callback);
}

void PosLib_event_clearCb(poslib_events_e requested_event)
{
    PosLibCtrl_clearEventCb(requested_event);
}
