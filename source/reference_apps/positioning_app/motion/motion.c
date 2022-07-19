/**
 * @file        motion.c
 * @copyright   Wirepas Ltd 2021
 */

#define DEBUG_LOG_MODULE_NAME "MOTION"
#ifdef DEBUG_POSLIB_LOG_MAX_LEVEL
#define DEBUG_LOG_MAX_LEVEL DEBUG_POSLIB_LOG_MAX_LEVEL
#else
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#endif

#include <stdio.h>
#include <stdbool.h>
#include "api.h"
#include "motion.h"
#include "mcu.h"
#include "gpio.h"
#include "acc_interface.h"
#include "app_scheduler.h"
#include "debug_log.h"

typedef enum
{
    ACC_IDLE = 0,
    ACC_START = 1,
    ACC_READ = 2,
} acc_state_e;

/** Monitoring variables */
static bool m_mon_enabled = false;
static posapp_motion_mon_settings_t m_mon_cfg = {
    .threshold_mg = 0, 
    .duration_ms = 0,
    .cb = NULL};
    
/** Acceletometer data sampling variables */
static bool m_acc_init = false;
static bool m_acc_sampling = false;
static posapp_motion_acc_callback_f m_acc_cb = NULL;
static acc_measurement_t m_meas = {.x = 0, .y = 0, .z = 0};
static acc_state_e m_acc_state = ACC_IDLE;

static posapp_motion_status_e  m_status = POSAPP_MOTION_NOT_SUPPORTED;
#define MOTION_STATIC_TIMEOUT_MS 60000

static uint32_t set_motion_static()
{
    if (m_mon_enabled && m_mon_cfg.cb != NULL)
    {
        /* Node goes to static mode */
        m_mon_cfg.cb(POSLIB_MOTION_STATIC);
    }

    return APP_SCHEDULER_STOP_TASK;
}

static void acc_event_cb(uint8_t pin, gpio_event_e event)
{
    if (m_mon_enabled && m_mon_cfg.cb != NULL)
    {
        m_mon_cfg.cb(POSLIB_MOTION_DYNAMIC);
        App_Scheduler_addTask_execTime(set_motion_static, MOTION_STATIC_TIMEOUT_MS, 500);
    }
}

static uint32_t accelerometer_task(void)
{
    switch (m_acc_state)
    {
        case ACC_START:
        {
            uint32_t run_next_ms;
            if (ACC_startMeasurement(&run_next_ms))
            {
                m_acc_state = ACC_READ;
                LOG(LVL_DEBUG, "Acceleration measurement started");
                return run_next_ms;
            }
            else
            {
                LOG(LVL_ERROR,"Cannot start measurement");

                if (m_acc_cb != NULL)
                {
                    m_acc_cb(NULL);
                }
                m_acc_sampling = false;
                m_acc_state = ACC_IDLE;
                return APP_SCHEDULER_STOP_TASK;
            }
            break;
        }
      
        case ACC_READ:
        {
            if (ACC_readMeasurement(&m_meas))
            {
                LOG(LVL_DEBUG, "AX: %i, AY: %i, AZ: %i", m_meas.x, m_meas.y, m_meas.z);
                m_acc_cb(&m_meas);
            }
            else
            {
                LOG(LVL_ERROR, "Accelerometer measurement error");
                m_acc_cb(NULL);   
            }

            m_acc_sampling = false;
            return APP_SCHEDULER_STOP_TASK;  
            break;
        }
    
        default:
        {
            m_acc_sampling = false;
            return APP_SCHEDULER_STOP_TASK; 
        }
        break;
    }
    return APP_SCHEDULER_STOP_TASK;
}

static posapp_motion_ret_e enable_monitoring(void)
{ 

    if (!m_acc_init)
    {
        LOG(LVL_ERROR, "Accelerometer not initialised");
        m_mon_enabled = false;
        return POSAPP_MOTION_RET_ACC_INIT_ERROR;  
    }

    if (!ACC_enableMonitoring(m_mon_cfg.threshold_mg, m_mon_cfg.duration_ms))
    {
        LOG(LVL_ERROR, "Cannot enable motion monitoring");
        m_mon_enabled = false;
        return POSAPP_MOTION_RET_ACC_INIT_ERROR;
    }
    
    //FixME: to determing through testing if we need debounce (i.e. limit acc. irq fireing)
    if (GPIO_register_for_event(MOTION_MON_INT_PIN, 
                            GPIO_NOPULL,
                            GPIO_EVENT_LH,
                            0,              
                            acc_event_cb) != GPIO_RES_OK)
    {

        LOG(LVL_ERROR, "Cannot enable acc. interrupt pin");
        m_mon_enabled = false;
        return POSAPP_MOTION_RET_GPIO_ERROR;
    }
    else
    {
        LOG(LVL_DEBUG, "Acc pin %u registered", MOTION_MON_INT_PIN);
    }

    m_mon_enabled = true;
    return POSAPP_MOTION_RET_OK;
}


