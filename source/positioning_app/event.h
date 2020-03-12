/**
    @file events.h
    @brief  Header file for the application events.
    @copyright  : Wirepas Oy 2018
*/

#ifndef POS_EVENTS_H
#define POS_EVENTS_H

#include <stdint.h>

#include "api.h"
#include "scheduler.h"

scheduler_state_e Event_beacon_reception(const app_lib_state_beacon_rx_t*
        beacon);

scheduler_state_e Event_network_scan_end(void);

scheduler_state_e Event_data_ack(const app_lib_data_sent_status_t* status);

scheduler_state_e Event_app_config(const uint8_t* bytes,
                                   uint32_t num_bytes,
                                   uint8_t seq);
#endif