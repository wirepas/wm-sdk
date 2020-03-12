// TEST_REG_SDK_ONLY_BEGIN
/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#ifdef TEST_LIB_SUPPORT

#include "tsap.h"

#include "function_codes.h"
#include "lock_bits.h"
#include "waps/waddr.h"
#include "waps_frames.h"
#include "waps_private.h"
#include "waps/protocol/waps_protocol.h"
#include "api.h"

static bool test_mode_enter(waps_item_t * item);
static bool test_mode_exit(waps_item_t * item);
static bool set_radio_channel(waps_item_t * item);
static bool set_radio_txpower(waps_item_t * item);
static bool send_radio_data(waps_item_t * item);
static bool allow_radio_reception(waps_item_t * item);
static bool read_radio_data(waps_item_t * item);
static bool get_radio_max_data_size(waps_item_t * item);
static bool send_radio_test_signal_tx(waps_item_t * item);

bool Tsap_handleFrame(waps_item_t * item)
{
    bool status = false;

    if (lib_test != NULL)
    {
        switch (item->frame.sfunc)
        {
            case    WAPS_FUNC_TSAP_TEST_MODE_ENTER_REQ:
            {
                status = test_mode_enter(item);
            }
            break;
            case    WAPS_FUNC_TSAP_TEST_MODE_EXIT_REQ:
            {
                status = test_mode_exit(item);
            }
            break;
            case    WAPS_FUNC_TSAP_RADIO_CHANNEL_REQ:
            {
                status = set_radio_channel(item);
            }
            break;
            case    WAPS_FUNC_TSAP_RADIO_TX_POWER_REQ:
            {
                status = set_radio_txpower(item);
            }
            break;
            case    WAPS_FUNC_TSAP_RADIO_DATA_TX_REQ:
            {
                status = send_radio_data(item);
            }
            break;
            case    WAPS_FUNC_TSAP_RADIO_DATA_RX_REQ:
            {
                status = allow_radio_reception(item);
            }
            break;
            case    WAPS_FUNC_TSAP_RADIO_DATA_READ_REQ:
            {
                status = read_radio_data(item);
            }
            break;
            case    WAPS_FUNC_TSAP_RADIO_GET_MAX_DATA_SIZE_REQ:
            {
                status = get_radio_max_data_size(item);
            }
            break;
            case WAPS_FUNC_TSAP_RADIO_TEST_SIGNAL_TX_REQ:
            {
                status = send_radio_test_signal_tx(item);
            }
            default:
            {

            }
        }
    }

    return status;
}

static bool test_mode_enter(waps_item_t * item)
{
    tsap_enter_result_e result  = TSAP_ENTER_TEST_MODE_REJECTED;
    waps_func_e conf            = WAPS_FUNC_TSAP_TEST_MODE_ENTER_CNF;
    bool status                 = true;
    frame_tsap tsap             = item->frame.tsap;
    app_lib_test_mode_enter_res_e retvalue;

    /* Check payload length */
    if (item->frame.splen == sizeof (tsap_set_test_mode_req_t))
    {
        retvalue = lib_test->setTestMode(tsap.testModeSet_req.network_address);
        if (retvalue ==  APP_LIB_TEST_SET_MODE_SUCCESS)
        {
            result = TSAP_ENTER_TEST_MODE_SUCCESS;
        }
        if (retvalue == APP_LIB_TEST_SET_MODE_NWK_ADDRESS_INVALID)
        {
            result = TSAP_ENTER_TEST_MODE_NWK_ADDRESS_INVALID;
        }
    }
    else
    {
        status = false;
    }
    /* Build response */
    Waps_item_init(item, conf, sizeof(simple_cnf_t));
    item->frame.simple_cnf.result = result;

    return status;
}
static void reboot_callback(waps_item_t * item)
{
    (void)item;
    /* Wait for uart to be empty */
    Waps_prot_flush_hw();
    /* Reset */
    (void)lib_state->stopStack();
}

