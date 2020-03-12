/**
    @file  format_data.c
    @brief This module contains functions to format data to different format
           before sending them.
    @copyright  Wirepas Oy 2019
*/
#include "format_data.h"
#include "app_config.h"
#include "tlv.h"

/**
 * @brief   List of sensor types for TLV packet format.
 */
typedef enum
{
    TLV_TYPE_COUNTER     = 0x01,
    TLV_TYPE_TEMPERATURE = 0x02,
    TLV_TYPE_HUMIDITY    = 0x03,
    TLV_TYPE_PRESSURE    = 0x04,
    TLV_TYPE_ACCEL_X     = 0x05,
    TLV_TYPE_ACCEL_Y     = 0x06,
    TLV_TYPE_ACCEL_Z     = 0x07,
} sensor_tlv_type_e;

int format_data_tlv(uint8_t * buffer, sensor_data_t *data, int length)
{
    tlv_res_e tlv_ret = TLV_RES_ERROR;
    tlv_record record;

    const app_config_t * app_cfg = App_Config_get();

    Tlv_init(&record, buffer, length);

    /* Always add counter to packet. */
    tlv_ret = Tlv_Encode_addItem(&record, TLV_TYPE_COUNTER,
                                          sizeof(data->count),
                                          &data->count);

    if (app_cfg->temperature_enable && tlv_ret == TLV_RES_OK)
    {
        tlv_ret = Tlv_Encode_addItem(&record, TLV_TYPE_TEMPERATURE,
                                           sizeof(data->temp),
                                           &data->temp);
    }

    if (app_cfg->humidity_enable && tlv_ret == TLV_RES_OK)
    {
        tlv_ret = Tlv_Encode_addItem(&record, TLV_TYPE_HUMIDITY,
                                           sizeof(data->humi),
                                           &data->humi);
    }

    if (app_cfg->pressure_enable && tlv_ret == TLV_RES_OK)
    {
        tlv_ret = Tlv_Encode_addItem(&record, TLV_TYPE_PRESSURE,
                                           sizeof(data->press),
                                           &data->press);
    }

    if (app_cfg->accel_x_enable && tlv_ret == TLV_RES_OK)
    {
        tlv_ret = Tlv_Encode_addItem(&record, TLV_TYPE_ACCEL_X,
                                           sizeof(data->acc_x),
                                           &data->acc_x);
    }

    if (app_cfg->accel_y_enable && tlv_ret == TLV_RES_OK)
    {
        tlv_ret = Tlv_Encode_addItem(&record, TLV_TYPE_ACCEL_Y,
                                           sizeof(data->acc_y),
                                           &data->acc_y);
    }

    if (app_cfg->accel_z_enable && tlv_ret == TLV_RES_OK)
    {
        tlv_ret = Tlv_Encode_addItem(&record, TLV_TYPE_ACCEL_Z,
                                           sizeof(data->acc_z),
                                           &data->acc_z);
    }

    if (tlv_ret == TLV_RES_OK)
    {
        return Tlv_Encode_getBufferSize(&record);
    }

    return -1;
}
