/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    app.c
 * \brief   App for sending directed-advertiser packets
 */

#include <string.h>
#include "api.h"
#include "node_configuration.h"
#include "time.h"
#include "app_scheduler.h"
#include "random.h"
#include "power.h"
#include "../inventory_app_router/common.h"

#ifndef EFR32MG22
#define USE_UICR_CONFIG
#endif

/** Role settings for Advertiser node !!! DO NOT CHANGE */
#define NODE_DEFAULT_ROLE \
    app_lib_settings_create_role(APP_LIB_SETTINGS_ROLE_ADVERTISER,0)


#define MAX_BEACONS 1
#define MAX_SCAN_RETRY 4    // max. number of scans in one iteration
#define MAX_SCAN_FAILURES (2 * MAX_SCAN_RETRY) // max. number of failed scans
#define MAX_SEND_RETRY 2 // max. number of send retires in one iteration

// Period to send advertisement
#define ADV_PERIOD_MS   (ADVERTISER_RATE_SEC * 1000)
#define MAX_SCAN_TIME_MS (NETWORK_BEACON_RATE_US / 1000)

//Timeouts
#define TIMEOUT_SCAN_MS 400
#define TIMEOUT_SCAN_RETRY_MS 50
#define TIMEOUT_SCAN_END_MS 2000
#define TIMEOUT_SEND_DATA_MS 2000
#define TIMEOUT_SEND_CONFIRM_MS 6000

// Source endpoint to send advertiser data
#define DIRADV_EP_SRC_DATA 248

//#define FORCE_NETWORK_CONFIG

typedef enum{
    IDLE = 0,
    SCAN = 1,
    SCAN_END = 2,
    SEND_DATA = 3,
    SEND_CONFIRM = 4,
    ACK = 5,
    MAX_STATE = 6
} state_res_e;

static uint32_t advertiser_task(void);
static app_lib_data_send_res_e res __attribute__((unused));

/* Advertiser m_state data */
typedef struct
{
    bool done;
    bool success;
    uint32_t timeout_ms;
    uint32_t start_hp;
    uint32_t end_hp;
} adv_tag_state_data_t;

/* Advertiser task internal data */
typedef struct
{
    uint8_t run;
    uint8_t current;
    uint8_t scan_failures;
    uint8_t scan_count;
    uint8_t send_count;
    uint32_t startup_ms;
    uint8_t tracking_id;
    uint16_t seq;
    adv_tag_state_data_t data[MAX_STATE];
} adv_tag_state_t;

/* Network beacon data */
typedef struct{
    uint8_t requested;
    uint8_t count;
    uint8_t target;
    uint8_t retry;
    uint32_t time_hp[MAX_BEACONS];
    app_lib_state_beacon_rx_t data[MAX_BEACONS];
} beacon_t;

/* Advertiser node settings */
typedef struct{
    uint32_t period_ms;
    uint8_t otap_seq;
    uint8_t max_beacons;
    uint32_t max_scan_time_ms;
    uint32_t period_phase_ms;
} adv_tag_settings_t;


// Global variables

static adv_tag_state_t m_state;
static beacon_t m_beacons;
static uint8_t m_ack_data[sizeof(adv_tag_app_cfg_t)];

static adv_tag_settings_t m_settings = {
    .period_ms = ADV_PERIOD_MS,
    .otap_seq = 0,
    .max_beacons = MAX_BEACONS,
    .max_scan_time_ms = MAX_SCAN_TIME_MS,
    .period_phase_ms = 0
};

static app_lib_data_to_send_t m_adv_to_send =
{
    .dest_address = APP_ADDR_ANYSINK,
    .src_endpoint = DIRADV_EP_SRC_DATA,
    .dest_endpoint = DIRADV_EP_DEST,
    .flags = 1,
    .tracking_id = APP_LIB_DATA_NO_TRACKING_ID,
};

/**
 *  @brief Callback for data send
 *  @param[in] status
 *             Sent status
 *  @param[out]  void
 *
**/
static void sentDataCb(const app_lib_data_sent_status_t * status)
{
  if (status->tracking_id == m_state.tracking_id)
  {
    m_state.data[SEND_CONFIRM].done = true;
    m_state.data[SEND_CONFIRM].success = status->success;
    // notify the main task
    App_Scheduler_addTask(advertiser_task, APP_SCHEDULER_SCHEDULE_ASAP);
  }
}


