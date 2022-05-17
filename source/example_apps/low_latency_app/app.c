/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * @file    app.c
 * @brief   This file is an example application to demonstrate Low-Latency capabilities for a lighting usecase. 
 *          (see README for additional information)
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "api.h"
#include "node_configuration.h"
#include "led.h"
#include "button.h"
#include "shared_data.h"
#include "app_scheduler.h"
#include "app_persistent.h"

#define DEBUG_LOG_MODULE_NAME "LOWLATENCY_APP"
/** To activate logs, configure the following line with "LVL_INFO". */
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG

#include "debug_log.h"

#define MAX_LED_PER_BOARD 4u
#define SWITCH_ON_COMMAND_PATTERN 0b00000001 
/** Endpoint used to communicate. */
#define DATA_EP        (25u)

/** Time needed to execute the send "button pressed message" task, in us. */
#define TASK_EXEC_TIME_US_SEND_BUTTON_PRESSED_MSG (250u)

/** ENUMS. */
/** @brief LED state. */
/** LED is switched OFF. */
#define LED_STATE_OFF 0
/** LED is switched ON. */
#define LED_STATE_ON 1


/** Message ID. */
typedef enum 
{
    /** Button pressed message, send to other nodes through active flooding. */
    MSG_ID_SWITCH_COMMAND  = 1,
    /** Set led state message, downlink message.*/
    MSG_ID_DOWNLINK_SET_LED_STATE = 2,
    /** Grouping message, downlink message, to add the node to a multicast group. */
    MSG_ID_MULTICAST_GROUPING = 3
} message_id_e;

/** Button ID which was pressed. */
static uint8_t m_button_pressed_id;

/** Multicast group address. */
static uint32_t m_mcast_address; /* This node is part of group 1. */

/** STRUCTURES. */

/** @brief Switch message payload data structure. */
typedef struct __attribute__((packed))
{
    uint8_t button_id;
    uint8_t action;
    uint16_t counter;
} payload_switch_command_t;

/** @brief Downlink light on/off message payload data structure. */
typedef struct __attribute__((packed))
{
    uint8_t led_id;
    uint8_t brightness;
} payload_on_off_lighting_t;

/** @brief Grouping message payload data structure. */
typedef struct __attribute__((packed))
{
    uint32_t multicast_address;
} payload_grouping_command_t;


/** @brief Application message data structure. */
typedef struct __attribute__((packed))
{
    uint8_t id;
    union
    {
        payload_switch_command_t         switch_status;
        payload_on_off_lighting_t        set_brightness;
        payload_grouping_command_t       set_multicast_address;
    } payload;
} msg_t;


/** 
 * @brief   LED state control function.
 * @param   led_id
 *          LED ID of led to change state.
 * @param   led_state
 *          Requested LED state to apply.
 */
static void set_led_state(uint8_t led_id, uint8_t led_state)
{
    LOG (LVL_INFO,
    "Set led_id %d to state %d", led_id, led_state);
    /* Switch the Led */
    if(Led_set(led_id,led_state) == LED_RES_INVALID_ID)
    {
        LOG(LVL_WARNING, "Led invalid id");
    }
}

/** Send functions. */
/** 
 * @brief Function sending a message to all nodes, with node to node method.
 * @param id : message Id, @ref message_id_e.
 * @param payload : pointer on the data to send.
 * @return  Result code, APP_LIB_DATA_SEND_RES_SUCCESS means that data was
 *          accepted for sending. See @ref app_res_e for other result codes.
 * */
static app_lib_data_send_res_e send_node_to_node_msg(message_id_e message_id, uint8_t * payload)
{
    LOG(LVL_INFO, "Send message payload");
    msg_t msg; /* Create node to node message structure. */
    size_t msg_byte_size = sizeof (msg.id); /* Message to send byte size. */
    msg.id = (uint8_t)message_id;

    /* Fill the message. */
    memcpy(&msg.payload.switch_status, 
        (payload_switch_command_t *)payload, 
        sizeof(payload_switch_command_t));
    msg_byte_size += sizeof(payload_switch_command_t);

    /* Create a packet data to send. */
    app_lib_data_to_send_t data_to_send;
    data_to_send.bytes = (const uint8_t *) &msg;
    data_to_send.num_bytes = msg_byte_size;
    data_to_send.dest_address = APP_ADDR_BROADCAST;
    data_to_send.src_endpoint = DATA_EP;
    data_to_send.dest_endpoint = DATA_EP;
    data_to_send.qos = APP_LIB_DATA_QOS_HIGH;
    data_to_send.delay = 0;
    data_to_send.flags = APP_LIB_DATA_SEND_FLAG_NONE;
    data_to_send.tracking_id = APP_LIB_DATA_NO_TRACKING_ID;

    /* Send the data packet. */
    return Shared_Data_sendData(&data_to_send, NULL);
}

