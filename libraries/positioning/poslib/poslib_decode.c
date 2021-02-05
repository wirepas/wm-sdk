/**
 * @file       poslib_decode.c
 * @brief      contains the routines associates with the commands present in
 *             configuration payloads (app_config, uni/broad/multicast).
 *
 * @copyright  Wirepas Ltd.2020
 */
#define DEBUG_LOG_MODULE_NAME "POSLIB_DECODE"
#ifdef DEBUG_POSLIB_LOG_MAX_LEVEL
#define DEBUG_LOG_MAX_LEVEL DEBUG_POSLIB_LOG_MAX_LEVEL
#else
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#endif
#include "debug_log.h"
#include <string.h>
#include "api.h"
#include "poslib.h"
#include "poslib_ble_beacon.h"
#include "poslib_control.h"
#include "poslib_decode.h"
#include "settings.h"

static poslib_decode_info_t m_decode_info;

/** Configuration manual ble beacon values if stack offline value not used */
#define POSLIB_DECODE_BEACON_OFF             0
#define POSLIB_DECODE_BEACON_ON              65535

/**
 * @brief   Defines what happens on a measurement rate command.
 *
 * @param   command       The command
 * @param   app_settings  The application settings
 */
static void decode_measurement_rate(poslib_decode_msg_meas_rate_t * command,
                                    poslib_settings_t * settings);

/**
 * @brief   Defines what happens on a mode command.
 * @note    if the requested mode does not match the allowed ones for the node
 *          role then the mode change is ignored and the previous
 *           more remain in use
 * @param   command       The command
 * @param   app_settings  The application settings
 */
static void decode_msg_mode(poslib_decode_msg_mode_t * command,
                           poslib_settings_t * settings);

/**
 * @brief   Defines what happens on a otap command.
 *          If the node contains a sctratchpad that matches the target
 *          sequence number, the scratchpad will be marked for processing.
 *          A reboot is need and asked for.
 * @param   command       The command
 */
static void decode_msg_otap(poslib_decode_msg_otap_t * command);

/**
 * @brief   Defines what happens on offline waittime command.
 *
 * @param   command       The command
 * @param   settings      The application settings
 */
static void decode_msg_blebeacon_offline_waittime(
    poslib_decode_msg_blebeacon_offline_waittime_t * command,
    poslib_settings_t * settings);

/**
 * @brief   Defines what happens on a class command.
 *          A class command allows the node to transition between clases. For
 *          example, it allows changing all devices in class A to class C
 *          upon the reception of the app config.
 *
 * @param   command       The command
 * @param   settings  The application settings
 */
static void decode_msg_class(poslib_decode_msg_class_t * command,
                             poslib_settings_t * settings);

void PosLibDecode_init(void)
{
    memset(&m_decode_info, '\0', sizeof(m_decode_info));
}

/**
 * @brief   Checks if the node mode is anchor.
 * @param   node_mode  The mode to be tested
 */
static bool is_node_mode_anchor(poslib_mode_e node_mode)
{

bool ret = ((node_mode == POSLIB_MODE_AUTOSCAN_ANCHOR) ||
            (node_mode == POSLIB_MODE_OPPORTUNISTIC_ANCHOR));

return ret;
}

/**
 * @brief  Checks if the node mode is tag.
 * @param  node_mode  The mode to be tested
 */
static bool is_node_mode_tag(poslib_mode_e node_mode)
{
    bool ret = ((node_mode == POSLIB_MODE_NRLS_TAG)
                || (node_mode == POSLIB_MODE_AUTOSCAN_TAG));
    return ret;
}

/**
 * @brief  Checks that received class is valid.
 * @param  new_class
 *         class value received from app_config
 * @return true if payload is valid
 */
