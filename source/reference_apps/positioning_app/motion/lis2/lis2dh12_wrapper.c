/**
 * @file  lis2dh12_wrapper.c
 * @brief Wrapper on top of LIS2DH12 driver from STMicroelectronics GitHub.
 * @copyright  Wirepas Ltd 2021
 */

#define DEBUG_LOG_MODULE_NAME "LIS2DH12"
#ifdef DEBUG_POSLIB_LOG_MAX_LEVEL
#define DEBUG_LOG_MAX_LEVEL DEBUG_POSLIB_LOG_MAX_LEVEL
#else
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#endif

#include "api.h"
#include "board.h"
#include "hal_api.h"
#include "debug_log.h"
#include "acc_interface.h"
#include "lis2dh12_reg.h"
#include "lis2_dev.h"
#include "lis2dh12_wrapper.h"

/* Constant to convert raw acceleration data to mg. */
#define RAW_ACCEL_TO_MG            16

/** Reference to the lis2dh12 driver. */
static stmdev_ctx_t m_lis2dh12_dev;

bool m_initialised = false;
bool m_monitoring_enabled = false;

/**
    @brief     Blocking wait for a given amount of time.
    @param[in] period Time to wait in ms.
*/
static void lis2dh12_delay_ms(uint32_t period)
{
    app_lib_time_timestamp_hp_t end;
    end = lib_time->addUsToHpTimestamp(lib_time->getTimestampHp(),
                                       period * 1000);

    /* Active wait until period is elapsed */
    while (lib_time->isHpTimestampBefore(lib_time->getTimestampHp(),
                                         end));
}

static uint8_t lis2dh12_get_int_threshold(uint16_t threshold, lis2dh12_fs_t fs)
{
    uint8_t lsb;
    
    switch(fs)
    {
        case LIS2DH12_2g:
            lsb = 16;
            break;
        case LIS2DH12_4g:
            lsb = 32;
            break;
        case LIS2DH12_8g:
            lsb = 62;
            break;
        case LIS2DH12_16g:
            lsb = 186;
            break;
        default:
            lsb = 1;
             LOG(LVL_ERROR, "LIS2DH12 full scale index %u not supported", fs);

    }

    return (uint8_t) (threshold / lsb);
}

static uint8_t lis2dh12_get_int_duration(uint16_t duration_ms, lis2dh12_odr_t odr)
{
    uint16_t duration = duration_ms;

    switch(odr)
    {
        case LIS2DH12_ODR_1Hz:
        {
            duration /= 1000;
            break;
        }
        case LIS2DH12_ODR_10Hz:
        {
            duration  /= 100;
            break;
        }
        case LIS2DH12_ODR_25Hz:
        {
            duration /= 40; 
            break;
        }
        case LIS2DH12_ODR_100Hz:
        {
            duration /= 10;
            break;
        }
         case LIS2DH12_ODR_200Hz:
        {
            duration /= 5;
            break;
        }
        case LIS2DH12_ODR_400Hz:
        {
            duration = (duration * 4) / 10;
            break;
        }
        case LIS2DH12_ODR_1kHz620_LP:
        {
            duration = (duration * 162) / 100;
            break;
        }
        case LIS2DH12_ODR_5kHz376_LP_1kHz344_NM_HP:
        {
            duration = ((uint32_t) duration * 5376) / 100;
            break;
        }
        default:
        {
            duration = 0;
        }
    }

    return (uint8_t) ((duration > 127) ? 127 : duration);
}

bool ACC_init()
{
    uint8_t whoamI;
    
    // Create device for LIS2DH12 driver with the adapted read/write functions

    LIS2_dev_init(&m_lis2dh12_dev);

    /* Check the device ID. */
    lis2dh12_device_id_get(&m_lis2dh12_dev, &whoamI);
    if (whoamI != LIS2DH12_ID)
    {
        /* Device not found or did not respond. */
         LOG(LVL_ERROR, "LIS2DH12 cannot be detected!");
         m_initialised = false;
        return false;
    }

    /* Set to Power Down. */
    lis2dh12_data_rate_set(&m_lis2dh12_dev, LIS2DH12_POWER_DOWN);
    lis2dh12_delay_ms(10);

    /* Reload memory. */
    lis2dh12_boot_set(&m_lis2dh12_dev, 1);
    lis2dh12_delay_ms(10);

    /* Enable Block Data Update. */
    lis2dh12_block_data_update_set(&m_lis2dh12_dev, PROPERTY_ENABLE);

    /* Set full duration to 2g. */
    lis2dh12_full_scale_set(&m_lis2dh12_dev, LIS2DH12_2g);
   
    /* Set device resolution. */
    lis2dh12_operating_mode_set(&m_lis2dh12_dev, LIS2DH12_INT_OPPERATING_MODE);

    m_initialised = true;
    m_monitoring_enabled = false;
    return true;
}