/**
 *  @brief      Data receive callback
 *  @param[in]  data
 *              Received data
 *  @param[out]
*                processing code
**/
static app_lib_data_receive_res_e dataReceivedCb(
    const app_lib_data_received_t * data)
{
    uint8_t len = sizeof(m_ack_data);

    /*!!! add ckeck for source, EP*/
    if (len > data->num_bytes)
    {
        memset(m_ack_data, 0, len);
        len = data->num_bytes;
    }

    memcpy(m_ack_data, data->bytes, len);
    m_state.data[ACK].done = true;
    m_state.data[ACK].end_hp = (uint32_t) lib_time->getTimestampHp();

    App_Scheduler_addTask(advertiser_task, APP_SCHEDULER_SCHEDULE_ASAP);
    return APP_LIB_DATA_RECEIVE_RES_HANDLED;
}


/**
 *  @brief      Initialize state
 *  @param[in]  void
 *               Give the input value
 *  @param[out]  void
 *
**/
static void adv_init(void)
{
    memset(&m_state, 0, sizeof(m_state));
    memset(&m_beacons, 0, sizeof(m_beacons));
}

/**
 *  @brief      Check if the scratchpad needs to be processed
 *  @param[in]  void
 *
 *  @param[out] bool : true if scratchpad needs processing, false otherwise
 *
**/

static bool check_otap (void)
{
    if ( lib_otap->isValid() && \
        !lib_otap->isProcessed() && \
        lib_otap->getSeq() == m_settings.otap_seq && \
        lib_otap->getProcessedSeq() != m_settings.otap_seq )
        {
            lib_otap->setToBeProcessed();
            return true;
        }
    return false;
}

/**
 *  @brief      Prepare the state machine for the next state
 *  @param[in]  next
 *              The next state
 *  @param[in]  timeout_ms
 *              State timeout
 *  @param[out] true/false for success/failure setting the next state
 *
**/

static bool next_state(uint8_t next, uint32_t timeout_ms)
{
    adv_tag_state_data_t * state_data;

    state_data = &m_state.data[m_state.current];
    state_data->done = true;

    if (m_state.current != ACK)
    {
        state_data->end_hp = (uint32_t) lib_time->getTimestampHp();
    }

    if (next == IDLE)
    {

        /* Reset m_state to IDLE */
        memset(&m_state.data, 0, sizeof(m_state.data));
        memset(&m_beacons, 0, sizeof(m_beacons));
        m_state.current = IDLE;
        state_data = &m_state.data[m_state.current];
        state_data->start_hp = (uint32_t) lib_time->getTimestampHp();
        state_data->done = false;

        if (m_state.run > 0) /*Another iteration requested*/
        {
           state_data->timeout_ms = APP_SCHEDULER_SCHEDULE_ASAP;
        }
        else  /* Iteration completed */
        {
           state_data->timeout_ms = APP_SCHEDULER_STOP_TASK;
        }

        return true;
    }
    else if (next < MAX_STATE)
    {
        m_state.current = next;
        state_data = &m_state.data[m_state.current];
        state_data->start_hp = (uint32_t) lib_time->getTimestampHp();
        state_data->timeout_ms = timeout_ms;
        return true;
    }
    else
    {
        return false;
    }
}

/**
 *  @brief      Checks if the current state is completed
 *  @param[in]  void
 *
 *  @param[out] boolean
 *              Return curent state completion status
**/

static bool is_state_done(void)
{
    return  m_state.data[m_state.current].done;
}

/**
 *  @brief      Provide current state timeout value
 *  @param[in]  void
 *
 *  @param[out]
 *              Return state timeout value
**/

static uint32_t state_timeout(void)
{
    return m_state.data[m_state.current].timeout_ms;
}

/**
 *  @brief      Decodes ACK data and stores is in m_settings
 *  @param[in]  data
 *              Input data pointer
 *  @param[out]  void
 *
**/

