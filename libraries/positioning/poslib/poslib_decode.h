/**
* @file       poslib_decode.h
* @brief      Header file for poslib_decode.c
* @copyright  Wirepas Ltd 2021
*/

#ifndef _POSLIB_DECODE_H_
#define _POSLIB_DECODE_H_

/**
 * @brief defines the available app-config commands.
 */
typedef enum
{
    POSLIB_MEASUREMENT_RATE_STATIC = 0x01,
    POSLIB_OPERATING_MODE = 0x02,
    POSLIB_MEASUREMENT_RATE_DYNAMIC = 0x03,
    POSLIB_OTAP = 0x04,
    POSLIB_MEASUREMENT_RATE_OFFLINE = 0x05,
    POSLIB_NODE_ADDR_SELECTION = 0x06,
    POSLIB_BEACON = 0x08,
	POSLIB_DEVICE_CLASS_CHANGE = 0x0A,
    POSLIB_MOTION_THRESHOLD = 0x0B,
    POSLIB_MOTION_DURATION = 0x0C,
    POSLIB_LED_ON_DURATION = 0x0E,
    POSLIB_LED_CMD_SEQ = 0x0F,
} poslib_decode_msg_e;

/**
 * @brief   Decodes received configuration.
 * @param   bytes
 *          Pointer to configuration
 * @param   num_bytes
 *          Length of the configuration
 * @param   settings
 *          Pointer to a poslib_settings_t structure where 
 *          decoded settings will be stored
 * @return  bool
 *          true: decoding succesfull; false: decoding error  
 */
bool PosLibDecode_config(const uint8_t * buffer, uint32_t length, 
                            poslib_settings_t * settings, poslib_aux_settings_t * aux);

#endif
