/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file app_persistent.h
 *
 * Application persistent library. It wraps the access of the application
 * persistent area if it exists.
 *
 * This library access the app persistent area in a basic way. It is mainly
 * designed to access in read mode as many time as app want but written only
 * very seldomly. Ideally, memory area is written first on the assembly line.
 * Here are some implementations points:
 *  - Reading more than was written the previous time is undefined. The delta
 *    may be random or 0xff
 *  - When writting the area, the minimum size is previously erased to ensure that
 *    the new data is written on clean area.
 *  - All access are synchronous with a timeout. So writting long chunks of data may
 *    be quite long (up to 100ms)
 *  - There is no protection in case of reboot during a write (no backup)
 */

#ifndef _APP_PERSISTENT_H_
#define _APP_PERSISTENT_H_

#include <stdlib.h>
#include <stdint.h>

/**
 * \brief   List of return code
 */
typedef enum
{
    /** Operation is successful */
    APP_PERSISTENT_RES_OK = 0,
    /** No area found to store persistent data */
    APP_PERSISTENT_RES_NO_AREA = 1,
    /** Library unitialized */
    APP_PERSISTENT_RES_UNINITIALIZED = 2,
    /** Read or write command is for a too big area */
    APP_PERSISTENT_RES_TOO_BIG = 3,
    /** Content of area was never initialized */
    APP_PERSISTENT_RES_INVALID_CONTENT = 4,
    /** Access to area has timeouted (unlikely on internal flash) */
    APP_PERSISTENT_RES_ACCESS_TIMEOUT = 5,
    /** Flash driver reported an error */
    APP_PERSISTENT_RES_FLASH_ERROR = 6
} app_persistent_res_e;

/**
 * \brief   Initialize app persistent module
 * \return  Return code of the operation
 */
app_persistent_res_e App_Persistent_init(void);

/**
 * \brief   Write to persistent
 * \param   data
 *          Pointer to the data to write
 * \param   len
 *          Length of data to write
 * \return  Return code of the operation
 */
app_persistent_res_e App_Persistent_write(uint8_t * data, size_t len);


/**
 * \brief   Read from persistent
 * \param   data
 *          Pointer to store read data
 * \param   len
 *          Length of data to read
 * \return  Return code of the operation
 * \note    \ref data is valid only if return code is APP_PERSISTENT_RES_OK
 */
app_persistent_res_e App_Persistent_read(uint8_t * data, size_t len);

#endif //_APP_PERSISTENT_H_
