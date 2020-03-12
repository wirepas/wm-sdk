/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \brief   Simple minimal I2C master driver
 *          It only manages one I2C instance at a time
 */
#ifndef I2C_H_
#define I2C_H_

#include <stdbool.h>
#include <stdint.h>


/** Structure to configure the I2C */
typedef struct
{
    uint32_t  clock;      //< I2C speed in Hz
    bool      pullup;     //< activate internal pull-up on SDA and SCL
} i2c_conf_t;

/**
 * Structure to describe an I2C transfer
 * In case of Read/Write transfer, Write is executed first and then the Read.
 */
typedef struct
{
    uint8_t     address;    //< Address of I2C slave
    uint8_t *   write_ptr;  //< Pointer to bytes to write (Must be NULL for pure read)
    uint8_t     write_size; //< Number of bytes to write (Must be 0 for pure read)
    uint8_t *   read_ptr;   //< Pointer to store bytes to read (Must be NULL for pure write)
    uint8_t     read_size;  //< Number of bytes to read (Must be 0 for pure write)
    uint32_t    custom;     //< Custom param (can be used to implement state machine)
} i2c_xfer_t;

/** Return codes of I2C functions */
typedef enum
{
    I2C_RES_OK,                  //< Last operation was successful
    I2C_RES_INVALID_CONFIG,      //< Invalid initial parameters
    I2C_RES_INVALID_XFER,        //< Invalid transfer parameters
    I2C_RES_NOT_INITIALIZED,     //< Driver is not initialized
    I2C_RES_ALREADY_INITIALIZED, //< Driver already initialized
    I2C_RES_BUSY,                //< Asynchronous transfer is ongoing
    I2C_RES_ANACK,               //< Slave responded with Address Nack
    I2C_RES_DNACK,               //< Slave responded with Data Nack
    I2C_RES_BUS_HANG             //< Slave did not respond during synchronous transfer
} i2c_res_e;

/** User callback when transfer is done */
typedef void (*i2c_on_transfer_done_cb_f)(i2c_res_e res, i2c_xfer_t * xfer_p);

/**
 * \brief   Initialize I2C module
 * \param   conf_p
 *          Pointer to an i2c configuration
 * \return  Return code of operation
 */
i2c_res_e I2C_init(i2c_conf_t * conf_p);

/**
 * \brief   Close an already initialized I2C module
 * \return  Return code of operation
 */
i2c_res_e I2C_close(void);

/**
 * \brief   Initiate an I2C transfer
            In case there is a write followed by a read, the read is
            initiated by a REPEATED START.
            Transfer must be finished before starting a new one.
 * \param   xfer_p
 *          Pointer to the transfer description
 * \param   cb
 *          Callback to call at end of transfer (Can be NULL for a blocking call)
 * \return  Return code of operation
 */
i2c_res_e I2C_transfer(i2c_xfer_t * xfer_p, i2c_on_transfer_done_cb_f cb);

/**
 * \brief   Return the status of the I2C driver
 * \return  Return code of operation
 */
i2c_res_e I2C_status(void);

#endif /* I2C_H_ */
