/**
    @file  app.c
    @brief Reference application of the Ruuvi evaluation kit.
    @copyright  Wirepas Oy 2019
*/
#include <stdio.h>
#include <string.h>

#include "api.h"
#include "node_configuration.h"
#include "tlv.h"
#include "app_scheduler.h"
#include "board.h"
#include "spi.h"
#include "power.h"

#include "bme280_wrapper.h"
#include "lis2dh12_wrapper.h"
#include "app_config.h"
#include "format_data.h"


/* Endpoints on which sensor data is sent. */
#define SENSOR_DATA_DST_ENDPOINT 11
#define SENSOR_DATA_SRC_ENDPOINT 11

/* State machine to manage sensors measurement start and read data.*/
typedef enum
{
    SENSOR_TASK_STATE_START_MEAS, /**< Start the sensor measurements. */
    SENSOR_TASK_STATE_SEND_DATA   /**< Read the measurement and send them. */
} sensor_task_state_e;

/* State of the application.*/
sensor_task_state_e m_task_state;
/* Read sensors data. */
sensor_data_t m_sensor_data;

/**
    @brief     initialize SPI driver and sensors chip select pins.
*/
static bool ruuvi_spi_init(void)
{
    spi_res_e res;
    spi_conf_t conf;

    /* Initialize LIS2DH12 Chip select pin. */
    nrf_gpio_pin_dir_set(BOARD_SPI_LIS2DH12_CS_PIN, NRF_GPIO_PIN_DIR_OUTPUT);
    nrf_gpio_cfg_output(BOARD_SPI_LIS2DH12_CS_PIN);
    nrf_gpio_pin_set(BOARD_SPI_LIS2DH12_CS_PIN);

    /* Initialize BME280 Chip select pin. */
    nrf_gpio_pin_dir_set(BOARD_SPI_BME280_CS_PIN, NRF_GPIO_PIN_DIR_OUTPUT);
    nrf_gpio_cfg_output(BOARD_SPI_BME280_CS_PIN);
    nrf_gpio_pin_set(BOARD_SPI_BME280_CS_PIN);

    /* Initialize SPI driver. */
    conf.bit_order = SPI_ORDER_MSB;
    conf.clock = 4000000;
    conf.mode = SPI_MODE_HIGH_FIRST;
    res = SPI_init(&conf);
    if ((res != SPI_RES_OK) && (res != SPI_RES_ALREADY_INITIALIZED))
    {
        return false;
    }

    return true;
}


/**
    @brief     Sends the sensors data.
    @param[in] data Pointer to the structure containing sensor data to send.
*/
static void send_data(sensor_data_t *data)
{
    app_lib_state_route_info_t route_info;
    uint8_t buff[102];
    int len = 102;

    /* For now the only supported format is TLV */
    len = format_data_tlv(buff, data, len);

    /* Only send data if there is a route to the Sink. */
    app_res_e res = lib_state->getRouteInfo(&route_info);
    if (res == APP_RES_OK && route_info.state == APP_LIB_STATE_ROUTE_STATE_VALID
        && len != -1)
    {
        app_lib_data_to_send_t data_to_send;
        data_to_send.bytes = (const uint8_t *) buff;
        data_to_send.num_bytes = len;
        data_to_send.dest_address = APP_ADDR_ANYSINK;
        data_to_send.src_endpoint = SENSOR_DATA_SRC_ENDPOINT;
        data_to_send.dest_endpoint = SENSOR_DATA_DST_ENDPOINT;
        data_to_send.qos = APP_LIB_DATA_QOS_HIGH;
        data_to_send.delay = 0;
        data_to_send.flags = APP_LIB_DATA_SEND_FLAG_NONE;
        data_to_send.tracking_id = APP_LIB_DATA_NO_TRACKING_ID;

        /* Send the data packet. */
        lib_data->sendData(&data_to_send);
    }
}