static bool check_class(uint8_t new_class)
{
    bool payload_valid = false;
    /** Read PosLib settings */
    poslib_settings_t * app_settings = Poslib_get_settings();

    switch (new_class)
    {
    /** Class definitions which can be used */
        case POSLIB_CLASS_DEFAULT:
        case POSLIB_CLASS_A:
        case POSLIB_CLASS_B:
        case POSLIB_CLASS_C:
        case POSLIB_CLASS_D:
        case POSLIB_CLASS_E:
        case POSLIB_CLASS_F:
            if(app_settings->node_class == new_class)
            {
                payload_valid = true;
            }
            break;
        default:
            break;;
    }

    return payload_valid;
}

bool PosLibDecode_config(const uint8_t * bytes, uint32_t num_bytes)
{
    bool decode_running = true;
    uint8_t i = 1;
    uint8_t field_nb = 0;
    uint8_t head = 0;
    uint8_t new_class = false;

    /** Read PosLib settings */
    poslib_settings_t * app_settings = Poslib_get_settings();
    LOG_BUFFER(LVL_DEBUG, bytes, num_bytes);

    new_class = bytes[0];
    /** Class definition needs to be correct */
    if (check_class(new_class) == true)
    {
        /** if the class matches its own, then it will process the payload fields
        otherwise the appconfig payload is ignored */
        decode_running = true;
        while(i < num_bytes && decode_running && field_nb < DECODE_MAX_FIELD)
        {
            /** decode bytes */
            switch (bytes[i])
            {
                case POSLIB_DECODE_MSG_MEASUREMENT_RATE:
                case POSLIB_DECODE_MSG_MODE:
                case POSLIB_DECODE_MSG_OTAP:
                case POSLIB_DECODE_MSG_BLEBEACON_OFFLINE_WAITTIME:
                case POSLIB_DECODE_MSG_CLASS:
                    head = i;
                    m_decode_info.field[field_nb].type = bytes[head];
                    m_decode_info.field[field_nb].length = bytes[head + 1];
                    m_decode_info.field[field_nb].ptr_payload = &bytes[head + 2];
                    /** update */
                    m_decode_info.num_fields = ++field_nb;
                    i += (bytes[head + 1] + 2); // length + header

                    LOG(LVL_DEBUG,"\"field_nb\":%d,\"head\":%d,\"type\":%d,\"length\":%d,\"i\":%d",
                            field_nb,
                            head,
                            bytes[head],
                            bytes[head + 1],
                            i);
                    break;

                default:
                    decode_running = false;
                    break;
            }
        }
        LOG(LVL_DEBUG, "\"msg\":\"validated app config payload\",\"class\":%d",
            app_settings->node_class);
        (void)app_settings;
    }
    else
    {
        LOG(LVL_DEBUG, "\"msg\":\"ignoring app config payload\",\"class\":%d",
            app_settings->node_class);
        (void)app_settings;
        return 0;
    }

    return m_decode_info.num_fields > 0;
}