static bool test_mode_exit(waps_item_t * item)
{
    tsap_exit_result_e result   = TSAP_EXIT_TEST_MODE_REJECTED;
    waps_func_e conf            = WAPS_FUNC_TSAP_TEST_MODE_EXIT_CNF;
    bool status                 = true;

    /* Check payload length, no parameters passed -> length should be 0 */
    if (item->frame.splen == 0)
    {
        if (lib_test->exitTestMode() == APP_LIB_TEST_EXIT_MODE_SUCCESS)
        {
            result = TSAP_EXIT_TEST_MODE_SUCCESS;
        }
    }
    else
    {
        status = false;
    }
    /* Build response */
    Waps_item_init(item, conf, sizeof(simple_cnf_t));
    item->frame.simple_cnf.result = result;

    if (result == TSAP_EXIT_TEST_MODE_SUCCESS)
    {
        /* Success, reboot even if stack was already stopped */
        item->post_cb = reboot_callback;
    }
    return status;
}

static bool set_radio_channel(waps_item_t * item)
{
    tsap_radio_set_channel_result_e result = TSAP_RADIO_CHANNEL_REJECTED;
    waps_func_e conf                       = WAPS_FUNC_TSAP_RADIO_CHANNEL_CNF;
    bool status                            = true;
    frame_tsap tsap                        = item->frame.tsap;
    app_lib_test_set_radio_channel_res_e retvalue;

    /* Check payload length */
    if (item->frame.splen == sizeof(tsap_radio_channel_req_t) )
    {
        retvalue = lib_test->setRadioChannel(tsap.radioChannel_req.channel);

        if (retvalue == APP_LIB_TEST_RADIO_CHANNEL_SUCCESS)
        {
            result = TSAP_RADIO_CHANNEL_SUCCESS;
        }
        if (retvalue == APP_LIB_TEST_RADIO_CHANNEL_INVALID)
        {
            result = TSAP_RADIO_CHANNEL_INVALID;
        }
    }
    else
    {
        status = false;
    }

    /* Build response */
    Waps_item_init(item, conf, sizeof(simple_cnf_t));
    item->frame.simple_cnf.result = result;

    return status;
}

static bool set_radio_txpower(waps_item_t * item)
{
    tsap_radio_set_tx_power_result_e result = TSAP_RADIO_TX_POWER_REJECTED;
    waps_func_e conf                        = WAPS_FUNC_TSAP_RADIO_TX_POWER_CNF;
    bool status                             = true;
    frame_tsap tsap                         = item->frame.tsap;
    app_lib_test_set_radio_power_level_res_e retvalue;
    /* Check payload length */
    if (item->frame.splen == sizeof(tsap_radio_tx_power_req_t) )
    {
        retvalue = lib_test->setRadioTXPowerLevel(tsap.radioTXpower_req.txpower);
        if (retvalue == APP_LIB_TEST_RADIO_POWER_SUCCESS)
        {
            result = TSAP_RADIO_TX_POWER_SUCCESS;
        }
        if (retvalue == APP_LIB_TEST_RADIO_POWER_INVALID)
        {
            result = TSAP_RADIO_TX_POWER_INVALID;
        }
    }
    else
    {
        status = false;
    }

    /* Build response */
    Waps_item_init(item, conf, sizeof(simple_cnf_t));
    item->frame.simple_cnf.result = result;

    return status;
}

static bool send_radio_data(waps_item_t * item)
{
    tsap_radio_send_data_result_e result= TSAP_RADIO_DATA_SEND_REJECTED;
    waps_func_e conf                    = WAPS_FUNC_TSAP_RADIO_DATA_TX_CNF;
    bool status                         = true;
    frame_tsap tsap                     = item->frame.tsap;
    uint32_t sentBursts                 = 0;

    uint8_t minLen= (sizeof(tsap.radioSendData_req.txCtrl) +
                     sizeof(tsap.radioSendData_req.hdr.len) +
                     sizeof(tsap.radioSendData_req.hdr.seq));

    /* Check payload length */
    if (item->frame.splen >= minLen &&
        item->frame.splen <= minLen+tsap.radioSendData_req.hdr.len)
    {
        /* Create request for test-library */
       app_lib_test_data_transmit_t txLibData;

       txLibData.txCtrl.bursts = tsap.radioSendData_req.txCtrl.bursts;
       txLibData.txCtrl.ccaDuration = tsap.radioSendData_req.txCtrl.ccaDuration;
       txLibData.txCtrl.txInterval = tsap.radioSendData_req.txCtrl.txInterval;
       txLibData.txPayload.hdr.seq = tsap.radioSendData_req.hdr.seq;
       txLibData.txPayload.hdr.len = tsap.radioSendData_req.hdr.len;
       txLibData.txPayload.data = tsap.radioSendData_req.data;

       app_lib_test_send_radio_data_res_e status;

       status = lib_test->sendRadioData(&txLibData, &sentBursts);

       if (status == APP_LIB_TEST_RADIO_SEND_SUCCESS)
       {
           result = TSAP_RADIO_DATA_SEND_SUCCESS;
       }
       if (status == APP_LIB_TEST_RADIO_SEND_CCA_ERROR)
       {
           result = TSAP_RADIO_DATA_SEND_CCA_FAIL;
       }
       if (status == APP_LIB_TEST_SEND_FAILED)
       {
           result = TSAP_RADIO_DATA_SEND_FAILED;
       }
    }
    else
    {
       status = false;
    }

    /* Build response */
    Waps_item_init(item, conf, sizeof(tsap_radio_send_data_cnf_t));
    item->frame.tsap.radioSendData_cnf.result = result;
    item->frame.tsap.radioSendData_cnf.sentBursts = sentBursts;

    return status;
}

