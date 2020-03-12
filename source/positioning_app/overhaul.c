/**
    @file overhaul.c
    @brief      contains the routines associates with the commands present in
             configuration payloads (app_config, uni/broad/multicast).

    @copyright  Wirepas Oy 2019
*/
#include "api.h"
#include "app_settings.h"
#include "overhaul.h"

static overhaul_info_t m_overhaul_info;



/**
    @brief      : cleans the overhaul table.
*/
void Overhaul_init(void)
{
    memset(&m_overhaul_info, '\0', sizeof(m_overhaul_info));
}


/**
    @brief      decodes an incoming configuration payload.

    @param[in]  bytes      The configuration bytes
    @param[in]  num_bytes  The number bytes
    @param[in]  seq        The configuration sequence number

    @return     True if it was able to decode at least one message.
*/
bool Overhaul_decode_config(const uint8_t* bytes,
                            uint32_t num_bytes,
                            uint8_t seq)
{

    bool payload_valid = false;
    bool decode_running = true;
    uint8_t i = 0;
    uint8_t field_nb = 0;
    uint8_t head = 0;

    positioning_settings_t* app_settings = Pos_get_settings();

    m_overhaul_info.num_fields = 0;
    // loops until it finds a message for its class
    for(i = 0; i < num_bytes && decode_running; i++)
    {

        switch (bytes[i])
        {
            case POS_APP_CLASS_DEFAULT:
            case POS_APP_CLASS_A:
            case POS_APP_CLASS_B:
            case POS_APP_CLASS_C:
            case POS_APP_CLASS_D:
            case POS_APP_CLASS_E:
            case POS_APP_CLASS_F:
                if(app_settings->node_class == bytes[i])
                {
                    payload_valid = true;
                    decode_running = false;
                }
                continue;
            default:
                continue;
        }
    }

    // if the class matches its own, then it will process the payload fields
    // otherwise the appconfig payload is ignored
    decode_running = true;
    while(i < num_bytes
            && payload_valid
            && decode_running
            && field_nb < OVERHAUL_MAX_FIELD)
    {
        // decode
        switch (bytes[i])
        {

            case OVERHAUL_MSG_MEASUREMENT_RATE:
            case OVERHAUL_MSG_MODE:
            case OVERHAUL_MSG_OTAP:
            case OVERHAUL_MSG_BLEBEACON_OFFLINE_WAITTIME:
            case OVERHAUL_MSG_CLASS:
                head = i;
                m_overhaul_info.field[field_nb].type = bytes[head];
                m_overhaul_info.field[field_nb].length = bytes[head + 1];
                m_overhaul_info.field[field_nb].ptr_payload = &bytes[head + 2];

                // update
                m_overhaul_info.num_fields = ++field_nb;
                i += (bytes[head + 1] + 2); // length + header

                break;

            default:
                decode_running = false;
                break;
        }
    }

    return m_overhaul_info.num_fields > 0;
}


/**
    @brief      Acts on the commands provided in the configuration payload.

    @return     The next desired state according to the decoded commands.
*/
scheduler_state_e Overhaul_msg(void)
{
    uint8_t i = 0;
    bool running = true;
    bool reboot = false;

    positioning_settings_t* app_settings = Pos_get_settings();
    scheduler_state_e next_state = Scheduler_get_state();

    overhaul_msg_meas_rate_t command_meas_rate;
    overhaul_msg_mode_t command_mode;
    overhaul_msg_otap_t command_otap;
    overhaul_msg_blebeacon_offline_waittime_t commnand_offline_waittime;
    overhaul_msg_class_t command_class;

    for (i = 0; i < m_overhaul_info.num_fields && running; i++)
    {
        switch(m_overhaul_info.field[i].type)
        {

            case OVERHAUL_MSG_MEASUREMENT_RATE:  //32bit or 16bit measurement rate accepted
                if(m_overhaul_info.field[i].length == sizeof(overhaul_msg_meas_rate_t) ||
                   m_overhaul_info.field[i].length == LEGACY_OVERHAUL_MSG_MEAS_RATE)
                {

                    memset(&command_meas_rate,  '\0', sizeof(command_meas_rate));
                    memcpy(&command_meas_rate,
                           m_overhaul_info.field[i].ptr_payload,
                           m_overhaul_info.field[i].length);
                    reboot |= Overhaul_msg_measurement_rate(&command_meas_rate,
                                                            app_settings);

                }
                break;

            case OVERHAUL_MSG_MODE:
                if(m_overhaul_info.field[i].length == sizeof(overhaul_msg_mode_t))
                {
                    memcpy(&command_mode,
                           m_overhaul_info.field[i].ptr_payload,
                           sizeof(overhaul_msg_mode_t));

                    reboot |= Overhaul_msg_mode(&command_mode,
                                                app_settings);
                }
                break;

            case OVERHAUL_MSG_OTAP:
                if(m_overhaul_info.field[i].length == sizeof(overhaul_msg_otap_t))
                {
                    memcpy(&command_otap,
                           m_overhaul_info.field[i].ptr_payload,
                           sizeof(overhaul_msg_otap_t));

                    reboot |= Overhaul_msg_otap(&command_otap, app_settings);
                }
                break;

            case OVERHAUL_MSG_BLEBEACON_OFFLINE_WAITTIME:
                if(m_overhaul_info.field[i].length == sizeof(overhaul_msg_blebeacon_offline_waittime_t))
                {
                    memcpy(&commnand_offline_waittime,
                           m_overhaul_info.field[i].ptr_payload,
                           sizeof(overhaul_msg_blebeacon_offline_waittime_t));
                    reboot |= Overhaul_msg_blebeacon_offline_waittime(&commnand_offline_waittime,
                                                                      app_settings);

                                }
                break;

            case OVERHAUL_MSG_CLASS:
                if(m_overhaul_info.field[i].length == sizeof(overhaul_msg_class_t))
                {
                    memcpy(&command_class,
                           m_overhaul_info.field[i].ptr_payload,
                           sizeof(overhaul_msg_class_t));
                    reboot |= Overhaul_msg_class(&command_class, app_settings);
                }
                break;


            default:
                break;
        }
    }

    if(reboot)
    {
        next_state = POS_APP_STATE_REBOOT;
    }

    return next_state;
}


