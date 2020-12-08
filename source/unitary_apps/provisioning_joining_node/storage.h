/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    storage.h
 * \brief   This file provides examples for persistant storage of provisioning
 *          parameters.
 */
#ifndef _STORAGE_H_
#define _STORAGE_H_

#include <stdint.h>
#include <stdbool.h>
#include "provisioning.h"

/**
 * \brief   Initialize the storage module.
 * \note    Select storage method (chipid or memarea) by setting storage
 *          variable in config.mk.
 * \return  false if an error occured, true otherwise.
 */
bool Storage_init();

/**
 * \brief   Get a pointer to the stored UID.
 * \param   uid
 *          A pointer to an uint8_t array.
 * \return  The length of the UID.
 * \note    Returning an int8_t is ok because UID fit in a 102 bytes packet.
 */
int8_t Storage_getUID(const uint8_t ** uid);

/**
 * \brief   Get a pointer to the stored KEY.
 * \param   key
 *          A pointer to an uint8_t array.
 * \return  The length of the KEY.
 * \note    Returning an int8_t is ok because KEY fit in a 102 bytes packet.
 */
int8_t Storage_getKey(const uint8_t ** key);

/**
 * \brief   Get the provisioning method.
 * \return  The provisioning method.
 */
provisioning_method_e Storage_getMethod();

#endif //_STORAGE_H_
