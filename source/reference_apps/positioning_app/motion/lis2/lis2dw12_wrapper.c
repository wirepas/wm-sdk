/**
 * @file  lis2dw12_wrapper.c
 * @brief Wrapper on top of LIS2DW12 driver from STMicroelectronics GitHub.
 * @copyright  Wirepas Ltd 2021
 */

#define DEBUG_LOG_MODULE_NAME "LIS2DW12"
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
#include "lis2dw12_reg.h"
#include "lis2_dev.h"
#include "lis2dw12_wrapper.h"

/* Constant to convert raw acceleration data to mg. */
#define RAW_ACCEL_TO_MG            16

/** Reference to the lis2dw12 driver. */
stmdev_ctx_t m_lis2dw12_dev;

bool m_initialised = false;
bool m_monitoring_enabled = false;

/**
    @brief     Blocking wait for a given amount of time.
    @param[in] period Time to wait in ms.
*/
static void lis2dw12_delay_ms(uint32_t period)
{
    app_lib_time_timestamp_hp_t end;
    end = lib_time->addUsToHpTimestamp(lib_time->getTimestampHp(),
                                       period * 1000);

    /* Active wait until period is elapsed */
    while (lib_time->isHpTimestampBefore(lib_time->getTimestampHp(),
                                         end));
}

static uint8_t lis2dw12_get_int_threshold(uint16_t threshold, lis2dw12_fs_t fs)
{
    uint8_t lsb;

    switch(fs)
    {
        case LIS2DW12_2g:
            lsb = 2*1000/64;
            break;
        case LIS2DW12_4g:
            lsb = 4*1000/64;
            break;
        case LIS2DW12_8g:
            lsb = 8*1000/64;
            break;
        case LIS2DW12_16g:
            lsb = 16*1000/64;
            break;
        default:
            lsb = 1;
             LOG(LVL_ERROR, "LIS2DW12 full scale index %u not supported", fs);

    }

    return (uint8_t) (threshold / lsb);
}

static uint8_t lis2dw12_get_int_duration(uint16_t duration_ms, lis2dw12_odr_t odr)
{
    uint16_t duration = duration_ms;

    switch(odr)
    {
        case LIS2DW12_XL_ODR_1Hz6_LP_ONLY:
            duration /= 625;
            break;
        case LIS2DW12_XL_ODR_12Hz5:
            duration  /= 80;
            break;
        case LIS2DW12_XL_ODR_25Hz:
            duration /= 40; 
            break;
        case LIS2DW12_XL_ODR_50Hz:
            duration /= 20; 
            break;
        case LIS2DW12_XL_ODR_100Hz:
            duration /= 10;
            break;
         case LIS2DW12_XL_ODR_200Hz:
            duration /= 5;
            break;
        case LIS2DW12_XL_ODR_400Hz:
            duration = (duration * 4) / 10;
            break;
        case LIS2DW12_XL_ODR_800Hz:
            duration = (duration * 8) / 10;
            break;
        case LIS2DW12_XL_ODR_1k6Hz:
            duration = (duration * 160) / 100;
            break;
        default:
            duration = 0;
    }

    return (uint8_t) ((duration > 3) ? 3 : duration);
}

bool ACC_init()
{
    uint8_t whoamI;
    
    // Create device for LIS2DW12 driver with the adapted read/write functions

    LIS2_dev_init(&m_lis2dw12_dev);

    /* Check the device ID. */
    lis2dw12_device_id_get(&m_lis2dw12_dev, &whoamI);
    LOG(LVL_INFO, "whoamI: %u", whoamI);
    if (whoamI != LIS2DW12_ID)
    {
        /* Device not found or did not respond. */
         LOG(LVL_ERROR, "LIS2DW12 cannot be detected!");
         m_initialised = false;
        return false;
    }

    /* Restore default configuration */
    lis2dw12_reset_set(&m_lis2dw12_dev, PROPERTY_ENABLE);

    /* Set to Power Down. */
    lis2dw12_data_rate_set(&m_lis2dw12_dev, LIS2DW12_XL_ODR_OFF);
    lis2dw12_delay_ms(80);

    /* Reload memory. */
    lis2dw12_boot_set(&m_lis2dw12_dev, 1);
    lis2dw12_delay_ms(10);

    /* Enable Block Data Update. */
    lis2dw12_block_data_update_set(&m_lis2dw12_dev, PROPERTY_ENABLE);

    /* Configure INT pin to pulsed interrupt */
    lis2dw12_int_notification_set(&m_lis2dw12_dev, LIS2DW12_INT_PULSED);

    /* Set full duration to 2g. */
    lis2dw12_full_scale_set(&m_lis2dw12_dev, LIS2DW12_2g);
   
    /* Set device resolution. */
    lis2dw12_power_mode_set(&m_lis2dw12_dev, LIS2DW12_INT_OPPERATING_MODE);
    lis2dw12_delay_ms(80);

    m_initialised = true;
    m_monitoring_enabled = false;
    return true;
}


