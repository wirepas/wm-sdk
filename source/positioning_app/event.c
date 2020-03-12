/**
    @file events.c
    @brief      Main code to process the application callbacks.
    @copyright  Wirepas Oy 2019
*/

#include "positioning_app.h"
#include "app_settings.h"
#include "measurements.h"
#include "overhaul.h"
#include "event.h"

/**
    @brief      Processing of a network/cluster beacon.

            The observed beacon is matched against the beacon table. If it is
            found, the rss value is updated, otherwise it is append (if there
            is enough space).

    @param[in]  beacon  The received network/cluster beacon

    @return     { description_of_the_return_value }
*/
scheduler_state_e Event_beacon_reception(const app_lib_state_beacon_rx_t*
        beacon)
{
    scheduler_state_e state = Scheduler_get_state();
    lib_system->enterCriticalSection();
    Measurements_insert_beacon(beacon); // defines lookup size
    lib_system->exitCriticalSection();
    return state;
}


/**
    @brief      Processing of a network scan edge.

            When a network scan is performed, the application bundles all the
            available beacons in positioning measurement payloads and
            dispatches them towards a sink.

            A message is only sent if there is at least one neighbor.

            After the message is *requested* to be sent, the node either goes
            to sleep or stays idle - waiting for a network scan trigger.

            Possible state changes:
                - Wait for packet acknowledgement;
                - Wait for connection;


    @return     returns the next scheduler state.
*/
scheduler_state_e Event_network_scan_end(void)
{

    positioning_settings_t* app_settings = Pos_get_settings();
    scheduler_state_e state =  Scheduler_get_state();

    uint8_t rc = 0xFF;
    size_t available_buffers;
    uint8_t len_payload = 0;
    const uint8_t* ptr_payload = 0;
    app_lib_data_to_send_t payload;

    positioning_measurements_e meas_type = DEFAULT_MEASUREMENT_TYPE_TAG;

    switch(app_settings->node_mode)
    {
        case POS_APP_MODE_NRLS_TAG:
        case POS_APP_MODE_AUTOSCAN_TAG:
            meas_type = DEFAULT_MEASUREMENT_TYPE_TAG;
            break;
        case POS_APP_MODE_AUTOSCAN_ANCHOR:
        case POS_APP_MODE_OPPORTUNISTIC_ANCHOR:
            meas_type = DEFAULT_MEASUREMENT_TYPE_ANCHOR;
            break;
        default:
            meas_type = DEFAULT_MEASUREMENT_TYPE_TAG;
            break;
    }

    lib_data->getNumFreeBuffers(&available_buffers);

    if(Measurements_get_num_beacon() > 0
            && !Scheduler_get_message_dispatch()
            && available_buffers > 0
            && app_settings->scan_period > 0)
    {

        lib_system->enterCriticalSection();
        ptr_payload = Measurements_payload_init();
        Measurements_payload_add_rss(meas_type);
        len_payload = Measurements_payload_length();

        //add voltage payload
        #if CONF_VOLTAGE_REPORT == 1
        if( !(len_payload + sizeof(measurement_header_t) + sizeof(measurement_payload_voltage_t) >
          MAX_PAYLOAD))
        {
            Measurements_payload_add_voltage();
            len_payload = Measurements_payload_length();
        }
        #endif

        // ensure proper tracking id
        if(Scheduler_get_message_sequence_id() >= 0xFFFF)
        {
            Scheduler_init_message_sequence_id();
        }

        payload.bytes = ptr_payload;
        payload.num_bytes = len_payload;
        payload.dest_address = APP_ADDR_ANYSINK;
        payload.src_endpoint = app_settings->source_endpoint;
        payload.dest_endpoint = app_settings->destination_endpoint;
        payload.qos = app_settings->payload_qos;
        payload.delay = 0;
        payload.flags = APP_LIB_DATA_SEND_FLAG_TRACK;
        payload.tracking_id = (uint16_t) Scheduler_get_message_sequence_id() & 0xFFFF;

        rc = lib_data->sendData(&payload);

        // stop callbacks
        if (rc == APP_LIB_DATA_SEND_RES_SUCCESS)
        {
            state = POS_APP_STATE_WAIT_FOR_PACKET_ACK;

            Scheduler_set_message_dispatch(true);
            Scheduler_increment_message_sequence_id();
        }
        else
        {

            if(rc == APP_LIB_DATA_SEND_RES_INVALID_TRACKING_ID)
            {
                Scheduler_init_message_sequence_id();
            }

            // ignore data failure and wait for app config
            state = POS_APP_STATE_WAIT_FOR_APP_CONFIG;
        }
        lib_system->exitCriticalSection();
    }
    else // no beacons received yet
    {
        state = POS_APP_STATE_DOWNTIME;
        Ble_Beacon_check_monitoring(MON_CONN_FAIL);
    }

    return state;
}


/**
    @brief   Processing of a data packet acknowledgement.

             When a packet acknowledgemetn is received, the application moves
             on to sleep or to idle until the next scan.

             However, the applicaiton moves on to sleep if the appconfig
             message has been received (which happens with all connected
             members) or the sent message was dispatched with success.

             Possible state changes:
                - Wait for app config;
                - Wait for connection;

    @param[in]  status        The status

    @return     returns the next scheduler state.
*/
scheduler_state_e Event_data_ack(const app_lib_data_sent_status_t* status)
{
    scheduler_state_e state = Scheduler_get_state();
    uint32_t current_seq_id = Scheduler_get_message_sequence_id();
    uint16_t id_match;

    if (status->success)
    {
        // make sure it does not overflow
        id_match = (uint16_t) status->tracking_id;
        id_match++;

        // Received ack of the most recently sent message so connection is ok
        if (current_seq_id == id_match)
        {
            Ble_Beacon_check_monitoring(MON_UPD_LATEST_MSG);
        }

        // sleep immediately if message other than 0
        if (Scheduler_get_appconfig_reception()
                && Scheduler_get_message_dispatch()
                && current_seq_id == id_match)

        {
            state = POS_APP_STATE_DOWNTIME;
        }
        else if(current_seq_id != id_match)
        {
            state = POS_APP_STATE_WAIT_FOR_PACKET_ACK;
        }
        else if(Scheduler_get_appconfig_reception() == false)
        {
            state = POS_APP_STATE_WAIT_FOR_APP_CONFIG;
        }
        else
        {
            state = POS_APP_STATE_DOWNTIME;
        }
    }
    else
    {
        state = POS_APP_STATE_WAIT_FOR_CONNECTION;
    }


    return state;
}


/**
    @brief   Processing of an appconfig message.

             When a node receives an appconfig, it attempts to decode it
             against the known overhaul types.

             By default nothing else is done.

             Possible state changes:
             - none;

    @param[in]  bytes         The bytes
    @param[in]  num_bytes     The number bytes
    @param[in]  seq           The sequence

    @return     returns the next scheduler state.
*/
scheduler_state_e Event_app_config(const uint8_t* bytes,
                                   uint32_t num_bytes,
                                   uint8_t seq)
{
    scheduler_state_e state = Scheduler_get_state();
    Ble_Beacon_check_monitoring(MON_UPD_APPCONF);

    lib_system->enterCriticalSection();
    // acts on decoded information - if any
    if(Overhaul_decode_config(bytes, num_bytes, seq))
    {
        state = Overhaul_msg();
    }
    lib_system->exitCriticalSection();
    return state;
}
