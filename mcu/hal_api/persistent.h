/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#ifndef PERSISTENT_H_
#define PERSISTENT_H_

#define PERSISTENT_MAGIC_KEY   (uint32_t)0xA133F0E8

/**
 * \brief   List of return code
 */
typedef enum
{
    /** Operation is successful */
    PERSISTENT_RES_OK = 0,
    /** Suggested read or write data area is too long */
    PERSISTENT_RES_DATA_AREA_OVERFLOW = 1,
    /** Magic key not valid */
    PERSISTENT_RES_MAGIC_KEY_NOT_VALID = 2
} persistent_res_e;

/**
 * \brief  Reading data from persistent memory from the beginnings
 * \param  data
 *         Pointer to store raw data from persistent memory
 * \param  offset
 *         The offset to read from
 * \param  len
 *         Read the desired amount of data length
 * \return PERSISTENT_RES_OK
 *         Succeed
 *         PERSISTENT_RES_DATA_AREA_OVERFLOW
 *         The suggested read data area is too long
 */
persistent_res_e Mcu_Persistent_read(uint8_t * data, uint16_t offset, uint16_t len);

/**
 * \brief  Writing data to persistent memory
 * \param  data
 *         Pointer to data to store in persistent memory
 * \param  offset
 *         The place where from writing start if not in the beginning
 * \param  len
 *         Write the desired amount of data length
 * \return PERSISTENT_RES_OK
 *         Succeed
 *         PERSISTENT_RES_DATA_AREA_OVERFLOW
 *         The suggested write data area or offset place is too long
 */
persistent_res_e Mcu_Persistent_write(uint8_t * data,
                                      uint16_t offset,
                                      uint16_t len);

/**
 * \brief  Check persistent memory data validity by checking magic key value
 * \param  magic_key
 *         Reference key
 * \return PERSISTENT_RES_OK
 *         Succeed
 *         PERSISTENT_RES_MAGIC_KEY_NOT_VALID
 *         Magic key not valid
 */
persistent_res_e Mcu_Persistent_isValid(uint32_t magic_key);

/**
 * \brief  Get persistent memory maximum data size
 * \return The maximum size of data in bytes
 */
uint8_t Mcu_Persistent_getMaxSize();


#endif /* PERSISTENT_H_ */
