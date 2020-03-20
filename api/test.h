/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file test.h
 *
 * Application library for testing services
 *
 */

#ifndef APP_LIB_TEST_H_
#define APP_LIB_TEST_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "app/app.h"

/** @brief Library symbolic name  */
#define APP_LIB_TEST_NAME 0x0013aa4c //!< "TEST"

/** @brief Maximum supported library version */
#define APP_LIB_TEST_VERSION 0x203

/**
 * @brief Reservation for maximum test data size in radio interface
 *
 * Test data header includes length (1 Byte) and sequence
 * number (4 Bytes) which allocates fixed 5 bytes from over the air
 * payload, remaining bytes can be used for free form data.
 *
 * Maximum data size depends on used radio hardware. Here we allocate
 * big enough buffer to cover all supported radio hardwares and
 * some extra bytes for future needs.
 *
 * True size of the radio payload can be read with test_lib->getMaxDataSize().
 *
 * Library services are accessed via @ref app_lib_test_t "lib_test"
 * handle.
 */
#define APP_LIB_TEST_MAX_DATA_SIZE   (180-1-4)

/**
 * @brief   Return values for enter test mode command
 */
typedef enum
{
    /** Test mode entered successfully */
    APP_LIB_TEST_SET_MODE_SUCCESS               = 0,
    /** Given network address is not valid */
    APP_LIB_TEST_SET_MODE_NWK_ADDRESS_INVALID   = 1,
    /** Test mode not allowed. Stack is running, it should
     *  be stopped first to avoid parallel radio access.*/
    APP_LIB_TEST_SET_MODE_REJECTED              = 2,
} app_lib_test_mode_enter_res_e;

/**
 * @brief   Return values for exit test mode command
 */
typedef enum
{
    /** Test mode exited successfully */
    APP_LIB_TEST_EXIT_MODE_SUCCESS   = 0,
    /** Device is not in test mode */
    APP_LIB_TEST_EXIT_MODE_REJECTED  = 1,
} app_lib_test_mode_exit_res_e;

/**
 * @brief   Return values for reading maximum test data size for
 *          transmission tests
 */
typedef enum
{
    /** Maximum data size read successfully */
    APP_LIB_TEST_DATA_SIZE_READ_SUCCESS  = 0,
} app_lib_test_get_max_data_size_res_e;

/**
 * @brief   Return values for radio channel command
 */
typedef enum
{
    /** Radio channel set successfully */
    APP_LIB_TEST_RADIO_CHANNEL_SUCCESS  = 0,
    /** Given channel number is not valid  */
    APP_LIB_TEST_RADIO_CHANNEL_INVALID  = 1,
    /** Device is not in test mode  */
    APP_LIB_TEST_RADIO_CHANNEL_REJECTED = 2,
} app_lib_test_set_radio_channel_res_e;

/**
 * @brief   Return values for radio power command
 */
typedef enum
{
    /** Radio power level set successfully */
    APP_LIB_TEST_RADIO_POWER_SUCCESS    = 0,
    /** Given power level is not valid */
    APP_LIB_TEST_RADIO_POWER_INVALID    = 1,
    /** Device is not in test mode */
    APP_LIB_TEST_RADIO_POWER_REJECTED   = 2,
} app_lib_test_set_radio_power_level_res_e;

/**
 * @brief   Return values for data send command
 */
typedef enum
{
    /** All test data transmitted successfully */
    APP_LIB_TEST_RADIO_SEND_SUCCESS         = 0,
    /** Sending prohibited due to busy channel (CCA fail) */
    APP_LIB_TEST_RADIO_SEND_CCA_ERROR       = 1,
    /** Transmission failed, check parameters */
    APP_LIB_TEST_SEND_FAILED                = 2,
    /** Device is not in test mode */
    APP_LIB_TEST_RADIO_SEND_REJECTED        = 3,
} app_lib_test_send_radio_data_res_e;

/**
 * @brief   Return values for enable/disable reception command
 */
typedef enum
{
    /** Radio reception enabled/disabled successfully */
    APP_LIB_TEST_RADIO_RECEPTION_SET_SUCCESS    = 0,
    /** Device is not in test mode */
    APP_LIB_TEST_RADIO_RECEPTION_SET_REJECTED   = 1,
} app_lib_test_radio_reception_control_res_e;