static void decode_ack(uint8_t * data)
{
    adv_tag_app_cfg_t * app_cfg = (adv_tag_app_cfg_t *) data;

    // update period; value 0 is invalid and ignored
    if (app_cfg->period != 0)
     {
        uint32_t period_ms = app_cfg->period * 1000;

        if (period_ms != m_settings.period_ms)
        {
            m_settings.period_phase_ms = Random_jitter(period_ms);
        }
        m_settings.period_ms = period_ms;
    }

    //OTAP sequence
    m_settings.otap_seq = app_cfg->otap_seq;

    // Maximum scan time; value 0 is invalid and ignored
    if( app_cfg->max_scan_time != 0 )
    {
        m_settings.max_scan_time_ms = app_cfg->max_scan_time * 10;
    }

}

/**
 *  @brief      Triggers a network re-scan
 *  @param[in]  void
 *
 *  @param[out]  timeout
 *               Returns the SCAN state timeout
**/

static uint32_t rescan(void)
{
    m_beacons.retry++;
    m_beacons.count = 0;
    next_state(SCAN, TIMEOUT_SCAN_RETRY_MS);
    memset(&m_state.data[SCAN_END], 0, sizeof(m_state.data[SCAN_END]));
    memset(&m_state.data[SEND_DATA], 0, sizeof(m_state.data[SEND_DATA]));
    memset(&m_state.data[SEND_CONFIRM], 0, sizeof(m_state.data[SEND_CONFIRM]));
    memset(&m_state.data[ACK], 0, sizeof(m_state.data[ACK]));
    return state_timeout();
}

/**
 *  @brief      Main advertiser task
 *  @param[in]  void
 *
 *  @param[out]  timeout
 *               Return the state timeout
**/

static uint32_t advertiser_task(void)
{

    uint32_t delta_ms;
    bool timeout = false;

    delta_ms =  lib_time->getTimeDiffUs(lib_time->getTimestampHp(), \
                            m_state.data[m_state.current].start_hp ) / 1000;
    timeout = delta_ms + 20 >= state_timeout();
    if (delta_ms > 2*m_settings.period_ms)
    {
        delta_ms = m_settings.period_ms / 2;
    }


   switch(m_state.current)
   {
       case IDLE:

            /* From IDLE -> SCAN after timeout */
            m_state.run = 0;
            uint32_t scan_delay = Random_jitter(SCAN_RAND_MS) + 50;
            m_state.data[IDLE].done = true;
            m_state.seq++;
            next_state(SCAN, scan_delay);


            return state_timeout();
            break;

       case SCAN:
            lib_state->setScanDuration(m_settings.max_scan_time_ms*1000);
            lib_state->startScanNbors();
            next_state(SCAN_END, TIMEOUT_SCAN_END_MS);
            m_state.scan_count = m_state.scan_count < 255 ? (m_state.scan_count + 1) : 255;
            return state_timeout();
            break;

       case SCAN_END:
            /* Waiting for scan completion */
            if(is_state_done() || timeout )
            {
                bool success = false;
                app_lib_state_beacon_rx_t * bp;


                for (int i = 0; i < m_beacons.count; i++)
                {
                    bp = &m_beacons.data[i];
                    if (bp->is_sink)
                    {
                    }
                    else
                    {
                        success = true;
                        m_beacons.target = i;
                        break;
                    }
                }

                m_state.scan_failures = (m_beacons.count > 0 && success) ? 0 : (m_state.scan_failures + 1);

                if (success)
                {
                    next_state(SEND_DATA, Random_jitter(SEND_RAND_MS));
                    m_state.scan_failures = 0;
                    return state_timeout();
                }
                else
                {
                    // scan failure is incremented when no beacons
                    // found | only sinks available
                    if (m_state.scan_failures < MAX_SCAN_FAILURES)
                    {
                          m_state.scan_failures++;
                    }
                    if(m_beacons.retry < MAX_SCAN_RETRY
                        && m_state.scan_failures < MAX_SCAN_FAILURES)
                    {
                        return rescan();
                    }
                }
            }
            timeout = true;  // scan failed; we go to idle
            break;

       case SEND_DATA:

        if (m_beacons.count > 0)
            {
                app_lib_state_beacon_rx_t * bp =
                    &m_beacons.data[m_beacons.target];
                app_lib_data_send_res_e res;
                adv_tag_data_t data;

                m_state.send_count = m_state.send_count < 255 ? (m_state.send_count + 1) : 255;
                m_state.tracking_id++;
                data.type = ADV_TYPE0;
                data.rssi = bp->rssi;
                data.seq = m_state.seq;
                data.voltage = lib_hw->readSupplyVoltage();
                data.proc_otap_seq = (uint8_t) lib_otap->getProcessedSeq();
                data.stored_otap_seq = (uint8_t) lib_otap->getSeq();
                data.scan_count = m_state.scan_count;
                data.send_count = m_state.send_count;

                m_adv_to_send.bytes = (uint8_t*) &data;
                m_adv_to_send.dest_address = bp->address;
                m_adv_to_send.num_bytes = sizeof(data);
                m_adv_to_send.tracking_id = (app_lib_data_tracking_id_t) m_state.tracking_id;

                res = lib_data->sendData(&m_adv_to_send);

                if (res == APP_LIB_DATA_SEND_RES_SUCCESS)
                {
                    next_state(SEND_CONFIRM, TIMEOUT_SEND_CONFIRM_MS);
                    return state_timeout();
                }
                else if (m_beacons.retry < MAX_SCAN_RETRY &&
                        m_state.scan_failures < MAX_SCAN_FAILURES &&
                        m_state.send_count < MAX_SEND_RETRY)
                {
                    return rescan();
                }

            }

            /*We did not found any m_beacons || data sent failed -> force a timeout and move to IDLE */
            timeout = true;
            break;

       case SEND_CONFIRM:

             if(is_state_done())
             {
                if (m_state.data[SEND_CONFIRM].success  &&
                    m_state.data[ACK].done)   /*if ACK is not received yet we wait for ACK*/
                {
                    decode_ack(m_ack_data);
                    next_state(IDLE, 0);
                    m_state.scan_count = 0;
                    m_state.send_count = 0;
                    return state_timeout();
                }
                else if(!m_state.data[SEND_CONFIRM].success &&    /* send failure will trigger a retry*/
                        m_beacons.retry < MAX_SCAN_RETRY &&
                        m_state.scan_failures < MAX_SCAN_FAILURES &&
                        m_state.send_count < MAX_SEND_RETRY)
                {
                    return rescan();
                }
             }

            if (!timeout)
             {
                return delta_ms;
             }

             break;
       default:
          /* Is and error if we reached this state -> force a timeout and move to IDLE */
          timeout = true;
          break;
   }

   if (timeout)
   {
        next_state(IDLE, 0);
        return state_timeout();
   }
   else
   {
        return delta_ms;
   }
}