/**
    @brief     Sensor task. Manage the sensors and sends the measurement to
               the Sink.
*/
static uint32_t sensor_task()
{
    const app_config_t * cfg = App_Config_get();
    uint32_t time_to_run;

    switch (m_task_state)
    {
        case SENSOR_TASK_STATE_START_MEAS:
        {
            time_to_run = APP_SCHEDULER_SCHEDULE_ASAP;
            m_task_state = SENSOR_TASK_STATE_SEND_DATA;

            if(cfg->temperature_enable ||
               cfg->humidity_enable ||
               cfg->pressure_enable )
            {
                time_to_run = BME280_wrapper_startMeasurement();
            }

            if(cfg->accel_x_enable ||
               cfg->accel_y_enable ||
               cfg->accel_z_enable )
            {
                uint32_t lis2dh12_time_to_run;
                lis2dh12_time_to_run = LIS2DH12_wrapper_startMeasurement();

                if(lis2dh12_time_to_run > time_to_run)
                {
                    time_to_run = lis2dh12_time_to_run;
                }
            }
            break;
        }
        case SENSOR_TASK_STATE_SEND_DATA:
        {
            /* Counter is always enabled. Increment it each period. */
            m_sensor_data.count++;

            if (cfg->temperature_enable ||
                cfg->humidity_enable ||
                cfg->pressure_enable )
            {
                bme280_wrapper_measurement_t measurement;
                BME280_wrapper_readMeasurement(&measurement);

                m_sensor_data.temp = measurement.temperature;
                m_sensor_data.press = measurement.pressure;
                m_sensor_data.humi = measurement.humidity;
            }

            if(cfg->accel_x_enable ||
               cfg->accel_y_enable ||
               cfg->accel_z_enable )
            {
                lis2dh12_wrapper_measurement_t measurement;
                LIS2DH12_wrapper_readMeasurement(&measurement);

                m_sensor_data.acc_x = measurement.accel_x;
                m_sensor_data.acc_y = measurement.accel_y;
                m_sensor_data.acc_z = measurement.accel_z;
            }

            send_data(&m_sensor_data);

            m_task_state = SENSOR_TASK_STATE_START_MEAS;
            time_to_run = cfg->sensors_period_ms;
            break;
        }
        default:
        {
            m_task_state = SENSOR_TASK_STATE_START_MEAS;
            time_to_run = cfg->sensors_period_ms;
            break;
        }
    }

    return time_to_run;
}

/**
    @brief Function called when the application configuration as changed.
*/
static void on_config_update(void)
{
    const app_config_t * cfg = App_Config_get();

    if(cfg->sensors_period_ms == 0)
    {
        /* Cancel sensor task. Don't send sensor data anymore. */
        App_Scheduler_cancelTask(sensor_task);
    }
    else
    {
        /* Config changed, schedule Sensor task ASAP. */
        App_Scheduler_addTask_execTime(sensor_task, APP_SCHEDULER_SCHEDULE_ASAP, 100);
    }
}

/**
    @brief   Initialization callback for application

    This function is called after hardware has been initialized but the
    stack is not yet running.
*/
void App_init(const app_global_functions_t * functions)
{
    /* Basic configuration of the node with a unique node address. */
    if (configureNodeFromBuildParameters() != APP_RES_OK)
    {
        /*
         * Could not configure the node.
         * It should not happen except if one of the config value is invalid.
         */
        return;
    }

    m_sensor_data.count = 0;
    m_task_state = SENSOR_TASK_STATE_START_MEAS;

    /* Initialize all the modules. */
    App_Config_init(on_config_update);

    ruuvi_spi_init();
    BME280_wrapper_init();
    LIS2DH12_wrapper_init();

    /* Launch the sensor task. */
    App_Scheduler_addTask_execTime(sensor_task, APP_SCHEDULER_SCHEDULE_ASAP, 100);

    /*
     * Start the stack.
     * This is really important step, otherwise the stack will stay stopped and
     * will not be part of any network. So the device will not be reachable
     * without reflashing it
     */
    lib_state->startStack();
}