/**
 * @brief   Return values for reception callback command
 */
typedef enum
{
    /** Radio reception callback function set successfully */
    APP_LIB_TEST_RADIO_RECEPTION_CB_SUCCESS     = 0,
} app_lib_test_radio_reception_cb_res_e;

/**
 * @brief   Return values for test data read command
 */
typedef enum
{
    /** Radio test data package read successfully */
    APP_LIB_TEST_RADIO_READ_DATA_SUCCESS    = 0,
    /** No test data received */
    APP_LIB_TEST_RADIO_READ_NO_DATA         = 1,
    /** Device is not in test mode */
    APP_LIB_TEST_RADIO_READ_REJECTED        = 2
} app_lib_test_set_radio_read_data_res_e;

/**
 * @brief   Return values of data reception CB function
 */
typedef enum
{
    /** Received data handled successfully */
    APP_LIB_TEST_RADIO_RECEIVE_RES_HANDLED  = 0,
    /** Data received but there was no space in data queues
     *  to handle it right now */
    APP_LIB_TEST_RADIO_RECEIVE_RES_NO_SPACE = 1,
} app_lib_test_radio_reception_res_e;

typedef enum
{
    /** Transmits random symbols */
    APP_LIB_TEST_SIGNAL_RANDOM       = 0,
    /** Transmits all symbols (0 -255) in the signaling alphabet 0-255,
     *  ref. FCC 558074 D01 15.247 Meas Guidance v05.*/
    APP_LIB_TEST_SIGNAL_ALL_SYMBOLS  = 1,
} app_lib_test_signal_type_e;

/**
 * @brief Header of test data send over the air
 *
 * Used in @ref app_lib_test_data_tx_payload_t
 */
typedef struct
{
    /** Sequence number of the message  */
    uint32_t    seq;
    /** Length of the test data  */
    uint8_t     len;
} app_lib_test_data_header_t;

/**
 * @brief   Structures for test data counters
 *
 * Used in @ref app_lib_test_data_received_t
 */
typedef struct
{
    /** Number of received messages since test mode started */
    uint32_t    cntr;
    /** Number of duplicate messages since test mode started
     *  Incremented if received message has the same sequence number as
     *  in previous test data package */
    uint32_t    dup;
} app_lib_test_data_counters_t;

/**
 * @brief   Structures for test data reception
 */
typedef struct
{
    /** Received signal strength in dBm with resolution of 1dBm. */
    int8_t rssi;
    /** Received data counters. */
    app_lib_test_data_counters_t rxCntrs;
    /** Header of received test data */
    app_lib_test_data_header_t hdr;
    /** Received test data */
    uint8_t data[APP_LIB_TEST_MAX_DATA_SIZE];
} app_lib_test_data_received_t;

/**
 * @brief   Structures for test data transmission
 */
typedef struct
{
    /** Number of messages to be sent per request.
        Bigger the bursts value is the longer the transmission takes and delays
        response to the caller.  Practical values are from 1 to 1000. */
    uint32_t    bursts;
    /** Time for Clear Channel Assessment in us. If 0, no CCA done.
        Practical values are from 0 to tens of milliseconds */
    uint32_t    ccaDuration;
    /** When CCA is not used (ccaDuration=0) transmit interval defines the delay between
     *  TX bursts in us. If 0, sending done as soon as possible. */
    uint32_t    txInterval;
} app_lib_test_data_transmitter_control_t;

/**
 * @brief   Data transmission payload format
 */
typedef struct
{
    /** Header for fixed data fields */
    app_lib_test_data_header_t hdr;
    /** Free form test data */
    const uint8_t * data;
} app_lib_test_data_tx_payload_t;

typedef struct
{
    /** Parameters for test data transmission */
    app_lib_test_data_transmitter_control_t txCtrl;
    /** Data to be transmitted over the air */
    app_lib_test_data_tx_payload_t txPayload;
} app_lib_test_data_transmit_t;

typedef struct
{
    /** Parameters for test data transmission */
    app_lib_test_data_transmitter_control_t txCtrl;
    /** Test signal type to be trasmitted */
    app_lib_test_signal_type_e  signalType;
 } app_lib_test_signal_transmit_t;

/**
 * @brief   Activates the test mode
 * @param   addr
 *          On-air radio address to be used in radio tests
 * @return  Result code
 */
