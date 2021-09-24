/**
 * @file       poslib_event.c
 * @brief      Main code to process the positioning callbacks.
 * @copyright  Wirepas Ltd. 2021
 */

#define DEBUG_LOG_MODULE_NAME "POSLIB_EVENT"
#ifdef DEBUG_POSLIB_LOG_MAX_LEVEL
#define DEBUG_LOG_MAX_LEVEL DEBUG_POSLIB_LOG_MAX_LEVEL
#else
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#endif
#include "debug_log.h"
#include <string.h>
#include "poslib.h"
#include "poslib_control.h"
#include "poslib_event.h"
#include "sl_list.h"
#include "app_scheduler.h"
#include "poslib_ble_beacon.h"

/** Module private data structures */
typedef struct{
poslib_events_e event;
poslib_events_listen_info_f cb;
} POSLIB_FLAG_EVENT_public_sub_t;

/** Module private function definitions */
void generate_public_events(poslib_internal_event_t * i_event);

/** Variables: internal control events */
bool m_internal_init = false;
static sl_list_head_t m_events_head;
static poslib_internal_event_t m_internal_events[MAX_INTERNAL_EVENTS];

/** Variables: PosLib public events */
bool m_public_init = false;
POSLIB_FLAG_EVENT_public_sub_t m_public_event_db[POSLIB_FLAG_EVENT_SUBSCRIBERS_MAX];


static uint32_t handle_events()
{
    poslib_internal_event_t * event;

    if (sl_list_size(&m_events_head) == 0)
    {
        return APP_SCHEDULER_STOP_TASK; 
    }

    lib_system->enterCriticalSection();
    event = (poslib_internal_event_t *) sl_list_pop_front(&m_events_head);
    lib_system->exitCriticalSection();

    LOG(LVL_DEBUG, "Event %u (left %u)", event->type, sl_list_size(&m_events_head));

    // Event processing call for all modules
    PosLibCtrl_processEvent(event);
    PosLibBle_processEvent(event);
    generate_public_events(event);

    // Free event slot
    event->type = POSLIB_CTRL_EVENT_NONE;

    return (sl_list_size(&m_events_head) == 0) ? APP_SCHEDULER_STOP_TASK : 
                                                APP_SCHEDULER_SCHEDULE_ASAP;
}

bool PosLibEvent_add(poslib_internal_event_type_e type)
{
    poslib_internal_event_t * event = NULL;
    uint8_t i;
    
    lib_system->enterCriticalSection();
    if (!m_internal_init)
    {
        sl_list_init(&m_events_head);
        memset(m_internal_events, 0, sizeof(m_internal_events));
        m_internal_init = true;
    }

    for (i = 0; i < MAX_INTERNAL_EVENTS; i++)
    {
        if (m_internal_events[i].type == POSLIB_CTRL_EVENT_NONE)
        {
            event = &m_internal_events[i];
            event->type = type;
            sl_list_push_back(&m_events_head, (sl_list_t *) event);
            break; 
        }
    }
    lib_system->exitCriticalSection();

    if (event == NULL)
    {
        LOG(LVL_ERROR, "Cannot add event %u", type);
        return false;
    }

    LOG(LVL_DEBUG, "Event %u added slot %u (left %u)", event->type, i, sl_list_size(&m_events_head));

    App_Scheduler_addTask_execTime(handle_events, APP_SCHEDULER_SCHEDULE_ASAP, 500);
    return true;
}


poslib_ret_e PosLibEvent_register(poslib_events_e event,
                            poslib_events_listen_info_f cb, 
                            uint8_t * id)
{
    if (!m_public_init)
    {
        memset(&m_public_event_db, 0, sizeof(m_public_event_db));
        m_public_init = true;
    }
    
    if (cb == NULL)
    {
        LOG(LVL_ERROR,"Event register - NULL callback")
        return POS_RET_EVENT_REG_ERROR;
    }

    if (!(event & POSLIB_FLAG_EVENT_ALL))
    {
        LOG(LVL_ERROR,"Event register - unknown event %04x", event);
        return POS_RET_EVENT_REG_ERROR;

    }

    for (uint8_t i = 0; i < POSLIB_FLAG_EVENT_SUBSCRIBERS_MAX; i++)
    {
        if (m_public_event_db[i].cb == NULL)
        {
            m_public_event_db[i].event = event;
            m_public_event_db[i].cb = cb;
            *id = i;
            LOG(LVL_DEBUG, "Registered event %04x cb %u", event, cb);
            return POS_RET_OK;
        }
 }
    
    return POS_RET_EVENT_REG_ERROR;
}

void PosLibEvent_deregister(uint8_t id)
{
    if (id < POSLIB_FLAG_EVENT_SUBSCRIBERS_MAX)
    {
        m_public_event_db[id].event = 0;
        m_public_event_db[id].cb = NULL;
    }
}

void generate_public_events(poslib_internal_event_t * ctrl_event)
{
   
    POSLIB_FLAG_EVENT_info_t msg;
    poslib_events_e event = POSLIB_FLAG_EVENT_NONE;

    switch (ctrl_event->type)
    {
        case POSLIB_CTRL_EVENT_SCAN_STARTED:
        {
            event = POSLIB_FLAG_EVENT_UPDATE_START;
            break;
        }

        case POSLIB_CTRL_EVENT_ONLINE:
        {
            event = POSLIB_FLAG_EVENT_UPDATE_START | POSLIB_FLAG_EVENT_ONLINE;
            break;
        }
        
        case POSLIB_CTRL_EVENT_CONFIG_CHANGE:
        {
            event = POSLIB_FLAG_EVENT_CONFIG_CHANGE;
            break;
        }
        
        case POSLIB_CTRL_EVENT_UPDATE_END:
        {
            event = POSLIB_FLAG_EVENT_UPDATE_END;
            break;
        }
        
        case POSLIB_CTRL_EVENT_BLE_START:
        {
            event = POSLIB_FLAG_EVENT_BLE_START;
            break; 
        }
        
        case POSLIB_CTRL_EVENT_BLE_STOP:
        {
            event = POSLIB_FLAG_EVENT_BLE_STOP;
            break; 
        }

        case POSLIB_CTRL_EVENT_LED_ON:
        {
            event = POSLIB_FLAG_EVENT_LED_ON;
            break; 
        }

        case POSLIB_CTRL_EVENT_LED_OFF:
        {
            event = POSLIB_FLAG_EVENT_LED_OFF;
            break; 
        }

        default:
        {
            event = POSLIB_FLAG_EVENT_NONE;
        }
    }

    if (event == POSLIB_FLAG_EVENT_NONE)
    {
        return;
    }

    msg.event_id = event;
    msg.time_hp = lib_time->getTimestampHp();
    msg.nrls_sleep_time_sec = 0; //FixMe: to deprecate

    for (uint8_t i = 0; i < POSLIB_FLAG_EVENT_SUBSCRIBERS_MAX; i++)
    {
        if ((m_public_event_db[i].cb != NULL) && (m_public_event_db[i].event & event))
        {
            m_public_event_db[i].cb(&msg);
        }
    }
}