bool ACC_enableMonitoring(uint16_t threshold_mg, uint16_t duration_ms)
{
    uint8_t data;

    if (!m_initialised)
    {
        return false;
    }

    //lis2dh12_high_pass_on_outputs_set(&m_lis2dh12_dev, 1);
    data = 0x09;
    lis2dh12_write_reg(&m_lis2dh12_dev, LIS2DH12_CTRL_REG2, &data, 1);
    lis2dh12_high_pass_int_conf_set(&m_lis2dh12_dev, LIS2DH12_ON_INT1_GEN);

    lis2dh12_full_scale_set(&m_lis2dh12_dev, LIS2DH12_2g);

    uint8_t int_ths = lis2dh12_get_int_threshold(threshold_mg, LIS2DH12_2g);
    uint8_t int_duration = lis2dh12_get_int_duration(duration_ms, LIS2DH12_INT_DURATION);
    uint8_t val;

    lis2dh12_filter_reference_get(&m_lis2dh12_dev, &val);
    lis2dh12_high_pass_bandwidth_set(&m_lis2dh12_dev, LIS2DH12_LIGHT);
    lis2dh12_high_pass_mode_set(&m_lis2dh12_dev, LIS2DH12_AUTORST_ON_INT);

#ifdef LIS2DH12_MOTION_USE_INT1

    lis2dh12_ctrl_reg3_t  ctrl_reg3 = {.i1_ia1 = 1};
    lis2dh12_pin_int1_config_set(&m_lis2dh12_dev, &ctrl_reg3);

    lis2dh12_int1_cfg_t int1_cfg = {.xhie = 1, .yhie = 1, .zhie =1};
    lis2dh12_int1_gen_conf_set(&m_lis2dh12_dev, &int1_cfg);
    
    lis2dh12_int1_gen_threshold_set(&m_lis2dh12_dev, int_ths);
    lis2dh12_int1_gen_duration_set(&m_lis2dh12_dev, int_duration); 
    
#elif defined LIS2DH12_MOTION_USE_INT2

    lis2dh12_ctrl_reg6_t ctrl_reg6 = {.i2_ia1 = 1};
    lis2dh12_pin_int2_config_set(&m_lis2dh12_dev, &ctrl_reg6);

    lis2dh12_int2_cfg_t int2_cfg = {.xhie = 1, .yhie = 1, .zhie =1 };
    lis2dh12_int2_gen_conf_set(&m_lis2dh12_dev,(lis2dh12_int2_cfg_t*) &int2_cfg);
    
    lis2dh12_int2_gen_threshold_set(&m_lis2dh12_dev, int_ths);
    lis2dh12_int2_gen_duration_set(&m_lis2dh12_dev, int_duration);

#else
    #error "Either LIS2DH12_MOTION_USE_INT1 | LIS2DH12_MOTION_USE_INT2 must be defined"
#endif

    lis2dh12_operating_mode_set(&m_lis2dh12_dev,LIS2DH12_INT_OPPERATING_MODE);
    lis2dh12_data_rate_set(&m_lis2dh12_dev,LIS2DH12_INT_DURATION);

    LOG(LVL_DEBUG, "Motion LIS2DH12 enabled. Threshold: %u Duration: %u", int_ths, int_duration);
    m_monitoring_enabled = true;
    return true;
}

void ACC_disableMonitoring(void)
{
    if (m_monitoring_enabled)
    {
        /* Put the accelerometer back to sleep. */
        lis2dh12_data_rate_set(&m_lis2dh12_dev, LIS2DH12_POWER_DOWN);
        m_monitoring_enabled = false;
    }
}

bool ACC_startMeasurement(uint32_t *wait_ms)
{
     
    if (!m_initialised)
    {
        return false;
    } 
  
    /* When monitoring enabled the same settings used */
    if (m_monitoring_enabled)
    {
        *wait_ms = LIS2DH12_WAKE_UP_TIME_MON_MS;
        return true;
    }

    lis2dh12_operating_mode_set(&m_lis2dh12_dev, LIS2DH12_HR_12bit);
    lis2dh12_data_rate_set(&m_lis2dh12_dev, LIS2DH12_ODR_1Hz);
    *wait_ms = LIS2DH12_WAKE_UP_TIME_MS;
    return true;
}

bool ACC_readMeasurement(acc_measurement_t * meas)
{
    int16_t acc_raw[3] = {0,};
    bool ret = false;

    /* Read accelerometer data. */
    ret = (lis2dh12_acceleration_raw_get(&m_lis2dh12_dev, acc_raw) == 0);

    meas->x = acc_raw[0] / RAW_ACCEL_TO_MG;
    meas->y = acc_raw[1] / RAW_ACCEL_TO_MG;
    meas->z = acc_raw[2] / RAW_ACCEL_TO_MG;

    if (!m_monitoring_enabled)
    {
        /* Put the accelerometer back to sleep. */
        lis2dh12_data_rate_set(&m_lis2dh12_dev, LIS2DH12_POWER_DOWN);
    }
    return ret;
}