/**
    @brief      Defines what happens on a measurement rate command.

    @param      command       The command
    @param      app_settings  The application settings

    @return     True if a reboot should happen.
*/
bool Overhaul_msg_measurement_rate(overhaul_msg_meas_rate_t* command,
                                   positioning_settings_t* app_settings)
{
    bool request_reboot = false;

    if (command->interval >= TX_INTERVAL_MIN &&
        command->interval <= TX_INTERVAL_MAX)
    {
        app_settings->scan_period = command->interval;
    }

    return request_reboot;
}

/**
    @brief      Defines what happens on a mode command.
                Note: if the requested mode does not match
                the allowed ones for the node role then the mode
                change is ignored and the previous more remain in use

    @param      command       The command
    @param      app_settings  The application settings

    @return     True if a reboot should happen.
*/
bool Overhaul_msg_mode(overhaul_msg_mode_t* command,
                       positioning_settings_t* app_settings)
{
    bool request_reboot = false;


    switch(app_settings->node_role)
    {
        case APP_LIB_SETTINGS_ROLE_SINK:
        case APP_LIB_SETTINGS_ROLE_HEADNODE:
        //only node modes for anchors are accepted
            if(is_node_mode_anchor(command->value))
            {
                app_settings->node_mode = command->value;
            }
        break;

        case APP_LIB_SETTINGS_ROLE_SUBNODE:
        //only node modes for tags are accepted
            if(is_node_mode_tag(command->value))
            {
                app_settings->node_mode = command->value;
            }
        break;
    }

    return request_reboot;
}


/**
    @brief      Defines what happens on a otap command.

            If the node contains a sctratchpad that matches the target
            sequence number, the scratchpad will be marked for processing.

            A reboot is need and asked for.

    @param      command       The command
    @param      app_settings  The application settings

    @return     True if a reboot should happen.
*/
bool Overhaul_msg_otap(overhaul_msg_otap_t* command,
                       positioning_settings_t* app_settings)
{

    bool is_valid = false;
    bool is_processed = false;
    bool request_reboot = false;

    app_lib_otap_seq_t scratchpad_sequence = 0;

    is_valid = lib_otap->isValid();
    is_processed = lib_otap->isProcessed();

    // perform operation
    if (is_valid && !is_processed)
    {
        scratchpad_sequence = lib_otap->getSeq();
        if (command->target_sequence == scratchpad_sequence)
        {
            lib_otap->setToBeProcessed();
            request_reboot = true;
        }
    }
    else
    {
    }

    return request_reboot;
}

/**
    @brief      Defines what happens on offline waittime command.

    @param      command       The command
    @param      app_settings  The application settings

    @return     True if a reboot should happen.
*/
bool Overhaul_msg_blebeacon_offline_waittime(overhaul_msg_blebeacon_offline_waittime_t* command,
                                             positioning_settings_t* app_settings)
{
    bool request_reboot = false;

    if (command->waittime == 0)
    {
        // Put Ble Beacon for off
        app_settings->ble_beacon_setup = BLE_BEACON_OFF;
        Ble_Beacon_init(BLE_BEACON_OFF);
    }
    else if (command->waittime == 0xFFFF)
    {
        // Set Ble Beacon for On
        app_settings->ble_beacon_setup = BLE_BEACON_ON;
        Ble_Beacon_init(BLE_BEACON_ON);
        Ble_Beacon_set_configuration();
    }
    else
    {
        // Set Ble Beacon for offline and take the waittime in use
        app_settings->ble_beacon_offline_waittime = command->waittime;
        app_settings->ble_beacon_setup = BLE_BEACON_ON_WHEN_OFFLINE;
        Ble_Beacon_init(BLE_BEACON_ON_WHEN_OFFLINE);
        Ble_Beacon_set_configuration();
    }

    return request_reboot;
}


/**
    @brief      Defines what happens on a class command.

    A class command allows the node to transition between clases. For
    example, it allows changing all devices in class A to class C
    upon the reception of the app config.

    @param      command       The command
    @param      app_settings  The application settings

    @return     True if a reboot should happen.
*/
bool Overhaul_msg_class(overhaul_msg_class_t* command,
                        positioning_settings_t* app_settings)
{

    bool request_reboot = false;

    switch(command->value)
    {
        case POS_APP_CLASS_DEFAULT:
        case POS_APP_CLASS_A:
        case POS_APP_CLASS_B:
        case POS_APP_CLASS_C:
        case POS_APP_CLASS_D:
        case POS_APP_CLASS_E:
        case POS_APP_CLASS_F:
            app_settings->node_class = command-> value;
            break;
        default:
            break;
    }

    return request_reboot;
}
