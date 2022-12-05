/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 */

/*
 * \file    app.c
 * \brief   This file is a template for writing a custom application
 */

#include <stdlib.h>
#include <string.h>

#include "api.h"
#include "node_configuration.h"
#include "app_scheduler.h"
#include "led.h"
#include "stack_state.h"

// Api needed for ipv6 library
#include "ipv6_lib.h"
#include "ipv6_config.h"
#include "icmpv6.h"
#include "udp.h"

#include "button.h"

#define DEBUG_LOG_MODULE_NAME "IPV6_APP"
#define DEBUG_LOG_MAX_LEVEL LVL_INFO

#include "debug_log.h"

/** Time needed to execute the periodic work, in us */
#define EXECUTION_TIME_US   100
/** Period to send data */
#define DEFAULT_PERIOD_S    10
#define DEFAULT_PERIOD_MS   (DEFAULT_PERIOD_S*1000)

/**
 * Task to periodicaly send udp packet from node to offMeshAddress
 */
static uint32_t send_udp_data_task(void)
{
    static uint32_t counter = 0; // Incremental counter incremented every perido
    LOG(LVL_INFO, "Sending a UDP packet from port 20 to 30 of 4 bytes");
    udp_data_to_send_t udp_data_to_send = {
        .src_port = 20,
        .dst_port = 30,
        .length = 4
    };

    // Check if offMesh address is set
    if (!Ipv6_config_getOffMeshIpv6Address(udp_data_to_send.ipv6_dst_address))
    {
        LOG(LVL_INFO, "No offmesh address yet, trying again in %d s", DEFAULT_PERIOD_MS);
        return DEFAULT_PERIOD_MS;
    }

    uint32_t * data = (uint32_t *) Udp_getDataPtr();
    *data = counter++;
    udp_data_to_send.data = (uint8_t *) data;
    if (Udp_sendData(&udp_data_to_send) != UDP_RES_OK)
    {
        LOG(LVL_ERROR, "Cannot send udp packet, trying again in %d s", DEFAULT_PERIOD_MS);
    }

    return DEFAULT_PERIOD_MS;
}

void udp_data_cb(const udp_received_data_t * udp_received_data)
{
    LOG(LVL_INFO, "Received UDP packet");
    LOG(LVL_INFO, "UDP packet: src port %u, dst port %u, length %u", 
        udp_received_data->src_port, 
        udp_received_data->dst_port,
        udp_received_data->length);
    LOG_BUFFER(LVL_INFO, udp_received_data->data, 16);
}

static void received_echo_reply(const uint8_t * icmpv6_payload, uint16_t length)
{
    LOG(LVL_INFO, "Received an ICMPv6 ECHO REPLY length %u", length);
}

static void on_button_pressed(uint8_t button_id,
                              button_event_e event)
{
    // Google Ipv6 address is 2a00:1450:4007:808::2003
    static const uint8_t GOOGLE_IPV6_ADD[16] = {0x2a, 0x0, 0x14, 0x50, 0x40, 0x07, 0x08, 0x08, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x20, 0x03};
    uint8_t ipv6_dst_address[16];

    if (button_id == 0)
    {
        // Ping off_mesh address if set
        if (!Ipv6_config_getOffMeshIpv6Address(ipv6_dst_address))
        {
            LOG(LVL_INFO, "Not off_mesh address set yet, cannot ping it");
            return;
        }
    }
    else if (button_id == 1)
    {
        LOG(LVL_INFO, "Trying to ping google server!");
        // Ping google server (will work only if Global public unicast address set and a route configured to
        // the rest of the world)
        memcpy(ipv6_dst_address, GOOGLE_IPV6_ADD, sizeof(ipv6_dst_address));
    }
    else
    {
        LOG(LVL_ERROR, "Button_id is not register")
        return;
    }

    // From here, ipv6_dst_address should be set
    uint16_t identifier = (uint16_t) lib_time->getTimestampHp();
    LOG(LVL_INFO, "Sending ECHO REQUEST FROM APP with id=0x%x", identifier);
    uint8_t icmpv6_payload[10];

    // Identifier as first two bytes
    *((uint16_t *) icmpv6_payload) = identifier;
    // Set sequence to 0
    memset(icmpv6_payload + 2, 0, 2);

    // Add 6 extra bytes of payload
    memset(icmpv6_payload + 4, 0xAB, 6);

    Icmpv6_sendEchoRequest(ipv6_dst_address, icmpv6_payload, 10);

}

void App_init(const app_global_functions_t * functions)
{
    LOG_INIT();
    LOG(LVL_INFO, "Ipv6 demo app");
    // Basic configuration of the node with a unique node address
    if (configureNodeFromBuildParameters() != APP_RES_OK)
    {
        // Could not configure the node
        // It should not happen except if one of the config value is invalid
        return;
    }

    // Force node to be low latency
    lib_settings->setNodeRole(APP_LIB_SETTINGS_ROLE_HEADNODE_LL);

    // Configure the buttons for testing purpose
    Button_init();
    Button_register_for_event(0, BUTTON_PRESSED, on_button_pressed);
    Button_register_for_event(1, BUTTON_PRESSED, on_button_pressed);

    // Register callbacks for Icmpv6 Echo Reply
    Icmpv6_setEchoReplyCb(received_echo_reply);
    // Add callback for all udp traffic
    Udp_setReceivedDataCb(udp_data_cb);

    // Periodicaly send a udp packet
    App_Scheduler_addTask_execTime(send_udp_data_task,
                                   APP_SCHEDULER_SCHEDULE_ASAP,
                                   EXECUTION_TIME_US);

    // Start the stack
    Stack_State_startStack();
}
