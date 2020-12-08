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
    tlv_item_t item;
    tlv_record record;

    const app_config_t * app_cfg = App_Config_get();

    Tlv_init(&record, buffer, length);

    /* Always add counter to packet. */
    item.type = TLV_TYPE_COUNTER;
    item.length = sizeof(data->count);
    item.value = (uint8_t *) &data->count;
    tlv_ret = Tlv_Encode_addItem(&record, &item);

    if (app_cfg->temperature_enable && tlv_ret == TLV_RES_OK)
    {
        item.type = TLV_TYPE_TEMPERATURE;
        item.length = sizeof(data->temp);
        item.value = (uint8_t *) &data->temp;
        tlv_ret = Tlv_Encode_addItem(&record, &item);
    }

    if (app_cfg->humidity_enable && tlv_ret == TLV_RES_OK)
    {
        item.type = TLV_TYPE_HUMIDITY;
        item.length = sizeof(data->humi);
        item.value = (uint8_t *) &data->humi;
        tlv_ret = Tlv_Encode_addItem(&record, &item);
    }

    if (app_cfg->pressure_enable && tlv_ret == TLV_RES_OK)
    {
        item.type = TLV_TYPE_PRESSURE;
        item.length = sizeof(data->press);
        item.value = (uint8_t *) &data->press;
        tlv_ret = Tlv_Encode_addItem(&record, &item);
    }

    if (app_cfg->accel_x_enable && tlv_ret == TLV_RES_OK)
    {
        item.type = TLV_TYPE_ACCEL_X;
        item.length = sizeof(data->acc_x);
        item.value = (uint8_t *) &data->acc_x;
        tlv_ret = Tlv_Encode_addItem(&record, &item);
    }

    if (app_cfg->accel_y_enable && tlv_ret == TLV_RES_OK)
    {
        item.type = TLV_TYPE_ACCEL_Y;
        item.length = sizeof(data->acc_y);
        item.value = (uint8_t *) &data->acc_y;
        tlv_ret = Tlv_Encode_addItem(&record, &item);
    }

    if (app_cfg->accel_z_enable && tlv_ret == TLV_RES_OK)
    {
        item.type = TLV_TYPE_ACCEL_Z;
        item.length = sizeof(data->acc_z);
        item.value = (uint8_t *) &data->acc_z;
        tlv_ret = Tlv_Encode_addItem(&record, &item);
    }

    if (tlv_ret == TLV_RES_OK)
    {
        return Tlv_Encode_getBufferSize(&record);
    }

    return -1;
}