void PosLibDecode_msg(void)
{
    uint8_t i = 0;
    bool running = true;
    poslib_settings_t * poslib_settings;
    poslib_decode_msg_meas_rate_t command_meas_rate;
    poslib_decode_msg_mode_t command_mode;
    poslib_decode_msg_otap_t command_otap;
    poslib_decode_msg_blebeacon_offline_waittime_t commnand_offline_waittime;
    poslib_decode_msg_class_t command_class;

    LOG(LVL_DEBUG, "\"number_of_fields\":%d", m_decode_info.num_fields);

    poslib_settings = Poslib_get_settings();

    for (i = 0; i < m_decode_info.num_fields && running; i++)
    {
        switch(m_decode_info.field[i].type)
        {
            //32bit or 16bit measurement rate accepted
            case POSLIB_DECODE_MSG_MEASUREMENT_RATE:
                if(m_decode_info.field[i].length ==
                    sizeof(poslib_decode_msg_meas_rate_t) ||
                    m_decode_info.field[i].length ==
                        LEGACY_DECODE_MSG_MEAS_RATE)
                {
                    memset(&command_meas_rate,  '\0',
                           sizeof(command_meas_rate));
                    memcpy(&command_meas_rate,
                           m_decode_info.field[i].ptr_payload,
                           m_decode_info.field[i].length);
                    decode_measurement_rate(&command_meas_rate, poslib_settings);
                }

                LOG(LVL_DEBUG, "\"case\":\"DECODE_MSG_MEASUREMENT_RATE(%d==%d)\"",
                    m_decode_info.field[i].length,
                    sizeof(poslib_decode_msg_meas_rate_t));
                break;

            case POSLIB_DECODE_MSG_MODE:
                if(m_decode_info.field[i].length ==
                    sizeof(poslib_decode_msg_mode_t))
                {
                    memcpy(&command_mode,
                           m_decode_info.field[i].ptr_payload,
                           sizeof(poslib_decode_msg_mode_t));
                    decode_msg_mode(&command_mode, poslib_settings);
                }

                LOG(LVL_DEBUG, "\"case\":\"DECODE_MSG_MODE(%d==%d)\"",
                  m_decode_info.field[i].length,
                  sizeof(poslib_decode_msg_mode_t));
                break;
            case POSLIB_DECODE_MSG_OTAP:
                if(m_decode_info.field[i].length == sizeof(poslib_decode_msg_otap_t))
                {
                    memcpy(&command_otap,
                           m_decode_info.field[i].ptr_payload,
                           sizeof(poslib_decode_msg_otap_t));
                    decode_msg_otap(&command_otap);
                }

                LOG(LVL_DEBUG, "\"case\":\"DECODE_MSG_OTAP(%d==%d)\"",
                  m_decode_info.field[i].length,
                  sizeof(poslib_decode_msg_otap_t));
                break;
            case POSLIB_DECODE_MSG_BLEBEACON_OFFLINE_WAITTIME:
                if(m_decode_info.field[i].length ==
                    sizeof(poslib_decode_msg_blebeacon_offline_waittime_t))
                {
                    memcpy(&commnand_offline_waittime,
                           m_decode_info.field[i].ptr_payload,
                           sizeof(poslib_decode_msg_blebeacon_offline_waittime_t));
                    decode_msg_blebeacon_offline_waittime(
                        &commnand_offline_waittime,
                        poslib_settings);
                 }

                LOG(LVL_DEBUG, "\"case\":\"DECODE_MSG_OFFLINE_WAITTIME(%d==%d)\"",
                  m_decode_info.field[i].length,
                  sizeof(poslib_decode_msg_blebeacon_offline_waittime_t));
                break;
            case POSLIB_DECODE_MSG_CLASS:
                if(m_decode_info.field[i].length ==
                    sizeof(poslib_decode_msg_class_t))
                {
                    memcpy(&command_class,
                           m_decode_info.field[i].ptr_payload,
                           sizeof(poslib_decode_msg_class_t));
                    decode_msg_class(&command_class, poslib_settings);
                }

                LOG(LVL_DEBUG, "\"case\":\"POSLIB_DECODE_MSG_CLASS(%d==%d)\"",
                  m_decode_info.field[i].length,
                  sizeof(poslib_decode_msg_class_t));
                break;
            default:
                LOG(LVL_DEBUG, "\"case\":\"UNKNOWN_MSG(%d==%d)\"",
                  m_decode_info.field[i].length,
                  sizeof(poslib_decode_msg_otap_t));
                break;
        }
    }

    PosLibCtrl_updateSettings(poslib_settings);
}

static void decode_measurement_rate(poslib_decode_msg_meas_rate_t * command,
                                    poslib_settings_t * settings)
{
    LOG(LVL_DEBUG, "\"interval\":%d", command->interval);

    if (command->interval >= TX_INTERVAL_MIN &&
        command->interval <= TX_INTERVAL_MAX)
    {
        settings->update_period_static_s = command->interval;
    }
    else
    {
        LOG(LVL_ERROR, "\"rejected interval\":%d", command->interval);
    }
}