/** CALLBACKS. **/
/* Multicast group filter for Downlink packets. */
static bool filter_multicast_cb(app_addr_t received_multicast_addr)
{
    LOG(LVL_INFO, "Multicast address received : 0x%x", received_multicast_addr);
    return m_mcast_address == received_multicast_addr;
}

/** @brief : set_led_for_button
 * Set led led_id on or off depending on button pressed :
 * Switch ON for button 0
 * Switch OFF for button 1
 * */
static void set_led_for_button(uint8_t button_id, uint8_t led_id)
{
    /* Check button received : */
    if (button_id == 0)
    {
       LOG(LVL_INFO, "set led %d ON", led_id);
       set_led_state(led_id, LED_STATE_ON);
    }
    if (button_id == 1)
    {
        LOG(LVL_INFO, "set led %d OFF", led_id);
        set_led_state(led_id, LED_STATE_OFF);
    }
}


/** Messages handling callback (Broadcast, Unicast and Multicast). */
static app_lib_data_receive_res_e data_received_cb(
    const shared_data_item_t * item,
    const app_lib_data_received_t * data)
{
    msg_t *msg = (( msg_t *) data->bytes);
    uint8_t msg_size = data->num_bytes;

    LOG (LVL_INFO,
        "Message received id %d",
        msg->id);
    if (msg_size < sizeof (msg->id))
    {
        /* Data is not for this application. */
        return APP_LIB_DATA_RECEIVE_RES_NOT_FOR_APP;
    }
    msg_size -= sizeof(msg->id);
    switch (msg->id)
    {
        case MSG_ID_SWITCH_COMMAND : /* Switch command. */
            LOG(LVL_INFO, "Msg Switch command received");
            /* Check if received message length match expected one. */
            if (msg_size == sizeof(payload_switch_command_t))
            {
                payload_switch_command_t *payload = & msg->payload.switch_status;
                LOG (LVL_INFO, "Switch command received : button_id = %d, action = %d", payload->button_id, payload->action);
                /* Check the action type. */
                if (payload->action == 1) /* Button pressed. */ 
                {
                   set_led_for_button(payload->button_id, payload->action); 
                }
                else if (payload->action == 0)
                {
                    /* Button released, no action yet. */
                }
            }
            else 
            {
                LOG(LVL_INFO, "Message received doesn't match the size of the expected message.");
            }
        break;

        case MSG_ID_DOWNLINK_SET_LED_STATE: /* Brightness Downlink message. */
            /*Check if received message length match expected one. */
            if (msg_size == sizeof(payload_on_off_lighting_t))
            {
                if (msg->payload.set_brightness.brightness == 1) /* Set led ON. */
                {
                    LOG(LVL_INFO,
                    "Downlink brightness action %d, set Led ON",
                    msg->payload.set_brightness.brightness);
                    set_led_state(msg->payload.set_brightness.led_id, LED_STATE_ON);
                }
                if (msg->payload.set_brightness.brightness == 0) /* Set led OFF. */
                {
                    LOG(LVL_INFO,
                    "Downlink brightness action %d, set Led ON",
                    msg->payload.set_brightness.brightness);
                    set_led_state(msg->payload.set_brightness.led_id, LED_STATE_OFF);
                }
            }
            else
            {
                /* Message error. */
                LOG(LVL_ERROR, "Message recieved size doesn't correspond to the payload size");
            }
        break;

        case MSG_ID_MULTICAST_GROUPING:
            /*Check if received message length match expected one. */
            if(msg_size == sizeof(payload_grouping_command_t))
            {
               LOG(LVL_INFO,
                    "Set new Mcast address : 0x%x",
                    msg->payload.set_multicast_address.multicast_address); 
                m_mcast_address=msg->payload.set_multicast_address.multicast_address;
                app_persistent_res_e res = App_Persistent_write((uint8_t *) &m_mcast_address, sizeof(m_mcast_address));
                if (res != APP_PERSISTENT_RES_OK)
                {
                    LOG(LVL_ERROR, "Cannot write\n");
                }
            }
        break;
        default:
        break;
    }
    /* Data handled successfully. */
    return APP_LIB_DATA_RECEIVE_RES_HANDLED;
}


/** FILTERS. */
/** Broadcast, multicast and unicast packets filter. */
static shared_data_item_t alltype_packets_filter =
{
    .cb = data_received_cb,
    .filter = {
        .mode = SHARED_DATA_NET_MODE_ALL,
        /* Filtering by source endpoint. */
        .src_endpoint = DATA_EP,
        /* Filtering by destination endpoint. */
        .dest_endpoint = DATA_EP,
        .multicast_cb = filter_multicast_cb
        }
};