/**
 *  @brief      Network beacon receive callback;  beacon data appended to m_beacons
 *  @param[in]  msg
 *               Beacon message
 *  @param[out] void
 *
**/

static void beaconReceivedCb(const app_lib_state_beacon_rx_t * beacon)
{
    uint8_t idx = m_beacons.count;

    if(idx >= MAX_BEACONS || beacon-> is_sink || !beacon->is_da_support)
    {
     return;  //buffer is full or we found a sink
    }

    memcpy(&m_beacons.data[idx], beacon, sizeof(app_lib_state_beacon_rx_t));
    m_beacons.time_hp[idx] = (uint32_t) lib_time->getTimestampHp();
    m_beacons.count++;
    if (m_beacons.count >= m_settings.max_beacons)
    {
        lib_state->stopScanNbors();
    }
}

/**
 *  @brief      Scan end callback; triggers main task
 *  @param[in]  void
 *
 *  @param[out] void
 *
**/

static void scanEndCb(void)
{
    m_state.data[SCAN_END].done = true;
    App_Scheduler_addTask(advertiser_task, APP_SCHEDULER_SCHEDULE_ASAP);
}

/**
 *  @brief      Main task; triggers the advertiser task and checks for OTAP
 *  @param[in]  void
 *
 *  @param[out] time
 *              Returns the delay to next run
**/

static uint32_t main_task(void)
{
    if (m_state.run < 255)
    {
        m_state.run++;
    }

    if (m_state.current == IDLE)
    {
      App_Scheduler_addTask(advertiser_task, APP_SCHEDULER_SCHEDULE_ASAP);
    }
    else if (m_state.run > 10)  /*advertiser task run missed too many times*/
    {
       App_Scheduler_cancelTask(advertiser_task);
       next_state(IDLE, 0);  //task will be started automatically next time
    }

    /*OTAP */
    if (check_otap())
    {
        lib_state->stopStack();
    }

    if (m_settings.period_phase_ms == 0)
    {
        return m_settings.period_ms;
    }
    else
    {
        /*Re-adjust the phase when update period changed*/
        uint32_t next_run_ms = m_settings.period_ms + m_settings.period_phase_ms;
        m_settings.period_phase_ms = 0;
        return next_run_ms;
    }
}

