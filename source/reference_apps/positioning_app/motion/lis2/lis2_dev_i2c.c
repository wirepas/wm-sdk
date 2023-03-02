/**
 * @file  lis2_i2c.c
 * @brief Defines the I2C read/write functions for STMems device.
 * @copyright  Wirepas Ltd 2021
 */

#define DEBUG_LOG_MODULE_NAME "LIS2I2C"
#ifdef DEBUG_POSLIB_LOG_MAX_LEVEL
#define DEBUG_LOG_MAX_LEVEL DEBUG_POSLIB_LOG_MAX_LEVEL
#else
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#endif

#include "string.h"
#include "board.h"
#include "hal_api.h"
#include "debug_log.h"
#include "i2c.h"
#include "api.h"
#include "lis2_dev.h"

#ifdef LIS2DH12_I2C
#define LIS2_I2C_ADD_L LIS2DH12_I2C_ADD_L
#define LIS2_I2C_ADD_H LIS2DH12_I2C_ADD_H
#define BOARD_I2C_LIS2_SA0  BOARD_I2C_LIS2DH12_SA0
#elif defined LIS2DW12_I2C
#define LIS2_I2C_ADD_L LIS2DW12_I2C_ADD_L
#define LIS2_I2C_ADD_H LIS2DW12_I2C_ADD_H
#define BOARD_I2C_LIS2_SA0  BOARD_I2C_LIS2DW12_SA0
#endif

/** Maximum I2C write transfer */
#define MAX_WRITE_SIZE             16

/** Maximum I2C read transfer */
#define MAX_READ_SIZE              16

static i2c_conf_t m_i2c_conf = {
    .clock = 100000,
    .pullup = BOARD_I2C_PIN_PULLUP,
};
static uint8_t m_lis2_address = 0;

/**
    @brief      Write with I2C (function required by STMicroelectronics lib).
    @param[in]  handle I2C driver id (unused here).
    @param[in]  reg First register to write to.
    @param[out] bufp Pointer in RAM to the data to be written.
    @param[in]  len Number of registers (of 1 byte) to write.
*/
static int32_t lis2_writeI2C(void * handle,
                                  uint8_t reg,
                                  uint8_t * bufp,
                                  uint16_t len)
{
    (void)handle;
    i2c_res_e res;
    uint8_t tx[MAX_WRITE_SIZE + 1];

    tx[0] = reg;
    memcpy(&tx[1], bufp, len);
    res = I2C_init(&m_i2c_conf);

    //if (res == I2C_RES_OK || res == I2C_RES_ALREADY_INITIALIZED)
    {
        i2c_xfer_t xfer_tx = {
        .address = m_lis2_address,
        .write_ptr = tx,
        .write_size = len + 1,
        .read_ptr = NULL,
        .read_size = 0};

        res = I2C_transfer(&xfer_tx, NULL);
        I2C_close();
    }

    //LOG(LVL_DEBUG, "I2C write. res:%u reg: 0x%x len: %u", res, reg, len);
    return res;
}

/**
    @brief     Read from I2C (function required by STMicroelectronics lib).
    @param[in] handle I2C driver id (unused here).
    @param[in] reg First register to read.
    @param[in] bufp Pointer to store read registers.
    @param[in] len Number of registers (of 1 byte) to read.
*/
static int32_t lis2_readI2C(void * handle,
                                 uint8_t reg,
                                 uint8_t * bufp,
                                 uint16_t len)
{
    (void)handle;
    i2c_res_e res;
    uint8_t rx[MAX_READ_SIZE + 1];
    uint8_t tx[MAX_WRITE_SIZE + 1];
    tx[0] = (len > 1) ? (reg | 0x80)  : reg;  //set the MSB for multi-byte reads

    res = I2C_init(&m_i2c_conf);
    //LOG(LVL_DEBUG, "I2C read - init res:%u", res);

    if (res == I2C_RES_OK || res == I2C_RES_ALREADY_INITIALIZED)
    {
        i2c_xfer_t xfer_rx = {
        .address =  m_lis2_address,          
        .write_ptr = tx,          
        .write_size = 1,         
        .read_ptr = rx,     
        .read_size = len,
        .custom = 0};

        res = I2C_transfer(&xfer_rx, NULL);
        memcpy(bufp, &rx[0], len);
    }
    I2C_close();

    //LOG(LVL_DEBUG, "I2C read. res:%u reg: 0x%x len: %u", res, reg, len);
    return res;
}


void LIS2_dev_init(stmdev_ctx_t * dev)
{
    m_lis2_address = (BOARD_I2C_LIS2_SA0 == 0) ? LIS2_I2C_ADD_L >> 1 : 
                                                    LIS2_I2C_ADD_H >> 1;
    dev->handle = NULL;
    dev->write_reg = lis2_writeI2C;
    dev->read_reg = lis2_readI2C;
}