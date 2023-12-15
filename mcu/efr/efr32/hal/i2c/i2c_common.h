/* Copyright 2023 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#ifndef I2C_COMMON_H_
#define I2C_COMMON_H_


#define USE_I2C1

/** Internal transfer description */
typedef struct
{
    i2c_xfer_t *                client_xfer;    //< Transfer asked by client
    i2c_on_transfer_done_cb_f   cb;     //< Callback to call at end of transfer
    i2c_res_e                   res;            //< Result of I2C transfer
    uint8_t                     pos;            //< Read or write position
    bool                        free;           //< False if transfer ongoing
    bool                        write_done;     //< Is write before read done
    bool                        done;           //< Is transfer done
    bool                        mstop_detected; //< Is MSTOP interrupt detected
} internal_xfer_desc;

/** I2C state machine state */
typedef enum
{
    I2C_SM_ST_UNINITIALIZED = 0,
    I2C_SM_ST_IDLE,
    I2C_SM_ST_START,
    I2C_SM_ST_WRITE_ADDR,
    I2C_SM_ST_WRITE_DATA,
    I2C_SM_ST_READ,
    I2C_SM_ST_STOP
} i2c_sm_state_e;

/** I2C state machine commands */
typedef enum
{
    I2C_SM_CMD_ABORT = 0,
    I2C_SM_CMD_START,
    I2C_SM_CMD_NEXT
} i2c_sm_command_e;


#endif //I2C_COMMON_H_
