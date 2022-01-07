/* Copyright 2022 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#include <stdlib.h>

#include "api.h"
#include "node_configuration.h"
#include "app_scheduler.h"
#include "shared_data.h"
#include "led.h"

#define DEBUG_LOG_MODULE_NAME "GEN APP"
#define DEBUG_LOG_MAX_LEVEL LVL_INFO
#include "debug_log.h"

/** Endpoint to send data */
#define DATA_EP       1

#define PAYLOAD_SIZE    102

static uint8_t m_message[PAYLOAD_SIZE];

static uint16_t m_packet_to_send = 0;

static uint16_t m_packet_sent = 0;

static uint16_t m_session_id = 0;

static size_t m_message_size = 0;

typedef struct __attribute__((packed)) {
    uint16_t    session_id;
    uint32_t    message_id;
} message_header_t;

typedef struct __attribute__((packed)) {
    uint16_t    session_id;
    uint32_t    number_messages;
    uint8_t     size;
} session_message_t;

static bool send_single_message(uint16_t session_id,
                                uint32_t message_id,
                                size_t size)
{
    message_header_t * header = (message_header_t *) m_message;

    if (size > PAYLOAD_SIZE)
    {
        size = PAYLOAD_SIZE;
    }
    else if (size < sizeof(message_header_t))
    {
        size = sizeof(message_header_t);
    }

    header->session_id = session_id;
    header->message_id = message_id;

    // Create a data packet to send
    app_lib_data_to_send_t data_to_send =
    {
        .bytes = (const uint8_t *) m_message,
        .num_bytes = size,
        .dest_address = APP_ADDR_ANYSINK,
        .src_endpoint = DATA_EP,
        .dest_endpoint = DATA_EP,
        .qos = APP_LIB_DATA_QOS_HIGH
    };

    // Send the data packet
    return Shared_Data_sendData(&data_to_send, NULL) == APP_LIB_DATA_SEND_RES_SUCCESS;
}

static uint32_t send_messages()
{
    if (m_packet_sent == m_packet_to_send)
    {
        return APP_SCHEDULER_STOP_TASK;
    }

    if (!send_single_message(m_session_id, m_packet_sent, m_message_size))
    {
        // Message not sent as stack buffers are probably full,
        // wait a bit to try again (100ms)
        Led_set(1, true);
        return 100;
    }
    Led_set(1, false);
    m_packet_sent++;

    return APP_SCHEDULER_SCHEDULE_ASAP;
}

static void start_session(size_t size, uint32_t number_of_message, uint16_t session_id)
{
    /* Initialize again session. It may end a running session */
    m_session_id = session_id;
    m_packet_sent = 0;
    m_message_size = size;
    m_packet_to_send = number_of_message;

    App_Scheduler_addTask_execTime(send_messages, APP_SCHEDULER_SCHEDULE_ASAP, 20);
}

static app_lib_data_receive_res_e shared_data_cb(
                                            const shared_data_item_t * item,
                                            const app_lib_data_received_t * data)
{
    LOG(LVL_DEBUG, "Rx (ep: %u -> %u, m: %u)",
                  item->filter.src_endpoint,
                  item->filter.dest_endpoint,
                  (uint8_t)item->filter.mode);

    if (data->num_bytes == sizeof(session_message_t))
    {
        session_message_t * message = (session_message_t * ) data->bytes;
        start_session(message->size, message->number_messages, message->session_id);
        LOG(LVL_ERROR,"Starting session %d for %d packets of size %d",
                    message->session_id,
                    message->number_messages,
                    message->size);
    }
    else
    {
        LOG(LVL_ERROR, "Wrong command format");
    }

    return APP_LIB_DATA_RECEIVE_RES_HANDLED;
}

/** Allow only packet in Unicast on endpoints [10, 10] */
static shared_data_item_t m_packet_session_start =
{
    .cb = shared_data_cb,
    .filter = {
                .mode = SHARED_DATA_NET_MODE_ALL,
                .src_endpoint = 10,
                .dest_endpoint = 10,
                .multicast_cb = NULL
              }
};

/**
 * \brief   Initialization callback for application
 *
 * This function is called after hardware has been initialized but the
 * stack is not yet running.
 *
 */
void App_init(const app_global_functions_t * functions)
{

    LOG_INIT();
    LOG(LVL_INFO, "Gen traffic app");

    // Basic configuration of the node with a unique node address
    if (configureNodeFromBuildParameters() != APP_RES_OK)
    {
        // Could not configure the node
        // It should not happen except if one of the config value is invalid
        return;
    }

    // Force to be a subnode
    lib_settings->setNodeRole(app_lib_settings_create_role(APP_LIB_SETTINGS_ROLE_SUBNODE,
                                                           APP_LIB_SETTINGS_ROLE_FLAG_LL));

    Led_init();

    Shared_Data_addDataReceivedCb(&m_packet_session_start);

    // Start the stack
    lib_state->startStack();
    Led_toggle(0);
}

