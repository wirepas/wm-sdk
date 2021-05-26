/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef INVENTORY_APP_COMMON_H
#define INVENTORY_APP_COMMON_H



typedef enum
{
    ADV_TYPE0 = 0,  // data type sent by tag
    ADV_TYPE1 = 1,  // reserved
    ADV_TYPE2 = 2,  // agregated data : RSSI
    ADV_TYPE3 = 3,  // agregated data : OTAP procesed scratchpag seq
    ADV_TYPE4 = 4,  // agregated data : OTAP available scratchpad seq
    ADV_TYPE5 = 5,  // agregated data : voltage
    ADV_TYPE6 = 6   // agregated data : message sequence (lowest byte)
}adv_payload_type_e;

// tag advertiser data
typedef struct  __attribute__ ((packed))
{
    uint8_t type;
    int8_t rssi;
    uint16_t seq;
    uint16_t voltage;
    uint8_t proc_otap_seq;
    uint8_t stored_otap_seq;
    uint8_t send_count;
    uint8_t scan_count;
} adv_tag_data_t;

// ACK control data: size should be <= ACK_DATA_SIZE
typedef struct{
    uint8_t period;   // seconds
    uint8_t max_scan_time;  //x10 ms
    uint8_t otap_seq;
} adv_tag_app_cfg_t;

#define DIRADV_AGR_EP 200 //source & destination enpoint for agregated advertiser data

//do not change! should be always <= than DIRADV_MAX_ACK_LEN and according to app config size
#define TAG_APP_CFG_SIZE 13

#ifndef NETWORK_BEACON_RATE_US
#define NETWORK_BEACON_RATE_US 1000000
#endif

//do not change these definitions!
#define NODE_ADDRESS_SIZE 4
#define MSG_TYPE2_PREFIX 0x80
#define DATA_MSG_SIZE 102
#define MSG_TYPE2_HEADER 2
#define MSG_TYPE2_PAYLOAD_SIZE 1
#define MSG_TYPE2_MAX_ITEMS (DATA_MSG_SIZE - MSG_TYPE2_HEADER) \
/ (NODE_ADDRESS_SIZE + MSG_TYPE2_PAYLOAD_SIZE)

#define INVENTORY_APPCFG_TLV_TYPE 0xC2

#endif
