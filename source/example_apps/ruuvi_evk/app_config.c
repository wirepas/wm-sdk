
#include <string.h>
#include "api.h"
#include "app_config.h"
#include "tlv.h"


/* Default configuration values */
#define DEFAULT_TEMPERATURE_EN    true
#define DEFAULT_HUMIDITY_EN       true
#define DEFAULT_PRESSURE_EN       true
#define DEFAULT_ACCEL_X_EN        true
#define DEFAULT_ACCEL_Y_EN        true
#define DEFAULT_ACCEL_Z_EN        true
#define DEFAULT_SENSORS_PERIOD_MS (10*1000)

/* Minimum time betwenn two sensor packet send. */
#define SENSOR_PERIOD_MIN_S       1
/* Maximum time betwenn two sensor packet send. */
#define SENSOR_PERIOD_MAX_S       3600
/* 1 Enables the sensor in CMDID_SENSORS_ENABLE command. */
#define SENSOR_ENABLE_VALUE        1

/* Position of each sensor enable in the CMDID_SENSORS_ENABLE command. */
#define SENSOR_EN_TEMPERATURE_BYTE 0
#define SENSOR_EN_HUMIDITY_BYTE    1
#define SENSOR_EN_PRESSURE_BYTE    2
#define SENSOR_EN_ACCEL_X_BYTE     3
#define SENSOR_EN_ACCEL_Y_BYTE     4
#define SENSOR_EN_ACCEL_Z_BYTE     5

/* Supported commands by the app config module. */
typedef enum
{
    CMDID_SENSORS_ENABLE = 1, /**< Sensor enable command. */
    CMDID_SENSORS_PERIOD = 2, /**< Set sensor period command. */
} app_config_cmd_e;

/* Configuration of the application. */
app_config_t m_config;
/* Callback function when config is changed. */
on_config_change_cb_f m_callback;

/**
    @brief  Configure which sensor is enable.
    @return Response to the request ok or not ok.
*/
static bool handleCommandSensorsEnable(tlv_item_t * item)
{
    bool ret = false;

    if (item->length == 6)
    {
        m_config.temperature_enable =
            (item->value[SENSOR_EN_TEMPERATURE_BYTE] ==  SENSOR_ENABLE_VALUE);
        m_config.humidity_enable =
            (item->value[SENSOR_EN_HUMIDITY_BYTE] ==  SENSOR_ENABLE_VALUE);
        m_config.pressure_enable =
            (item->value[SENSOR_EN_PRESSURE_BYTE] ==  SENSOR_ENABLE_VALUE);
        m_config.accel_x_enable =
            (item->value[SENSOR_EN_ACCEL_X_BYTE] ==  SENSOR_ENABLE_VALUE);
        m_config.accel_y_enable =
            (item->value[SENSOR_EN_ACCEL_Y_BYTE] ==  SENSOR_ENABLE_VALUE);
        m_config.accel_z_enable =
            (item->value[SENSOR_EN_ACCEL_Z_BYTE] ==  SENSOR_ENABLE_VALUE);
        ret = true;
    }

    return ret;
}

/**
    @brief  Check that Sensor period  is within boundaries before it will take
            it into use.
    @return Response to the request ok or not ok.
*/
static bool handleCommandSensorsPeriod(tlv_item_t * item)
{
    bool ret = false;

    if ((item->length == 1) || (item->length == 2))
    {
        uint16_t val = 0;
        memcpy(&val, item->value, item->length);

        if (val >= SENSOR_PERIOD_MIN_S && val <= SENSOR_PERIOD_MAX_S)
        {
            m_config.sensors_period_ms = val*1000;
            ret = true;
        }
        else if (val == 0)
        {
            m_config.sensors_period_ms = 0;
            ret = true;
        }
    }

    return ret;
}

/**
    @brief     Parse the received configuration.
    @param[in] msg Pointer to the received configuration buffer.
    @param[in] length Length of the configuration buffer.
    @return    Response to the request ok or not ok.
*/
static bool parseMessage(const uint8_t * msg, uint8_t length)
{
    bool rval = true;
    tlv_record record;

    Tlv_init(&record, (uint8_t *)msg, length);

    while (rval == true)
    {
        tlv_item_t item;
        tlv_res_e tlv_res;

        tlv_res = Tlv_Decode_getNextItem(&record, &item);

        if (tlv_res == TLV_RES_ERROR)
        {
            rval = false;
            break;
        }
        else if (tlv_res == TLV_RES_END)
        {
            break;
        }

        switch (item.type)
        {
            case CMDID_SENSORS_ENABLE:
                rval = handleCommandSensorsEnable(&item);
                break;
            case CMDID_SENSORS_PERIOD:
                rval = handleCommandSensorsPeriod(&item);
                break;
            default:
                /* Unknown command but tlv record is valid. */
                break;
        }
    }
    return rval;
}

/**
    @brief     Wrapper for the reception of an appconfig
    @param[in] bytes     The appconfig bytes
    @param[in] seq       The appconfig sequence number
    @param[in] interval  The appconfig interval rate (in seconds)
*/
static void cb_app_config(const uint8_t* bytes, uint8_t seq, uint16_t interval)
{
    /* Unused parameters. */
    (void) seq;
    (void) interval;

    parseMessage(bytes, lib_data->getAppConfigNumBytes());

    if (m_callback != NULL)
    {
        m_callback();
    }
}

void App_Config_init(on_config_change_cb_f cb)
{
    m_config.temperature_enable = DEFAULT_TEMPERATURE_EN;
    m_config.humidity_enable = DEFAULT_HUMIDITY_EN;
    m_config.pressure_enable = DEFAULT_PRESSURE_EN;
    m_config.accel_x_enable = DEFAULT_ACCEL_X_EN;
    m_config.accel_y_enable = DEFAULT_ACCEL_Y_EN;
    m_config.accel_z_enable = DEFAULT_ACCEL_Z_EN;
    m_config.sensors_period_ms = DEFAULT_SENSORS_PERIOD_MS;

    m_callback = cb;

    lib_data->setNewAppConfigCb(cb_app_config);
}

const app_config_t * App_Config_get(void)
{
    return &m_config;
}
