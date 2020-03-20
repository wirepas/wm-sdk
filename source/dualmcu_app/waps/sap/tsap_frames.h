// TEST_REG_SDK_ONLY_BEGIN
/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef TSAP_FRAMES_H_
#define TSAP_FRAMES_H_

/** Reservation for maximum size of test data in WAPS/TSAP interface.
 *  True size depends on used radio and it can be read with
 *  WAPS_FUNC_TSAP_RADIO_GET_MAX_DATA_SIZE_REQ */
#define APDU_TSAP_MAX_SIZE  APP_LIB_TEST_MAX_DATA_SIZE

/** Return values for test mode enter confirmation
 *  WAPS_FUNC_TSAP_MODE_ENTER_CNF  */
typedef enum
{
    /** Success: Test mode entered successfully */
    TSAP_ENTER_TEST_MODE_SUCCESS                = 0,
    /** Failure: Given network address is not valid */
    TSAP_ENTER_TEST_MODE_NWK_ADDRESS_INVALID    = 1,
    /** Failure: Stack is running, it should be stopped first before
     *  enabling test mode to avoid conflicting radio access. */
    TSAP_ENTER_TEST_MODE_REJECTED               = 2,
} tsap_enter_result_e;

/** Return values for test mode exit confirmation
 *  WAPS_FUNC_TSAP_EXIT_CNF */
typedef enum
{
    /** Success: Test mode exited successfully */
    TSAP_EXIT_TEST_MODE_SUCCESS     = 0,
    /** Failure: Device is not in test mode  */
    TSAP_EXIT_TEST_MODE_REJECTED    = 1,
} tsap_exit_result_e;

/** Return values for reading maximum data size which radio
 *  can send/receive at once.
 *  WAPS_FUNC_TSAP_RADIO_GET_MAX_DATA_SIZE_CNF */
typedef enum
{
    /**  Maximum data size read successfully */
    TSAP_RADIO_DATA_SIZE_READ_SUCCESS   = 0,
    /** Failure: Device was not in test mode  */
    TSAP_RADIO_DATA_SIZE_READ_REJECTED  = 1,
}tsap_radio_max_data_size_result_e;

/** Return values for setting radio channel confirmation
 *  WAPS_FUNC_TSAP_SET_RADIO_CHANNEL_CNF */
typedef enum
{
    /** Success:  Radio channel set successfully */
    TSAP_RADIO_CHANNEL_SUCCESS  = 0,
    /** Failure: Given channel number is not valid */
    TSAP_RADIO_CHANNEL_INVALID  = 1,
    /** Failure: Device was not in test mode  */
    TSAP_RADIO_CHANNEL_REJECTED = 2,
} tsap_radio_set_channel_result_e;

/** Return values for setting radio TX power confirmation
 *  WAPS_FUNC_TSAP_SET_RADIO_TX_POWER_CNF */
typedef enum
{
    /** Success: Radio power level set successfully  */
    TSAP_RADIO_TX_POWER_SUCCESS     = 0,
    /** Failure: Given power level is not valid */
    TSAP_RADIO_TX_POWER_INVALID     = 1,
    /** Failure: Device was not in test mode */
    TSAP_RADIO_TX_POWER_REJECTED    = 2,
} tsap_radio_set_tx_power_result_e;

/** Return values for sending data confirmation
 *  WAPS_FUNC_TSAP_SEND_DATA_CNF */
typedef enum
{
    /** Success: Date sent successfully */
    TSAP_RADIO_DATA_SEND_SUCCESS    = 0,
    /** Failure: Sending failed due to busy channel (CCA fail) */
    TSAP_RADIO_DATA_SEND_CCA_FAIL   = 1,
    /** Failure: Transmission failed, check parameters */
    TSAP_RADIO_DATA_SEND_FAILED     = 2,
    /** Failure: Device was not in test mode */
    TSAP_RADIO_DATA_SEND_REJECTED   = 3,
} tsap_radio_send_data_result_e;

/** Return values for receive data confirmation
 *  WAPS_FUNC_TSAP_RADIO_DATA_RX_CNF */
typedef enum
{
    /** Success: Reception enabled/disabled successfully */
    TSAP_RADIO_RECEPTION_SET_SUCCESS    = 0,
    /** Failure: Device was not in test mode */
    TSAP_RADIO_RECEPTION_SET_REJECTED   = 1,
}tsap_radio_reception_set_result_e;

/** Return values for read data confirmation
 *  WAPS_FUNC_TSAP_RADIO_DATA_READ_CNF */
typedef enum
{
    /** Success: Test data read from the buffer */
    TSAP_RADIO_DATA_READ_SUCCESS    = 0,
    /** Failure: No received test data in buffer */
    TSAP_RADIO_DATA_READ_NO_DATA    = 1,
    /* Failure: Device was not in test mode */
    TSAP_RADIO_READ_DATA_REJECTED   = 2
}tsap_radio_read_data_result_e;

typedef enum
{
    /** Transmits random symbols */
    TSAP_RADIO_TEST_SIGNAL_RANDOM       = 0,
    /** Transmits all symbols (0 -255) in the signaling alphabet 0-255,
     *  ref. FCC 558074 D01 15.247 Meas Guidance v05.*/
    TSAP_RADIO_TEST_SIGNAL_ALL_SYMBOLS  = 1,
}tsap_radio_test_signal_type_e;