bool ACC_enableMonitoring(uint16_t threshold_mg, uint16_t duration_ms)
{
    if (!m_initialised)
    {
        return false;
    }

    /* Set full scale */
    lis2dw12_full_scale_set(&m_lis2dw12_dev, LIS2DW12_2g);

    lis2dw12_power_mode_set(&m_lis2dw12_dev,LIS2DW12_INT_OPPERATING_MODE);
    lis2dw12_delay_ms(80);
    lis2dw12_data_rate_set(&m_lis2dw12_dev,LIS2DW12_INT_DURATION);

    /* Apply high-pass digital filter on Wake-Up function */
    lis2dw12_filter_path_set(&m_lis2dw12_dev, LIS2DW12_HIGH_PASS_ON_OUT);
    /* Data sent to wake-up interrupt function */
    lis2dw12_wkup_feed_data_set(&m_lis2dw12_dev, LIS2DW12_HP_FEED);

    uint8_t int_ths = lis2dw12_get_int_threshold(threshold_mg, LIS2DW12_2g);
    uint8_t int_duration = lis2dw12_get_int_duration(duration_ms, LIS2DW12_INT_DURATION);

    /* Wake-Up interrupt signal is generated for
    * each X,Y,Z filtered data exceeding the configured threshold
    */
    /* Set wake-up duration
    * Wake up duration event 1LSb = 1 / ODR
    */
    lis2dw12_wkup_dur_set(&m_lis2dw12_dev, int_duration);

    /* Set sleep duration
    * Duration to go in sleep mode (1 LSb = 512 / ODR)
    */
    lis2dw12_act_sleep_dur_set(&m_lis2dw12_dev, 0);

    /* Set wake-up threshold
    * Set Wake-Up threshold: 1 LSb corresponds to FS_XL/2^6
    */
    lis2dw12_wkup_threshold_set(&m_lis2dw12_dev, int_ths);

    /* Config activity / inactivity or stationary / motion detection */
    lis2dw12_act_mode_set(&m_lis2dw12_dev, LIS2DW12_DETECT_STAT_MOTION);

#ifdef LIS2DW12_MOTION_USE_INT1
    lis2dw12_reg_t int_route;
    
    /* Enable interrupt generation on Wake-Up INT1 pin */
    lis2dw12_pin_int1_route_get(&m_lis2dw12_dev, &int_route.ctrl4_int1_pad_ctrl);
    int_route.ctrl4_int1_pad_ctrl.int1_wu = PROPERTY_ENABLE;
    lis2dw12_pin_int1_route_set(&m_lis2dw12_dev, &int_route.ctrl4_int1_pad_ctrl);
    lis2dw12_delay_ms(80);
#else
    #error "Only LIS2DW12_MOTION_USE_INT1 must be defined"
#endif

    /* Enable relative reference wake-up */
    //lis2dw12_reference_mode_set(&m_lis2dw12_dev, PROPERTY_ENABLE);

    LOG(LVL_DEBUG, "Motion LIS2DW12 enabled. Threshold: %u Duration: %u", int_ths, int_duration);
    m_monitoring_enabled = true;
    return true;
}

void ACC_disableMonitoring(void)
{
    if (m_monitoring_enabled)
    {
        /* Put the accelerometer back to sleep. */
        lis2dw12_data_rate_set(&m_lis2dw12_dev, LIS2DW12_XL_ODR_OFF);
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
        *wait_ms = LIS2DW12_WAKE_UP_TIME_MON_MS;
        return true;
    }

    lis2dw12_power_mode_set(&m_lis2dw12_dev, LIS2DW12_CONT_LOW_PWR_12bit);
    lis2dw12_data_rate_set(&m_lis2dw12_dev, LIS2DW12_XL_ODR_1Hz6_LP_ONLY);
    *wait_ms = LIS2DW12_WAKE_UP_TIME_MS;
    return true;
}

bool ACC_readMeasurement(acc_measurement_t * meas)
{
    int16_t acc_raw[3] = {0,};
    bool ret = false;

    /* Read accelerometer data. */
    ret = (lis2dw12_acceleration_raw_get(&m_lis2dw12_dev, acc_raw) == 0);

    meas->x = acc_raw[0] / RAW_ACCEL_TO_MG;
    meas->y = acc_raw[1] / RAW_ACCEL_TO_MG;
    meas->z = acc_raw[2] / RAW_ACCEL_TO_MG;

    if (!m_monitoring_enabled)
    {
        /* Put the accelerometer back to sleep. */
        lis2dw12_data_rate_set(&m_lis2dw12_dev, LIS2DW12_XL_ODR_OFF);
    }
    return ret;
}