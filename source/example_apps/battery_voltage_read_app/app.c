/** Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 *  See file LICENSE.txt for full license details.
 *
 */

/*
 * @file    app.c
 * @brief   This file is a battery voltage reporting example application.
 *
 * This application measures battery voltage on selected hardware boards
 * (see config.mk).
 * It uses moving average filter to filter out peaks from measurements which
 * occur normally due to differences on processor load. Then, application can print
 * the averaged measured battery voltage on the UART interface if APP_PRINTING=yes
 * is defined in the application makefile. It also sends the value to backend via
 * a packet on dedicated reserved endpoints for backward compatibility with WNT
 * node voltage visualisation feature.
 * @note    Application assumes battery voltage can be measured on MCU VDD pin.
 */


#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "app_scheduler.h"

#include "api.h"
#include "node_configuration.h"
#include "hal_api.h"
#include "cbor.h"

#define DEBUG_LOG_MODULE_NAME "BATT VOLT RD APP"
#define DEBUG_LOG_MAX_LEVEL LVL_INFO
#include "debug_log.h"


 /** Endpoints reserved to battery voltage reception on backend side. */
#define APP_VOLT_SRC_EP 248
#define APP_VOLT_DST_EP 255

/** Key used in battery voltage packet. */
#define APP_VOLT_KEY_VOLTAGE 2ULL

/*
 * Voltage measurement interval (in seconds). This is kept really short just for
 * demonstration purposes. In real low energy installations, this should be way
 * rarer because measurement mechanism itself consumes energy and battery is not
 * draining that often.
 */
#define MEA_INTERVAL    60

/*
 * Size of averaging filter (samples). When filter is full, diagnostics packet
 * is sent. Note: Sending is done really often just for demonstration purposes.
 * In real life low energy installations, this should be done way rarer because
 * sending consumes energy and battery is not draining that often. For example:
 * once per day should be fine to send the voltage information. This now results
 * MEA_INTERVAL*60=3600s once per hour sending.
 */
#define AVG_FILTER_SIZE 60


/** Index for averaging filter. */
static uint8_t m_avg_filter_index=0;

/** Averaging filter itself. */
static uint32_t m_avg_filter[AVG_FILTER_SIZE];

/** Buffer for composing battery voltage packet data. */
static uint8_t m_tx_buf[102];

/** Encoder and map encoder. */
static CborEncoder m_encoder,m_mapEncoder;

/** Just a flag to ensure that buffer has been initialized. */
static bool m_initialized;

/**
 * @brief   Initialize cbor encoder and buffer.
 */
static void start_packet(void)
{
    /* Initialize cbor encoder. */
    cbor_encoder_init(&m_encoder, &m_tx_buf[0], sizeof(m_tx_buf), 0);
    cbor_encoder_create_map(&m_encoder, &m_mapEncoder, CborIndefiniteLength);
    m_initialized = true;
}

/**
 * @brief   Encode battery voltage value in CBOR format.
 *
 * @param value Value to encode
 *
 */
static bool encode_voltage_in_cbor(uint64_t value)
{
    if (!m_initialized)
    {
        start_packet();
    }

    /* Add key and value. */
    if (cbor_encode_uint(&m_mapEncoder, APP_VOLT_KEY_VOLTAGE) != CborNoError)
    {
        m_initialized = false;
        return false;
    }

    if (cbor_encode_uint(&m_mapEncoder, value) != CborNoError)
    {
        m_initialized = false;
        return false;
    }

    return true;
}

/**
 * @brief   Send battery voltage packet.
 * @note    Only for backward compatibility with WNT Client
 * node voltage visualisation feature and WNT API.
 */
static app_lib_data_send_res_e send_voltage_to_wnt(void)
{
    cbor_encoder_close_container(&m_encoder, &m_mapEncoder);
    /* Send packet. */
    app_lib_data_to_send_t tx_def =
    {
        .bytes = &m_tx_buf[0],
        .num_bytes = cbor_encoder_get_buffer_size(&m_encoder, &m_tx_buf[0]),
        .dest_address = APP_ADDR_ANYSINK,
        .delay = 0,
        .tracking_id = APP_LIB_DATA_NO_TRACKING_ID,
        .qos = APP_LIB_DATA_QOS_NORMAL,
        .flags = APP_LIB_DATA_SEND_FLAG_NONE,
        .src_endpoint = APP_VOLT_SRC_EP,
        .dest_endpoint = APP_VOLT_DST_EP,
        .hop_limit = 0,
    };
    m_initialized = false;
    return lib_data->sendData(&tx_def);
}

/**
 * @brief   Measure power supply output voltage task.
 *
 * @return Task's next calling time in milliseconds.
 */
uint32_t measure_voltage(void)
{
    /* Fill filter with a new power supply output voltage measurement. */
    m_avg_filter[m_avg_filter_index] = Mcu_voltageGet();

    /* Check if diagnostics is sent. */
    if (++m_avg_filter_index >= AVG_FILTER_SIZE)
    {
        /* Calculate average on items. */
        uint32_t avg=0;
        for (uint_fast8_t i=0; i < AVG_FILTER_SIZE; i++)
        {
            avg += m_avg_filter[i];
        }
        avg /= AVG_FILTER_SIZE;

        /* Print calculation result. */
        LOG(LVL_INFO, "Batt voltage is %lu mV", avg);

        /* Generate and send packet. */
        if (encode_voltage_in_cbor(avg))
        {
            if(send_voltage_to_wnt() != APP_LIB_DATA_SEND_RES_SUCCESS)
            {
                LOG(LVL_ERROR, "Could not send batt voltage");
            }
        }
        else
        {
            LOG(LVL_ERROR, "Could not encode voltage value");
        }

        /* And start filling buffer from the beginning. */
        m_avg_filter_index = 0;
    }
    return MEA_INTERVAL * 1000;
}

/**
 * @brief   Initialization callback for application
 *
 * This function is called after hardware has been initialized but the
 * stack is not yet running.
 *
 */
void App_init(const app_global_functions_t * functions)
{
    /* Init battery voltage value logging on UART. */
    LOG_INIT();
    LOG(LVL_INFO, "Starting Battery voltage read app");

    /* Basic configuration of the node with a unique node address. */
    if (configureNodeFromBuildParameters() != APP_RES_OK)
    {
        /*
         * Could not configure the node.
         * It should not happen except if one of the config value is invalid.
         */
        return;
    }

    App_Scheduler_addTask_execTime(measure_voltage,
                                   MEA_INTERVAL*1000,
                                   1000);

    /* Initialize voltage measurement. */
    Mcu_voltageInit();

    /*
     * Start the stack.
     * This is really important step, otherwise the stack will stay stopped and
     * will not be part of any network. So the device will not be reachable
     * without reflashing it.
     */
    lib_state->startStack();
}

