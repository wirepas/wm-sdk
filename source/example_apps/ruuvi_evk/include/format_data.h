/**
    @file  format_data.h
    @brief This module contains functions to format sensor data to different
           format before sending them.
    @copyright  Wirepas Oy 2019
*/
 #ifndef FORMAT_DATA_H_
 #define FORMAT_DATA_H_

#include <stdint.h>

/**
    @brief This structure contains all the sensors data to be sent.
*/
typedef struct
{
    uint16_t count; /**< Counter; incremented by one each period. */
    int32_t temp;  /**< Temperature in 0.01°C. */
    uint32_t humi;  /**< Humidity in 1/1024 % relative humidity (1% is 1024). */
    uint32_t press; /**< Pressure in 0.01 Pascal. */
    int32_t acc_x;  /**< Acceleration on X axis in mg [-2g / +2g]. */
    int32_t acc_y;  /**< Acceleration on Y axis in mg [-2g / +2g]. */
    int32_t acc_z;  /**< Acceleration on Z axis in mg [-2g / +2g]. */
} sensor_data_t;

/**
    @brief      Format sensor data to TLV format.

                Formatted buffer looks like this:
                [Type]              [Length] [Value]
                [0x01: Counter]     [0x02]   [uint16_t counter]
                [0x02: Temperature] [0x04]   [int32_t  temperature]
                [0x03: Humidity]    [0x04]   [uint32_t humidity]
                [0x04: Pressure]    [0x04]   [uint32_t pressure]
                [0x05: Accel X]     [0x04]   [int32_t  accel X]
                [0x06: Accel Y]     [0x04]   [int32_t  accel Y]
                [0x07: Accel Z]     [0x04]   [int32_t  accel Z]
                Sensor data is only present in the formatted packet if it as
                been enabled in the configuration. Counter is always present.
    @param[out] buffer Buffer to store formatted data.
    @param[in]  data Pointer to the structure holding sensors data.
    @param[in]  len Length of the buffer
    @return     Size of the generated buffer. Or -1 if the buffer is to small.
*/
int format_data_tlv(uint8_t * buffer, sensor_data_t *data, int len);

#endif /* FORMAT_DATA_H_ */
