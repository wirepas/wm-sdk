/**
* @file         poslib_event.h
* @brief        Header file for the poslib_event.c
* @copyright    Wirepas Ltd. 2021
*/

#ifndef _POSLIB_EVENT_H_
#define _POSLIB_EVENT_H_

#include "sl_list.h"

#define MAX_INTERNAL_EVENTS 16

typedef enum {
    /**< No event */
    POSLIB_CTRL_EVENT_NONE = 0,
    /**< Scan started */
    POSLIB_CTRL_EVENT_SCAN_STARTED = 1,
    /**< Scan ended */
    POSLIB_CTRL_EVENT_SCAN_END = 2,
    /**< Stack entered in online mode */
    POSLIB_CTRL_EVENT_ONLINE = 3,
    /**< Stack entered in offline mode */
    POSLIB_CTRL_EVENT_OFFLINE = 4,
    /**< AppCfg received*/
    POSLIB_CTRL_EVENT_APPCFG = 5,
    /**< Timeout occured while waiting for an event */
    POSLIB_CTRL_EVENT_TIMEOUT = 6,
    /**< Measurement data sent */
    POSLIB_CTRL_EVENT_DATA_SENT = 7,
    /**< Oneshot update requested */
    POSLIB_CTRL_EVENT_ONESHOT = 8,
    /**< Configuration changed */
    POSLIB_CTRL_EVENT_CONFIG_CHANGE = 9,
    /**< Stack route changed */
    POSLIB_CTRL_EVENT_ROUTE_CHANGE = 10,
    /**< Update started */
    POSLIB_CTRL_EVENT_UPDATE_START = 11,
    /**< Update ended */
    POSLIB_CTRL_EVENT_UPDATE_END = 12,
    /**< Node is outside WM coverage */
    POSLIB_CTRL_EVENT_OUTSIDE_WM = 13,
    /**< Node is under WM coverage */
    POSLIB_CTRL_EVENT_UNDER_WM = 14,
    /**< Motion changed */
    POSLIB_CTRL_EVENT_MOTION = 15,
    /**< BLE start */
    POSLIB_CTRL_EVENT_BLE_START = 16,
    /**< BLE stop */
    POSLIB_CTRL_EVENT_BLE_STOP = 17,
    /**< LED on event */
    POSLIB_CTRL_EVENT_LED_ON = 18,
    /**< LED off event */
    POSLIB_CTRL_EVENT_LED_OFF = 19,
} poslib_internal_event_type_e;

typedef struct {
    sl_list_t list;
    poslib_internal_event_type_e type;
} poslib_internal_event_t;

bool PosLibEvent_add(poslib_internal_event_type_e type);

/**
 * @brief   Register an PosLib event subscriber for 
 * @param   event Events of interest (type of poslib_events_e)
 * @param   cb Callback to be called (type poslib_events_listen_info_f)
 * @param   id Returned subscriber ID
 * @return  POS_RET_OK is succesful / POS_RET_EVENT_ERROR if failed
 */
poslib_ret_e PosLibEvent_register(poslib_events_e event,
                            poslib_events_listen_info_f cb, 
                            uint8_t * id);

/**
 * @brief   Deregister the PosLib event subscriber id.
 * @param   id the subscriber ID
 * @return  void 
 */
void PosLibEvent_deregister(uint8_t id);

#endif
