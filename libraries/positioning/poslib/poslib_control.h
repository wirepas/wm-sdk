/**
* @file       poslib_control.h
* @brief      Header for poslib_control.c
* @copyright  Wirepas Ltd. 2020
*/

#ifndef _POSLIB_CONTROL_H_
#define _POSLIB_CONTROL_H_

/**
 * @brief Endpoints for measurements
 */
#define POS_DESTINATION_ENDPOINT 238
#define POS_SOURCE_ENDPOINT      238

/**
 * @brief   PosLib control internal state definitions.
 */
typedef enum
{
    POSLIB_STATE_STOPPED = 1,
    POSLIB_STATE_PERFORM_SCAN = 2,
    POSLIB_STATE_OPPORTUNISTIC = 3,
    POSLIB_STATE_IDLE = 4,
    POSLIB_STATE_TOSLEEP = 5,
    POSLIB_STATE_ONESHOT_SCAN = 6,
    POSLIB_STATE_NRLS_START = 7,
    // exceptions
    POSLIB_STATE_REBOOT = 0xFF,
} positioning_state_e;

/**
 * @brief   Sets PosLib configuration.
 * @param   settings type of poslib_settings_t
 */
void PosLibCtrl_updateSettings(poslib_settings_t * settings);

/**
 * @brief   Returns PosLib configuration set for PosLib internal use.
 * @return  See \ref poslib_settings_t
 */
poslib_settings_t * Poslib_get_settings(void);

/**
 * @brief   Increases the message sequence id and returns the value.
 * @return  id type of uint16_t
 */
uint16_t PosLibCtrl_incrementSeqId(void);

/**
 * @brief   Returns the message sequence id value.
 * @return  id type of uint16_t
 */
uint16_t PosLibCtrl_getSeqId(void);

/**
 * @brief   Called from measurement task when network scan end.
 *          Controls PosLib next functionalities  based on
 *          used PosLib configuration.
 */
void PosLibCtrl_endScan(void);

/**
 * @brief   Data sent ack is enabled when in NRLS mode. Function is called
 *          when data ack is received and NRLS PosLib controls based on that.
 */
void PosLibCtrl_receiveAck(void);

/**
 * @brief   Returns active node_mode.
 * @return  See \ref poslib_mode_e
 */
poslib_mode_e PosLibCtrl_getMode(void);

/**
 * @brief   Called from PosLib functions to generate event.
 * @param   msg type of poslib_events_info_t
 */
void PosLibCtrl_generateEvent(poslib_events_info_t * msg);

/**
 * @brief   Returns the status of oneshot PosLib request.
 * @return  true if oneshot is requested from PosLib api.
 */
bool posLibCtrl_getOneshotStatus(void);

/**
 * @brief   Initializes PosLib to default and initializes shared libraries
 *          to PosLib use.
 * @note    Following shared libraries needs to be initialized before use
 *          of PosLib_init:
 *          Shared_Data_init();
 *          Shared_Appconfig_init();
 *          Shared_Neighbors_init();
 *          Shared_Shutdown_init();
 */
void PosLibCtrl_init(void);

/**
 * @brief   Sets new configration to PosLib.
 * @param   settings type of poslib_settings_t
 * @return  See \ref poslib_ret_e
 */
poslib_ret_e PosLibCtrl_setConf(poslib_settings_t * settings);

/**
 * @brief   Returns PosLib configuration set for PosLib external interface.
 * @param   settings type of poslib_settings_t
 * @return  See \ref poslib_ret_e
 */
poslib_ret_e PosLibCtrl_getConfig(poslib_settings_t * settings);

/**
 * @brief   Starts PosLib activity defined in configuration.
 * @return  See \ref poslib_ret_e
 */
poslib_ret_e PosLibCtrl_start(void);

/**
 * @brief   Stops task run_poscontrol_state_machine activity.
 * @param   callbacks_removed
 *          true if shared callbacks are removed as well
 *          false if shared callbacks are not removed
 */
void PosLibCtrl_stop(bool callbacks_removed);

/**
 * @brief   Triggers one position update. If PosLib is started the scheduled
 *          operation will continue after the one-shot update with the
 *          respective schedule. Of is in stop mode then will stay in stop mode
 *          after the one-shot update.
 * @note    If called during NRLS sleep, sleep is stopped which triggers one
 *          position update. NRLS period continues with the respective schedule.
 * @return  See \ref poslib_ret_e
 */
poslib_ret_e PosLibCtrl_oneshot(void);

/**
 * @brief   Is used to indicate that the tag is static or dynamic. The update
 *          rate used will be adjusted according to the state:  when moving from
 *          static to dynamic:  an update will be triggered immediately if the
 *          last update is older than (current time - dynamic rate),
 *          otherwise the next update is re-scheduled as
 *          (last update + dynamic rate).
 * @param   mode type of poslib_motion_mode_e
 * @return  See \ref poslib_ret_e
 */
poslib_ret_e PosLibCtrl_motion(poslib_motion_mode_e mode);

/**
 * @brief   Returns the current status of PosLib.
 * @return  See \ref poslib_status_e
 */
poslib_status_e PosLibCtrl_status(void);

/**
 * @brief   Sets requested event or event group and callback function.
 * @param   requested_event type of poslib_events_e
 * @param   callback type of poslib_events_listen_info_f
 */
void PosLibCtrl_setEventCb(poslib_events_e requested_event,
                           poslib_events_listen_info_f callback);

/**
 * @brief   Clears event or event group set via PosLib_control_setCb.
 * @param   requested_event type of poslib_events_e
 */
void PosLibCtrl_clearEventCb(poslib_events_e requested_event);

#endif