static bool allow_radio_reception(waps_item_t * item)
{
    tsap_radio_reception_set_result_e result= TSAP_RADIO_RECEPTION_SET_REJECTED;
    waps_func_e conf                        = WAPS_FUNC_TSAP_RADIO_DATA_RX_CNF;
    bool status                             = true;
    frame_tsap tsap                         = item->frame.tsap;

    /* Check payload length */
    if (item->frame.splen == sizeof(tsap_radio_receive_req_t))
    {
       if (lib_test->allowRadioReception(tsap.radioReceive_req.rxEnable,
                                        tsap.radioReceive_req.dataIndiEnable) ==
                                    APP_LIB_TEST_RADIO_RECEPTION_SET_SUCCESS)
       {
           result = TSAP_RADIO_RECEPTION_SET_SUCCESS;
       }
    }
    else
    {
       status = false;
    }

    /* Build response */
    Waps_item_init(item, conf, sizeof(simple_cnf_t));
    item->frame.simple_cnf.result = result;

    return status;
}

static bool read_radio_data(waps_item_t * item)
{
    tsap_radio_read_data_result_e result  = TSAP_RADIO_READ_DATA_REJECTED;
    waps_func_e conf                      = WAPS_FUNC_TSAP_RADIO_DATA_READ_CNF;
    bool status                           = true;
    uint8_t testDataLen                   = 0;
    tsap_radio_rx_data_t * cnf         = &item->frame.tsap.radioDataRead_cnf.rx;
    app_lib_test_set_radio_read_data_res_e retval;
    if (item->frame.splen == 0 )
    {
        app_lib_test_data_received_t rxData;

        retval = lib_test->readRadioData(&rxData);
        if (retval == APP_LIB_TEST_RADIO_READ_DATA_SUCCESS)
        {
             testDataLen = rxData.hdr.len + sizeof(rxData.hdr.seq)
                                          + sizeof(rxData.hdr.len)
                                          + sizeof(rxData.rxCntrs.cntr)
                                          + sizeof(rxData.rxCntrs.dup)
                                          + sizeof(rxData.rssi);

             cnf->rssi = rxData.rssi;
             cnf->rxCntrs.cntr = rxData.rxCntrs.cntr;
             cnf->hdr.seq = rxData.hdr.seq;
             cnf->rxCntrs.dup = rxData.rxCntrs.dup;
             cnf->hdr.len = rxData.hdr.len;
             memcpy(cnf->data,rxData.data, rxData.hdr.len);

             result = TSAP_RADIO_DATA_READ_SUCCESS;
        }
        else
        {
            /** There is no test data, so send header and one dummy data byte in
             *  playload to avoid parser crash in regression test environment */
            uint8_t HdrSize = sizeof(cnf->rxCntrs.cntr) + sizeof(cnf->hdr.seq) +
                              sizeof(cnf->rxCntrs.dup) + sizeof(cnf->hdr.len) +
                              sizeof(cnf->rssi);
            memset(cnf, 0x00, HdrSize);
            cnf->hdr.len = 1;
            cnf->data[0] = 0;
            testDataLen = cnf->hdr.len + HdrSize;

            if (retval == APP_LIB_TEST_RADIO_READ_NO_DATA)
            {
              result = TSAP_RADIO_DATA_READ_NO_DATA;
            }
        }

    }
    else
    {
        status = false;

    }

    /* Build response */
    Waps_item_init(item, conf, testDataLen);

    item->frame.tsap.radioDataRead_cnf.result = result;
    item->frame.splen = sizeof(item->frame.tsap.radioDataRead_cnf.result) +
                        testDataLen;

    return status;
}

