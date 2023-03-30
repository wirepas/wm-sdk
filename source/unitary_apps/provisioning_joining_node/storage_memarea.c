/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    storage_memarea.c
 * \brief   This file provides example for persistant storage of provisioning
 *          parameters. Unique ID, shared key and provisioning method are stored
 *          inside a dedicated memory area (see
 *          scratchpad_ini/scratchpad_<mcu_name>.ini file).
 */

#include "storage.h"
#include "api.h"
#define DEBUG_LOG_MODULE_NAME "STORAGE "
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#include "debug_log.h"

/** Storage organization :
 *   [0...3]                         Magic number
 *   [4]                             Method
 *   [5]                             UID length
 *   [6...uid_len+5]                 UID
 *   [uid_len+6]                     KEY length
 *   [uid_len+8...uid_len+key_len+6] KEY
 */

#define OFFSET_MAGIC 0
#define OFFSET_METHOD 4
#define OFFSET_UID_LEN 5
#define OFFSET_UID 6
#define OFFSET_KEY_LEN (m_uid_len + 6)
#define OFFSET_KEY (m_uid_len + 7)

#define MAX_UID_LEN 79
#define MAX_KEY_LEN 32

#define STORAGE_AREA_ID 0x8AE573BA
#define STORAGE_MAGIC 0x647650fb //'PROVIS'

static uint8_t m_uid_len, m_key_len;
static provisioning_method_e m_method;
static uint8_t m_node_uid[MAX_UID_LEN];
static uint8_t m_node_key[MAX_KEY_LEN];

bool read(app_lib_mem_area_id_t id, void * to, uint32_t from, size_t amount)
{
    uint32_t timeout_us = 100000;
    app_lib_mem_area_info_t info;
    uint32_t call_time;

    if (lib_memory_area->getAreaInfo(id, &info) != APP_LIB_MEM_AREA_RES_OK)
    {
        return false;
    }

    /* Round up timeout to at least one call to isBusy */
    call_time = info.flash.is_busy_call_time;

    if (timeout_us < call_time)
    {
        timeout_us = call_time;
    }

    if (lib_memory_area->startRead(id, to, from, amount) ==
                                                    APP_LIB_MEM_AREA_RES_OK)
    {
        /* Wait end of read */
        bool busy;

        do
        {
            busy = lib_memory_area->isBusy(id);
            timeout_us-=call_time;
        } while(busy == true && timeout_us > 0);

        if(busy)
        {
            return false;
        }
    }

    return true;
}


bool Storage_init()
{
    uint32_t magic;

    /* Read Magic number */
    if (!read(STORAGE_AREA_ID, &magic, OFFSET_MAGIC, 4) ||
        magic != STORAGE_MAGIC)
    {
        LOG(LVL_DEBUG, "Error Magic");
        return false;
    }

    /* Read Method. */
    if (!read(STORAGE_AREA_ID, &m_method, OFFSET_METHOD, 1) ||
        (m_method != PROV_METHOD_UNSECURED && m_method != PROV_METHOD_SECURED && m_method != PROV_METHOD_EXTENDED_UID))
    {
        LOG(LVL_DEBUG, "Error Method");
        return false;
    }

    /* Read UID. */
    if (!read(STORAGE_AREA_ID, &m_uid_len, OFFSET_UID_LEN, 1) ||
        m_uid_len == 0 ||
        m_uid_len > MAX_UID_LEN)
    {
        LOG(LVL_DEBUG, "Error uid len: %d", m_uid_len);
        return false;
    }

    if (!read(STORAGE_AREA_ID, &m_node_uid, OFFSET_UID, m_uid_len))
    {
        LOG(LVL_DEBUG, "Error Uid");
        LOG_BUFFER(LVL_DEBUG, m_node_uid, m_uid_len);
        return false;
    }

    /* Read KEY. */
    if (!read(STORAGE_AREA_ID, &m_key_len, OFFSET_KEY_LEN, 1) ||
        m_key_len > MAX_KEY_LEN)
    {
        LOG(LVL_DEBUG, "Error Key len: %d", m_key_len);
        return false;
    }

    if (!read(STORAGE_AREA_ID, &m_node_key, OFFSET_KEY, m_key_len))
    {
        LOG(LVL_DEBUG, "Error Key");
        LOG_BUFFER(LVL_DEBUG, m_node_key, m_key_len);
        return false;
    }

    return true;
}

int8_t Storage_getUID(const uint8_t ** uid)
{
    *uid = m_node_uid;
    return m_uid_len;
}

int8_t Storage_getKey(const uint8_t ** key)
{
    *key = m_node_key;
    return m_key_len;
}

provisioning_method_e Storage_getMethod()
{
    return m_method;
}
