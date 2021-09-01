/**
* @file       poslib_control.h
* @brief      Header for poslib_control.c
* @copyright  Wirepas Ltd 2021
*/

#ifndef _POSLIB_CONTROL_H_
#define _POSLIB_CONTROL_H_

#include "poslib_event.h"

/**
 * @brief   PosLib control internal state definitions.
 */
typedef enum
{
    POSLIB_STATE_STOPPED = 0,
    POSLIB_STATE_IDLE = 1,
    POSLIB_STATE_UPDATE = 2
} poslib_state_e;

/**
 * @brief   Sets new configration to PosLib.
 * @param   settings type of \ref poslib_settings_t with settings
 * @return  See \ref poslib_ret_e
 */
poslib_ret_e PosLibCtrl_setConfig(poslib_settings_t * settings);

/**
 * @brief   Returns the current PosLib configuration
 * @param   settings type of \ref poslib_settings_t where settings will be delivered
 * @return  See \ref poslib_ret_e
 */
poslib_ret_e PosLibCtrl_getConfig(poslib_settings_t * settings);

/**
 * @brief   Starts periodic positioning update
 * @return  See \ref poslib_ret_e
 */
poslib_ret_e PosLibCtrl_startPeriodic(void);

/**
 * @brief   Stops periodic positioning update
 *     
 */
poslib_ret_e PosLibCtrl_stopPeriodic(void);

/**
 * @brief   Triggers a single position update imediatelly.
 * @note    If periodic update is active it will continue after oneshot completed
 *          If an update is already in progress the call will be ignored
 * @return  See \ref poslib_ret_e
 */
poslib_ret_e PosLibCtrl_startOneshot(void);

/**
 * @brief  Adjust the update rate according to motion state (static/dynamic)
 * @param   mode type of \ref poslib_motion_mode_e
 * @return  See \ref poslib_ret_e
 */
poslib_ret_e PosLibCtrl_motion(poslib_motion_mode_e mode);

/**
 * @brief   Returns the current status of PosLib.
 * @return  See \ref poslib_status_e
 */
poslib_status_e PosLibCtrl_status(void);

/**
 * @brief   Process the internal event
 * @param   event type of \ref poslib_internal_event_t
 * @return   void
 */
void PosLibCtrl_processEvent(poslib_internal_event_t * event);


/**
 * @brief   Returns the current app config payload
 * @param[out] cfg pointer to the app config array
 * @param[out] len the length of the app config, 0 no configuration valid    
 * @return  void
 */
void PosLibCtrl_getAppConfig(uint8_t ** cfg, uint8_t * len);
void PosLibCtrl_setAppConfig(const uint8_t * cfg, uint8_t len);

#endif