typedef app_lib_test_mode_enter_res_e
    (*app_lib_test_set_test_mode_f) (const uint32_t addr);

/**
 * @brief   Exit from the test mode
 * @return  Result code
 */
typedef app_lib_test_mode_exit_res_e
    (*app_lib_test_exit_test_mode_f) (void);

/**
 * @brief   Get the maximum data size radio can handle
 * @param   size
 *          Pointer where to store the size
 * @return  Result code
 */
typedef app_lib_test_get_max_data_size_res_e
    (*app_lib_test_get_max_data_size_f) (uint8_t * size);

/**
 * @brief   Sets radio channel
 * @param   radio_channel
 *          Logical channel number for transceiver
 * @return  Result code
 */
typedef app_lib_test_set_radio_channel_res_e
    (*app_lib_test_set_radio_channel_f) (const uint8_t radio_channel);

/**
 * @brief   Sets radio power level
 * @param   dbm
 *          Power level for transceiver
 * @return  Result code
 */
typedef app_lib_test_set_radio_power_level_res_e
    (*app_lib_test_set_radio_tx_power_level_f) (const int8_t dbm);

/**
 * @brief   Sends test data
 * @param   data
 *          Defines parameters for transmission and test data itself
 * @param   sentBursts
 *          Pointer to location where to store number of sent bursts
 * @return  Result code
 */
typedef app_lib_test_send_radio_data_res_e
  (*app_lib_test_send_radio_data_f) (const app_lib_test_data_transmit_t * data,
                                                         uint32_t * sentBursts);

/**
 * @brief   Sends test signal
 * @param   data
 *          Defines type of the test signal and parameters for transmission
 * @param   sentBursts
 *          Pointer to location where to store number of sent bursts
 * @return  Result code
 */
typedef app_lib_test_send_radio_data_res_e
  (*app_lib_test_send_radio_test_signal_f) \
                                   (const app_lib_test_signal_transmit_t * data,
                                    uint32_t * sentBursts);

/**
 * @brief   Control of data reception
 * @param   rxEnable
 *          True to enable receiver and calling of reception callback function.
 *          False to disable receiver and calling callback function.
 * @param   dataIndiEnable,
 *          True to enable sending of data indications each time new test data
 *          package is received.
 *          False to disable sending of data indications.
 *          Note! Latest received data package can always be read with
 *          app_lib_test_read_radio_data_f.
 * @return  Result code
 */
typedef app_lib_test_radio_reception_control_res_e
    (*app_lib_test_allow_radio_reception_f) (const bool rxEnable,
                                             const bool dataIndiEnable);

/**
 * @brief   Function type for test data callback function
 * @param   data
 *          Received data
 * @return  Result code
 */
typedef app_lib_test_radio_reception_res_e
    (*app_lib_test_data_received_cb_f)(app_lib_test_data_received_t * data);

/**
 * @brief   Defines callback function to handle received test data
 * @param   cb
 *          The function to be executed, or NULL to void
 * @return  Result code
 */
typedef app_lib_test_radio_reception_cb_res_e
    (*app_lib_test_set_data_received_cb_f)(app_lib_test_data_received_cb_f cb);

/**
 * @brief   Reads the last received test data package from buffer
 * @param   data
 *          Pointer to the data structure where data will be copied
 * @return  Result code
 */
typedef app_lib_test_set_radio_read_data_res_e
     (*app_lib_test_read_radio_data_f)(app_lib_test_data_received_t * data);

/**
 * @brief   List of library functions
 */
typedef struct
{
    app_lib_test_set_test_mode_f                setTestMode;
    app_lib_test_exit_test_mode_f               exitTestMode;
    app_lib_test_get_max_data_size_f            getMaxDataSize;
    app_lib_test_set_radio_channel_f            setRadioChannel;
    app_lib_test_set_radio_tx_power_level_f     setRadioTXPowerLevel;
    app_lib_test_send_radio_data_f              sendRadioData;
    app_lib_test_allow_radio_reception_f        allowRadioReception;
    app_lib_test_set_data_received_cb_f         setDataReceivedCb;
    app_lib_test_read_radio_data_f              readRadioData;
    app_lib_test_send_radio_test_signal_f       sendRadioTestSignal;
} app_lib_test_t;


#endif /* APP_LIB_TEST_H_ */