/**
 * @brief   Task handling "button pressed" message sending.
 *
 *          This task will be called each time a button pressed event
 *          is generated.
 *          Note: Some event might be missed if new interrupts are generated and
 *          some previous message sending process is not over.
 *          Only the last event is sent.
 *
 *          This task is called when a button is pressed down.
 */
static uint32_t task_send_button_pressed_msg(void)
{
    payload_switch_command_t payload;      /* Message payload data. */

    static uint32_t counter_value = 0;
    payload.counter = counter_value;
    uint8_t led_id = 1;
    
    /* Construct switch command. */
    payload.button_id = m_button_pressed_id; 
    payload.action = 1; /* Always do something when a button is pressed */
    /* Send message. */
    app_lib_data_send_res_e res = send_node_to_node_msg(MSG_ID_SWITCH_COMMAND, (uint8_t *)&payload);
    if ( res != APP_LIB_DATA_SEND_RES_SUCCESS)
    {
        /*
         * Message was not accepted for sending.
         * Error handling can be performed here.
         */
        LOG(LVL_ERROR, "Message was not accepted for sending. ERROR CODE : %d", res);
    }
    counter_value ++;
    LOG(LVL_INFO, "Button pressed id : %d", m_button_pressed_id);
    /* Light On or Off current board led */
    set_led_for_button(m_button_pressed_id, led_id);

    return APP_SCHEDULER_STOP_TASK;
}


/**
 * @brief   Callback for button pressed event which store information to send in
 *          the "button event message. "
 *
 *          This function will be called each time a button pressed event
 *          is generated.
 *
 * @param   button_id
 *          Button's number that was pressed.
 * @param   event
 *          Always BUTTON_PRESSED here.
 *
 * This function is called when a button is pressed down.
 */
static void button_pressed_handler(uint8_t button_id, button_event_e event)
{
    /* Store button_id to process it in a dedicated application task. */
    m_button_pressed_id = button_id;
    LOG(LVL_INFO,"Button %d pressed",  m_button_pressed_id);

    /*
     * Send "button pressed message" in a single shot application task (called
     * each time button is pressed) as we are here in an IRQ context.
     */
    App_Scheduler_addTask_execTime(task_send_button_pressed_msg,
                                   APP_SCHEDULER_SCHEDULE_ASAP,
                                   TASK_EXEC_TIME_US_SEND_BUTTON_PRESSED_MSG);
}


/**
 * @brief   Initialization callback for application.
 *
 * This function is called after hardware has been initialized but the
 * stack is not yet running.
 *
 */
void App_init(const app_global_functions_t * functions)
{
    app_persistent_res_e res;

    LOG_INIT();
    LOG(LVL_INFO, "Starting low-latency application");
    uint8_t num_buttons;

    /* Basic configuration of the node with a unique node address. */
    if (configureNodeFromBuildParameters() != APP_RES_OK)
    {
        /*
         * Could not configure the node.
         * It should not happen except if one of the config value is invalid.
         */
        return;
    }

    /*
     * Set node operating mode to router low-latency
     */
    lib_settings->setNodeRole(APP_LIB_SETTINGS_ROLE_HEADNODE_LL);


    /* Use app persistance to read and write multicast address. */
    res = App_Persistent_read( (uint8_t* ) &m_mcast_address, sizeof(m_mcast_address));
    if (res == APP_PERSISTENT_RES_INVALID_CONTENT)
    {
        LOG(LVL_INFO, "App persistant first initialization\n");
        m_mcast_address = 0x80000001;
        App_Persistent_write((uint8_t *) &m_mcast_address, sizeof(m_mcast_address));
    }
    else if (res == APP_PERSISTENT_RES_UNINITIALIZED)
    {
        LOG(LVL_ERROR, "Persistent area is not initialized as it should (no area defined?)\n");
    }

    /* Set up LED. */
    Led_init();

    /* Set up buttons. */
    Button_init();
    num_buttons = Button_get_number();

    for (uint8_t button_id = 0; button_id < num_buttons; button_id++)
    {
        /* Register button pressed event on all user available button. */
        Button_register_for_event(button_id,
                                  BUTTON_PRESSED,
                                  button_pressed_handler);
    }

    /* Set unicast, multicast & broadcast received messages callback. */
    Shared_Data_addDataReceivedCb(&alltype_packets_filter);

    /*
     * Start the stack.
     * This is really important step, otherwise the stack will stay stopped and
     * will not be part of any network. So the device will not be reachable.
     * without reflashing it.
     */
    lib_state->startStack();
}
