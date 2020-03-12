/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    storage_chipid.c
 * \brief   This file provides example for persistant storage of provisioning
 *          parameters. The chipset ID is used for the Unique ID. Only
 *          supported provisioning method is \ref PROV_METHOD_UNSECURED as
 *          there is no storage for the shared key.
 */

#include "storage.h"
#include "mcu.h"

bool Storage_init()
{
    /* Nothing to do. */
    return true;
}

int8_t Storage_getUID(const uint8_t ** uid)
{
#if defined(NRF52)
    *uid = (uint8_t *)&NRF_FICR->DEVICEID[0];
#elif defined(EFR32)
    *uid = (uint8_t *)&DEVINFO->UNIQUEL;
#else
#error MCU not supported.
#endif

    /* Chip Unique Id is 64bits. */
    return 8;
}

int8_t Storage_getKey(const uint8_t ** key)
{
    *key = NULL;

    return 0;
}

provisioning_method_e Storage_getMethod()
{
    return PROV_METHOD_UNSECURED;
}