static posapp_motion_ret_e disable_monitoring(void)
{
    bool ret;

    if (!m_mon_enabled)
    {
        return POSAPP_MOTION_RET_ERROR;
    }

    ret = (GPIO_deregister_for_event(MOTION_MON_INT_PIN) != GPIO_RES_OK);
    
    return ret ? POSAPP_MOTION_RET_OK : POSAPP_MOTION_RET_ERROR;     
}


posapp_motion_ret_e PosAppMotion_startMonitoring(posapp_motion_mon_settings_t * cfg)
{
    
    if (!m_acc_init)
    {
        LOG(LVL_ERROR, "Aceelerometer not initialized");
        return POSAPP_MOTION_RET_ERROR;
    }

    if (m_acc_sampling)
    {
        LOG(LVL_ERROR, "Acceleration measurement ongoing");
        return POSAPP_MOTION_RET_ERROR;
    }

    if ( cfg->threshold_mg < POSAPP_MOTION_THRESHOLD_MG_MIN ||
         cfg->threshold_mg > POSAPP_MOTION_THRESHOLD_MG_MAX)
    {
        LOG(LVL_ERROR, "Monitoring threshold %u outside range", cfg->threshold_mg);
        return POSAPP_MOTION_RET_INVALID_PARAMETER;
    }

    if ( /* cfg->duration_ms < POSAPP_MOTION_DURATION_MS_MIN || */
         cfg->duration_ms > POSAPP_MOTION_DURATION_MS_MAX)
    {
        LOG(LVL_ERROR, "Monitoring duration %u outside range", cfg->duration_ms);
        return POSAPP_MOTION_RET_INVALID_PARAMETER;
    }

    if (cfg->cb == NULL)
    {
        LOG(LVL_ERROR, "Monitoring callback is NULL");
        return POSAPP_MOTION_RET_INVALID_PARAMETER;     
    }

    m_mon_cfg = *cfg;
    return enable_monitoring();
    
}

posapp_motion_ret_e PosAppMotion_stopMonitoring()
{
    if(m_mon_enabled)
    {
        m_mon_enabled = false;
        if(!m_acc_sampling)
        {
            return disable_monitoring();
        }
    }
    return POSAPP_MOTION_RET_OK;
}

posapp_motion_status_e PosAppMotion_getStatus()
{
    return m_status;
}

posapp_motion_ret_e PosAppMotion_getAcceleration(posapp_motion_acc_callback_f cb)
{
    if (!m_acc_init)
    {
        LOG(LVL_ERROR, "Aceelerometer not initialized");
        return POSAPP_MOTION_RET_ERROR;
    }

    if (m_acc_sampling)
    {
        LOG(LVL_ERROR, "Measurement collection ongoing");
        return POSAPP_MOTION_RET_ERROR;
    }

    if (cb == NULL)
    {
        LOG(LVL_ERROR, "Callback cannot be NULL");
        return POSAPP_MOTION_RET_ERROR;
    }

    m_status = POSAPP_MOTION_SAMPLING;
    m_acc_sampling = true;

    m_acc_state = ACC_START;
    App_Scheduler_addTask_execTime(accelerometer_task, APP_SCHEDULER_SCHEDULE_ASAP, 500);
    
    return POSAPP_MOTION_RET_OK;
}

posapp_motion_ret_e PosAppMotion_init()
{
    if (!ACC_init())
    {
        LOG(LVL_ERROR, "Cannot initialize the accelerometer");
        m_acc_init = false;
        return POSAPP_MOTION_RET_ACC_INIT_ERROR;
    } 

    m_acc_init = true;
    LOG(LVL_DEBUG, "Accelerometer intitialised");
    return POSAPP_MOTION_RET_OK;
}