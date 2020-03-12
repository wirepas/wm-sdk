/**
    @file scheduler.h
    @brief      Header file containing the scheduler interface.
    @note       Several fields are uint32_t for future remodeling of next_time.
    @copyright  Wirepas Oy 2018
*/
#ifndef POS_SCHEDULER_H
#define POS_SCHEDULER_H

#include <stdbool.h>
#include <stdint.h>

#define SCHEDULER_PERIODIC_CHECK 60 // amount in seconds to check if anything has happened

typedef enum
{
    // event timeouts
    POS_APP_STATE_WAIT_FOR_CONNECTION = 0x00,
    POS_APP_STATE_WAIT_FOR_APP_CONFIG = 0x01,
    POS_APP_STATE_WAIT_FOR_PACKET_ACK = 0x02,

    // actions or lack of
    POS_APP_STATE_PERFORM_SCAN = 0xFA,
    POS_APP_STATE_DOWNTIME = 0xFB,
    POS_APP_STATE_IDLE = 0xFC,

    // exceptions
    POS_APP_STATE_REBOOT = 0xFF,
    //APP_STATE_TIMEOUT = 0xFF,
} scheduler_state_e;


typedef struct
{
    scheduler_state_e state;
    uint32_t period;
    uint32_t state_time;
    uint32_t schedule_next;

    bool app_config_received;
    bool message_sent;
    bool ack_received;

    uint32_t deadline;
    uint32_t timeout_ack;
    uint32_t timeout_connection;

    app_lib_time_timestamp_hp_t time_since_start;
} scheduler_info_t;

void Scheduler_init_message_sequence_id(void);
void Scheduler_init(scheduler_state_e initial_state,
                    uint32_t access_cycle_s,
                    app_lib_time_timestamp_hp_t timestamp,
                    bool on_boot);

uint32_t Scheduler_loop(const app_lib_state_t* lib_state);

void Scheduler_reset_time();
void Scheduler_increment_time();
void Scheduler_increment_message_sequence_id(void);

void Scheduler_set_state(scheduler_state_e next_state);
void Scheduler_set_next(uint32_t schedule_next);
void Scheduler_set_deadline(uint32_t deadline);

uint32_t Scheduler_get_state();
uint32_t Scheduler_get_next();
uint32_t Scheduler_get_time();
uint32_t Scheduler_get_deadline();

bool Scheduler_get_appconfig_reception(void);
bool Scheduler_get_message_dispatch(void);

uint32_t Scheduler_get_message_sequence_id(void);
uint32_t Scheduler_get_execution_time(void);

void Scheduler_set_appconfig_reception(bool flag);
void Scheduler_set_message_dispatch(bool flag);


bool Scheduler_timeout(void);

#endif /*POS_SCHEDULER_H*/