/**
 *  @brief      Initialize callbacks
 *  @param[in]  void
 *
 *  @param[out] void
 *
**/

static void callbacks_init(void)
{
    // Set periodic callback for sending advertisement/bcast messages
    lib_state->setOnBeaconCb(beaconReceivedCb);
    lib_state->setOnScanNborsCb(scanEndCb,
                                APP_LIB_STATE_SCAN_NBORS_ONLY_REQUESTED);
    lib_data->setDataReceivedCb(dataReceivedCb);
    lib_data->setDataSentCb(sentDataCb);
}

/**
 *  @brief      Node initialization
 *  @param[in]  void
 *
 *  @param[out]  bool
 *               Return true if stack can be started, false if reboot required
**/

static bool node_init(void)
{
   bool ret = true;
#ifdef USE_UICR_CONFIG
   app_addr_t node_addr = getUniqueAddress();
    /*
        UICR register are read
    */

    if (lib_settings->getNodeAddress(&node_addr) != APP_RES_OK)
    {
        // Node ID : CUSTOMER[0]
        node_addr = (app_addr_t) (NRF_UICR->CUSTOMER[0] & 0xFFFFFF);
        if (node_addr == 0xFFFFFF)
        {
            node_addr = getUniqueAddress();
        }
        // Network ID : CUSTOMER[1]
        app_lib_settings_net_addr_t network_addr = (app_lib_settings_net_addr_t) (NRF_UICR->CUSTOMER[1] & 0xFFFFFF);
        if (network_addr == 0xFFFFFF)
        {
            network_addr = NETWORK_ADDRESS;
        }
        //Network channel : CUSTOMER[2]
        app_lib_settings_net_channel_t network_ch = (app_lib_settings_net_channel_t) (NRF_UICR->CUSTOMER[2] & 0xFF);
        if (network_ch == 0xFF)
        {
            network_ch = NETWORK_CHANNEL;
        }
    }
#endif

#ifdef FORCE_NETWORK_CONFIG
    /*  !!! Don't activate this unless in special use case !!!
        Default network address & channel applied always
        Node address is preserved if exists
    */
     app_addr_t prev_node_addr;

     if (lib_settings->getNodeAddress(&prev_node_addr) != APP_RES_OK)
    {
        // Not set
        if (lib_settings->setNodeAddress(node_addr) != APP_RES_OK)
       {
           ret = false;
       }
    }

    if (lib_settings->setNetworkAddress(NETWORK_ADDRESS) != APP_RES_OK)
    {
        ret = false;
    }

    if (lib_settings->setNetworkChannel(NETWORK_CHANNEL) != APP_RES_OK)
    {
        ret = false;
    }

#else
    // Settings are applied only when do not exist
    if (configureNodeFromBuildParameters() != APP_RES_OK)
    {
        ret = false;
    }
#endif

    /*
        Role is always enforced to NODE_DEFAULT_ROLE i.e. directed-advertiser
        If role is changed reboot is needed
    */

    app_lib_settings_role_t prev_node_role;
    if ((lib_settings->getNodeRole(&prev_node_role) != APP_RES_OK) || (prev_node_role!= NODE_DEFAULT_ROLE))
    {
        if (lib_settings->setNodeRole(NODE_DEFAULT_ROLE) != APP_RES_OK)
        {
        }
         //reboot is needed when the role is changed to directed-advertiser
        ret = false;
    }
    return ret;
}


/**
 *  @brief      Application initialization
 *  @param[in]  functions
 *              pointer to API functions
 *  @param[out]  void
 *
**/

void App_init(const app_global_functions_t * functions)
{
    adv_init();
    next_state(IDLE, 0);
    App_Scheduler_init();
    Random_init(getUniqueAddress());
    uint32_t delay = m_settings.period_ms + Random_jitter(m_settings.period_ms);
    App_Scheduler_addTask(main_task, delay) ;
    callbacks_init();


    // Set TTL to QUEUING_TIME_MS
    lib_advertiser->setQueuingTimeHp(QUEUING_TIME_MS);

    if(node_init())
    {
        lib_state->startStack();
    }
    else
    {
        lib_state->stopStack();
    }
}