/**  Frame structure for WAPS_FUNC_TSAP_SET_RADIO_CHANNEL_REQ */
typedef struct __attribute__ ((__packed__))
{
    uint8_t    channel;
} tsap_radio_channel_req_t;

/** Frame structure for WAPS_FUNC_TSAP_MODE_ENTER_REQ */
typedef struct __attribute__ ((__packed__))
{
    /** Radio network address to be used in data transmission */
    uint32_t    network_address;
} tsap_set_test_mode_req_t;

/**  Frame structure for WAPS_FUNC_TSAP_SET_RADIO_TX_POWER_REQ */
typedef struct __attribute__ ((__packed__))
{
    int8_t    txpower;
} tsap_radio_tx_power_req_t;


/** Frame structures for WAPS_FUNC_TSAP_SEND_DATA_REQ */
typedef struct __attribute__ ((__packed__))
{
    /* Sequence number of the message  */
    uint32_t    seq;
    /* Length of the test data in payload  */
    uint8_t     len;
}tsap_test_data_header_t;

typedef struct __attribute__ ((__packed__))
{
    /** Number of data bursts to be sent.
     *  Note! Data transmission is atomic operation in test mode.
     *  Bigger the bursts value is longer the transmission takes and delays
     *  response to caller.  **/
    uint32_t    bursts;
    /* CCA duration in ms. If 0, no CCA done */
    uint32_t    ccaDuration;
    /** When CCA is not used (ccaDuration=0) transmit interval defines the delay between
     *  TX bursts in us. If 0, sending done as soon as possible. */
    uint32_t    txInterval;
}tsap_radio_transmitter_control_t;

typedef struct __attribute__ ((__packed__))
{
    tsap_radio_transmitter_control_t txCtrl;
    /* Header of received test data */
    tsap_test_data_header_t hdr;
    /** Test data to be transmitted */
    uint8_t     data[APDU_TSAP_MAX_SIZE];
} tsap_radio_tx_data_req_t;


typedef struct __attribute__ ((__packed__))
{
    tsap_radio_transmitter_control_t txCtrl;
    /** Test signal type to be trasmitted */
    tsap_radio_test_signal_type_e  signalType;
} tsap_radio_tx_test_signal_req_t;

typedef struct __attribute__ ((__packed__))
{
    /* Number of received messages since test mode started.
     * Can be used e.g. for PER% calculation together with rxSeq */
    uint32_t    cntr;
    /* Number of duplicate messages.
     * Incremented if received message has the same sequence number than
     * previous test data package. */
    uint32_t    dup;
}tsap_test_data_counters_t;

typedef struct __attribute__ ((__packed__))
{
    /** Received signal strength in dBm with resolution of 1dBm. */
    int8_t rssi;
    /** Received data counters. */
    tsap_test_data_counters_t rxCntrs;
    /* Header of received test data */
    tsap_test_data_header_t hdr;
    /** Received test data */
    uint8_t     data[APDU_TSAP_MAX_SIZE];
}tsap_radio_rx_data_t;

/** Frame structure for WAPS_FUNC_TSAP_TEST_DATA_RX_IND */
typedef struct __attribute__ ((__packed__))
{
    uint8_t   queued_indications;
    tsap_radio_rx_data_t rx;
}tsap_radio_data_rx_ind_t;

#define FRAME_TSAP_TEST_DATA_RX_IND_HEADER_SIZE \
        (sizeof(tsap_radio_data_rx_ind_t) - APDU_TSAP_MAX_SIZE)

/** Frame structure for WAPS_FUNC_TSAP_RADIO_DATA_READ_CNF */
typedef struct __attribute__ ((__packed__))
{
    tsap_radio_read_data_result_e result;
    tsap_radio_rx_data_t rx;
} tsap_radio_data_read_cnf_t;

/** Frame structure for WAPS_FUNC_TSAP_RECEIVE_DATA_REQ */
typedef struct __attribute__ ((__packed__))
{
    bool    rxEnable;
    bool    dataIndiEnable;
} tsap_radio_receive_req_t;

/** Frame structure for WAPS_FUNC_TSAP_RADIO_GET_MAX_DATA_SIZE_CNF */
typedef struct __attribute__ ((__packed__))
{
    tsap_radio_max_data_size_result_e result;
    uint8_t size;
} tsap_radio_max_data_size_cnf_t;

/** Frame structure for WAPS_FUNC_TSAP_RADIO_DATA_TX_CNF */
typedef struct __attribute__ ((__packed__))
{
    tsap_radio_send_data_result_e result;
    uint32_t sentBursts;
} tsap_radio_send_data_cnf_t;

typedef union
{
    tsap_set_test_mode_req_t    testModeSet_req;
    tsap_radio_channel_req_t    radioChannel_req;
    tsap_radio_tx_power_req_t   radioTXpower_req;
    tsap_radio_tx_data_req_t    radioSendData_req;
    tsap_radio_receive_req_t    radioReceive_req;
    tsap_radio_data_rx_ind_t    radioRXdata_ind;
    tsap_radio_send_data_cnf_t  radioSendData_cnf;
    tsap_radio_data_read_cnf_t  radioDataRead_cnf;
    tsap_radio_max_data_size_cnf_t radioMaxDataSize_cnf;
    tsap_radio_tx_test_signal_req_t radioSendTestSignal_req;
} frame_tsap;

#endif /* TSAP_FRAMES_H_ */
// TEST_REG_SDK_ONLY_END
