/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file dualmcu_lib.h
 *
 */

#ifndef _DUALMCU_LIB_H_
#define _DUALMCU_LIB_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * \brief   List of return code
 */
typedef enum
{
    /** Operation is successful */
    DUALMCU_LIB_RES_OK = 0,
    /** Something went wrong */
    DUALMCU_LIB_RES_INTERNAL_ERROR = 1,
} dualmcu_lib_res_e;


/**
 * \brief   Initialize Dualmcu_lib
 * \param   baudrate
 *          Baudrate for the uart
 * \param   flow_ctrl
 *          Is hardware flow control enabled
 * \return  Return code of the operation
 */
dualmcu_lib_res_e Dualmcu_lib_init(uint32_t baudrate, bool flow_ctrl);


#endif //_DUALMCU_LIB_H_
