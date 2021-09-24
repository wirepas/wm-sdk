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
#include "poslib_da.h"
#include "poslib_mbcn.h"

poslib_ret_e PosLib_setConfig(poslib_settings_t * settings)
{
    return PosLibCtrl_setConfig(settings);
}

poslib_ret_e PosLib_getConfig(poslib_settings_t * settings)
{
    return PosLibCtrl_getConfig(settings);
}

poslib_ret_e PosLib_startPeriodic(void)
{
    return PosLibCtrl_startPeriodic();
}

poslib_ret_e PosLib_stopPeriodic(void)
{
   return  PosLibCtrl_stopPeriodic();
}

poslib_ret_e PosLib_startOneshot(void)
{
    return PosLibCtrl_startOneshot();
}

poslib_ret_e PosLib_motion(poslib_motion_mode_e mode)
{
    return PosLibCtrl_motion(mode);
}

poslib_status_e PosLib_status(void)
{
    return PosLibCtrl_status();
}

poslib_ret_e PosLib_eventRegister(poslib_events_e event,
                            poslib_events_listen_info_f cb, 
                            uint8_t * id)
{
    return PosLibEvent_register(event, cb, id);
}

void PosLib_eventDeregister(uint8_t id)
{
    PosLibEvent_deregister(id);
}

app_lib_data_send_res_e PosLib_sendData(app_lib_data_to_send_t * data,
                                        app_lib_data_data_sent_cb_f sent_cb)
{
    return PosLibDa_sendData(data, sent_cb);
}

bool PosLib_decodeMbcn(uint8_t * buf, uint8_t length, poslib_mbcn_data_t * mbcn)
{
    return PosLibMbcn_decode(buf, length, mbcn);
}