static void decode_msg_mode(poslib_decode_msg_mode_t * command,
                            poslib_settings_t * settings)
{
    app_lib_settings_role_t role;
    uint8_t base_role;
    poslib_mode_e node_mode = command->value;

    lib_settings->getNodeRole(&role);
    base_role = app_lib_settings_get_base_role(role);

    switch(base_role)
    {
        case APP_LIB_SETTINGS_ROLE_SINK:
        case APP_LIB_SETTINGS_ROLE_HEADNODE:
        //only node modes for anchors are accepted
            if(is_node_mode_anchor(command->value))
            {
                settings->node_mode = node_mode;
                LOG(LVL_DEBUG, "\"settings->node_mode for router\":%d",
                    settings->node_mode);
            }
        break;
        case APP_LIB_SETTINGS_ROLE_SUBNODE:
        //only node modes for tags are accepted
            if(is_node_mode_tag(command->value))
            {
                settings->node_mode = node_mode;
                LOG(LVL_DEBUG, "\"settings->node_mode for subnode \":%d",
                    settings->node_mode);
            }
        break;
        default:
            LOG(LVL_ERROR, "\"ERROR PosLib node role is not set \":%d",
                base_role);
        break;

    }
}

static void decode_msg_otap(poslib_decode_msg_otap_t * command)
{
    LOG(LVL_INFO, "\"target_sequence\":%d",
      command->target_sequence);

    bool is_valid = false;
    bool is_processed = false;

    app_lib_otap_seq_t scratchpad_sequence = 0;

    is_valid = lib_otap->isValid();
    is_processed = lib_otap->isProcessed();

    // perform operation
    if (is_valid && !is_processed)
    {
        scratchpad_sequence = lib_otap->getSeq();
        LOG(LVL_INFO, "\"msg\":\"OTAP(%d=%d)\"", scratchpad_sequence,
          command->target_sequence);

        if (command->target_sequence == scratchpad_sequence)
        {
            lib_otap->setToBeProcessed();
            LOG(LVL_WARNING, "\"msg\":\"OTAP:RESQUESTING_REBOOT\"");

            // Start the reboot
            lib_state->stopStack();
        }
    }
    else
    {
        LOG(LVL_INFO, "\"msg\":\"OTAP:NOTHING_TO_DO\"");
    }
}

void decode_msg_class(poslib_decode_msg_class_t * command,
                      poslib_settings_t * settings)
{
    LOG(LVL_INFO, "\"class_id\":%d", command->value);

    switch(command->value)
    {
        case POSLIB_CLASS_DEFAULT:
        case POSLIB_CLASS_A:
        case POSLIB_CLASS_B:
        case POSLIB_CLASS_C:
        case POSLIB_CLASS_D:
        case POSLIB_CLASS_E:
        case POSLIB_CLASS_F:
            settings->node_class = command->value;
            break;
        default:
            LOG(LVL_WARNING, "\"msg\":\"received unknown class identifier\"");
            break;
    }
}

static void decode_msg_blebeacon_offline_waittime(
    poslib_decode_msg_blebeacon_offline_waittime_t * command,
    poslib_settings_t * settings)
{
    uint16_t waittime = 0;

    LOG(LVL_DEBUG, "\"waittime\":%d", command->waittime);

    waittime = command->waittime;
    settings->poslib_ble_settings.update_period_bleon_offline_s =
        command->waittime;

    switch(waittime)
    {
        case POSLIB_DECODE_BEACON_OFF:
        {
            PosLibBle_set(POSLIB_BLE_STOP);
            settings->poslib_ble_settings.ble_mode = POSLIB_BLE_STOP;
        }
        break;
        case POSLIB_DECODE_BEACON_ON:
        {
            PosLibBle_set(POSLIB_BLE_START);
            settings->poslib_ble_settings.ble_mode = POSLIB_BLE_START;
        }
        break;
        default:
        break;
    }
}
