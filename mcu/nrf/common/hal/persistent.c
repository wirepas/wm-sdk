/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    persistent.c
 * \brief   Board-specific UICR module for nrf52
 */
#include <stdint.h>
#include <string.h>

#include "api.h"
#include "persistent.h"
#include "mcu.h"

#define KEY_SLOT  31                                  // Magic key slot index
#define MAX_CUSTOMER_DATA_AREA (sizeof(uint32_t)*31)  // Reserved for user

/**
 * \brief  Ensure that writing/reading/erasing are not enable at the same time
 */
static void isBusy()
{
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {}
}

/**
 * \brief  Erasing User Information Configuration Registers (UICR)
 */
static void uicr_erase()
{
    // Erase enabled
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos);
    isBusy();

    NRF_NVMC->ERASEUICR = 1; // perform erase of UICR only
    while(NRF_NVMC->READY == NVMC_READY_READY_Busy || NRF_NVMC->ERASEUICR == 1);
}

persistent_res_e Mcu_Persistent_read(uint8_t * data, uint16_t offset, uint16_t len)
{
    if ((offset + len) > MAX_CUSTOMER_DATA_AREA)
    {
        return PERSISTENT_RES_DATA_AREA_OVERFLOW;
    }

    memcpy(data, (uint8_t *)&NRF_UICR->CUSTOMER[0] + offset, len);

    return PERSISTENT_RES_OK;
}

persistent_res_e Mcu_Persistent_write(uint8_t * data,
                                      uint16_t offset,
                                      uint16_t len)
{
    uint8_t i = 0, j = 0;
    uint8_t buffer[MAX_CUSTOMER_DATA_AREA] = {'\0'};

    if ((len > MAX_CUSTOMER_DATA_AREA) || (offset >= MAX_CUSTOMER_DATA_AREA) ||
        ((offset + len) > MAX_CUSTOMER_DATA_AREA))
    {
        return PERSISTENT_RES_DATA_AREA_OVERFLOW;
    }

    // Save it temporarily all data from UICR
    memcpy(&buffer[0], (uint8_t *)&NRF_UICR->CUSTOMER[0], MAX_CUSTOMER_DATA_AREA);

    // Add the user data from the desired position with the desired length
    memcpy(&buffer[offset], data, len);

    // UICR must be erased before it is possible to rewrite
    uicr_erase();

    // Enable write
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
    isBusy();

    // Write back all data also the new ones
    for (i = 0, j = 0; i < MAX_CUSTOMER_DATA_AREA; j++)
    {
        isBusy();
        // Write a full 32-bit word to a word-aligned address
        memcpy((uint32_t *)&NRF_UICR->CUSTOMER[j],
               (uint8_t *)&buffer[i],
               sizeof(uint32_t));

        i = i + 4;
    }
    isBusy();

    // Enable write and save the Magic Key to the last UICR memory place
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
    isBusy();
    NRF_UICR->CUSTOMER[KEY_SLOT] = (uint32_t )PERSISTENT_MAGIC_KEY;
    isBusy();
    // Enable read
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);
    isBusy();

    return PERSISTENT_RES_OK;
}

persistent_res_e Mcu_Persistent_isValid(uint32_t magic_key)
{
    if ((uint32_t) (NRF_UICR->CUSTOMER[31] == magic_key))
    {
        return PERSISTENT_RES_OK;
    }
    return PERSISTENT_RES_MAGIC_KEY_NOT_VALID;
}

uint8_t Mcu_Persistent_getMaxSize()
{
    // Data is stored from CUSTOMER[0] to CUSTOMER[30]
    // Magic key is stored in CUSTOMER[31]
    return MAX_CUSTOMER_DATA_AREA;
}