static bool get_radio_max_data_size(waps_item_t * item)
{
    tsap_radio_max_data_size_result_e
               result = TSAP_RADIO_DATA_SIZE_READ_REJECTED;
    waps_func_e conf  =   WAPS_FUNC_TSAP_RADIO_GET_MAX_DATA_SIZE_CNF;
    bool status       =   true;

    uint8_t size;
    if (lib_test->getMaxDataSize(&size) == APP_LIB_TEST_DATA_SIZE_READ_SUCCESS)
    {
        result = TSAP_RADIO_DATA_SIZE_READ_SUCCESS;
    }

    /* Build response */
    Waps_item_init(item, conf, sizeof(tsap_radio_max_data_size_cnf_t));
    item->frame.tsap.radioMaxDataSize_cnf.result = result;
    item->frame.tsap.radioMaxDataSize_cnf.size = size;

    return status;
}

static bool send_radio_test_signal_tx(waps_item_t * item)
{
    tsap_radio_send_data_result_e result= TSAP_RADIO_DATA_SEND_REJECTED;
    waps_func_e conf                  = WAPS_FUNC_TSAP_RADIO_TEST_SIGNAL_TX_CNF;
    bool status                       = true;
    frame_tsap tsap                   = item->frame.tsap;
    uint32_t sentBursts               = 0;

    uint8_t minLen= (sizeof(tsap.radioSendTestSignal_req.txCtrl) +
                     sizeof(tsap.radioSendTestSignal_req.signalType));

    /* Check payload length */
    if (item->frame.splen == minLen)
    {
        /* Create request for test-library */
       app_lib_test_signal_transmit_t txLibData;
       txLibData.signalType = tsap.radioSendTestSignal_req.signalType;
       txLibData.txCtrl.bursts = tsap.radioSendTestSignal_req.txCtrl.bursts;
       txLibData.txCtrl.ccaDuration = tsap.radioSendData_req.txCtrl.ccaDuration;
       txLibData.txCtrl.txInterval = tsap.radioSendData_req.txCtrl.txInterval;

       app_lib_test_send_radio_data_res_e status;

       status = lib_test->sendRadioTestSignal(&txLibData, &sentBursts);

       if (status == APP_LIB_TEST_RADIO_SEND_SUCCESS)
       {
           result = TSAP_RADIO_DATA_SEND_SUCCESS;
       }
       if (status == APP_LIB_TEST_RADIO_SEND_CCA_ERROR)
       {
           result = TSAP_RADIO_DATA_SEND_CCA_FAIL;
       }
       if (status == APP_LIB_TEST_SEND_FAILED)
       {
           result = TSAP_RADIO_DATA_SEND_FAILED;
       }
    }
    else
    {
       status = false;
    }

    /* Build response */
    Waps_item_init(item, conf, sizeof(tsap_radio_send_data_cnf_t));
    item->frame.tsap.radioSendData_cnf.result = result;
    item->frame.tsap.radioSendData_cnf.sentBursts = sentBursts;

    return status;
}

void Tsap_testPacketReceived(const uint8_t * bytes,
                                   size_t size,
                                   uint32_t seq,
                                   uint32_t duplicates,
                                   uint32_t rxCntr,
                                   int8_t rssi,
                                   waps_item_t * item)
{
    // Create indication
    Waps_item_init(item,
                   WAPS_FUNC_TSAP_RADIO_DATA_RX_IND,
                   FRAME_TSAP_TEST_DATA_RX_IND_HEADER_SIZE + size);

    // Populate indication frame with received data
    tsap_radio_data_rx_ind_t * ind_ptr = &item->frame.tsap.radioRXdata_ind;
    item->time               = 0;
    ind_ptr->rx.rssi         = rssi;
    ind_ptr->rx.rxCntrs.cntr = rxCntr;
    ind_ptr->rx.hdr.seq      = seq;
    ind_ptr->rx.rxCntrs.dup  = duplicates;
    ind_ptr->rx.hdr.len      = size;

    memcpy(ind_ptr->rx.data, bytes, size);

}
#endif /* TEST_LIB_SUPPORT */
// TEST_REG_SDK_ONLY_END
