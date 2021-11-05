/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#include "app_persistent.h"
#include "api.h"

#define DEBUG_LOG_MODULE_NAME "APP_PER_LIB"
#define DEBUG_LOG_MAX_LEVEL LVL_DEBUG
#include "debug_log.h"

#define APP_PERSISTENT_MEMORY_AREA_ID 0x8AE573BA

// Randomly generated to consider area correctly initialized
#define APP_PERSISTENT_MAGIC 0x1E75FED8

// Size to store the magic at beggining of area.
// It must be at least on write boundary for custom data to start
// on a boundary too.
static size_t m_magic_size;

static bool m_initialized = false;

static size_t m_usable_memory_size;

static app_lib_mem_area_info_t m_memory_area;

static bool active_wait_for_end_of_operation(int32_t timeout_us)
{
    app_lib_time_timestamp_hp_t timeout_end;
    bool busy, timeout_reached;

    timeout_end = lib_time->addUsToHpTimestamp(lib_time->getTimestampHp(),
                                               timeout_us);

    /* Wait for flash to be ready */
    do
    {
        busy = lib_memory_area->isBusy(APP_PERSISTENT_MEMORY_AREA_ID);
        timeout_reached = lib_time->isHpTimestampBefore(timeout_end, lib_time->getTimestampHp());
    } while(busy == true && !timeout_reached);

    return !busy;
}

static bool read(void * to,  uint32_t from, size_t amount)
{
    int32_t timeout_us;
    if (lib_memory_area->startRead(APP_PERSISTENT_MEMORY_AREA_ID,
                                   to, from, amount) != APP_LIB_MEM_AREA_RES_OK)
    {
        return false;
    }

    /* Compute timeout depending of external/internal flash */
    if (m_memory_area.external_flash)
    {
        // Most of time for external flash is on bus (SPI or I2C)
        // And probably already spent in StartRead.
        // Take a large timeout that should never be reached
        timeout_us = 100000; // 100ms
    }
    else
    {
        // Read access in internal flash are synchronous
        timeout_us = 0;
    }

    /* Wait end of read */
    return active_wait_for_end_of_operation(timeout_us);
}

/**
 * \brief Max size for the storage
 */
app_persistent_res_e App_Persistent_init(void)
{
    if (m_initialized)
    {
        return APP_PERSISTENT_RES_OK;
    }

    if (lib_memory_area->getAreaInfo(APP_PERSISTENT_MEMORY_AREA_ID, &m_memory_area) != APP_LIB_MEM_AREA_RES_OK)
    {
        return APP_PERSISTENT_RES_NO_AREA;
    }

    // Magic size must be at least uint32_t size and a multiple of writable flash size
    // to keep next region alligned too.
    // We assume that write_aligment is either lower than sizeof(uint32_t) or a multiple
    // of it.
    m_magic_size = m_memory_area.flash.write_alignment > sizeof(uint32_t) ?
                    m_memory_area.flash.write_alignment : sizeof(uint32_t);

    m_usable_memory_size = m_memory_area.area_size - m_magic_size;

    m_initialized = true;

    return APP_PERSISTENT_RES_OK;
}

app_persistent_res_e App_Persistent_read(uint8_t * data, size_t len)
{
    uint32_t magic;

    if (!m_initialized)
    {
        return APP_PERSISTENT_RES_UNINITIALIZED;
    }

    if (len > m_usable_memory_size)
    {
        return APP_PERSISTENT_RES_TOO_BIG;
    }

    /* First read the magic */
    if (!read(&magic, 0, 4))
    {
        return APP_PERSISTENT_RES_FLASH_ERROR;
    }

    LOG(LVL_DEBUG, "Magic is 0x%x\n", magic);
    /* Check Magic */
    if (magic != APP_PERSISTENT_MAGIC)
    {
        return APP_PERSISTENT_RES_INVALID_CONTENT;
    }

    /* Access data just after magic */
    if (!read(data, m_magic_size, len))
    {
        return APP_PERSISTENT_RES_FLASH_ERROR;
    }

    return APP_PERSISTENT_RES_OK;
}

app_persistent_res_e App_Persistent_write(uint8_t * data, size_t len)
{
    size_t erase_block_size = m_memory_area.flash.erase_sector_size;
    uint32_t sector_base = 0;
    uint32_t timeout;
    size_t write_alignement = m_memory_area.flash.write_alignment;
    uint32_t magic = APP_PERSISTENT_MAGIC;

    if (!m_initialized)
    {
        return APP_PERSISTENT_RES_UNINITIALIZED;
    }

    if (len > m_usable_memory_size)
    {
        return APP_PERSISTENT_RES_TOO_BIG;
    }

    // Erase the minimum number of blocks for a given area
    size_t num_block = ((len + erase_block_size - 1) / erase_block_size );
    // Copy it as next function will update it
    size_t num_block_temp = num_block;

    if (lib_memory_area->startErase(APP_PERSISTENT_MEMORY_AREA_ID, &sector_base, &num_block_temp) 
        != APP_LIB_MEM_AREA_RES_OK)
    {
        return APP_PERSISTENT_RES_FLASH_ERROR;
    }

    // Determine the timeout dynamically with 100% margin (x2)
    timeout = (m_memory_area.flash.sector_erase_time * num_block) * 2;

    // Wait end of erase
    if (!active_wait_for_end_of_operation(timeout))
    {
        return APP_PERSISTENT_RES_ACCESS_TIMEOUT;
    }

    // align len to write alignement
    // undesired out of boundary data can be saved to flash but easier for app usage
    len = ((len + write_alignement - 1) / write_alignement) * write_alignement;

    // Write new data
    if (lib_memory_area->startWrite(APP_PERSISTENT_MEMORY_AREA_ID,
                                    m_magic_size,
                                    data,
                                    len) 
        != APP_LIB_MEM_AREA_RES_OK)
    {
        return APP_PERSISTENT_RES_FLASH_ERROR;
    }

    // Determine the timeout dynamically with 100% margin (x2)
    timeout = ((m_memory_area.flash.byte_write_time
                + m_memory_area.flash.byte_write_call_time) * len) * 2;

    // Wait end of write
    if (!active_wait_for_end_of_operation(timeout))
    {
        return APP_PERSISTENT_RES_ACCESS_TIMEOUT;
    }

    // Write magic back
    if (lib_memory_area->startWrite(APP_PERSISTENT_MEMORY_AREA_ID,
                                    0,
                                    &magic,
                                    m_magic_size)
        != APP_LIB_MEM_AREA_RES_OK)
    {
        return APP_PERSISTENT_RES_FLASH_ERROR;
    }

    // Determine the timeout dynamically with 100% margin (x2)
    timeout = ((m_memory_area.flash.byte_write_time
                + m_memory_area.flash.byte_write_call_time) * m_magic_size) * 2;

    // Wait end of write
    if (!active_wait_for_end_of_operation(timeout))
    {
        return APP_PERSISTENT_RES_ACCESS_TIMEOUT;
    }

    return APP_PERSISTENT_RES_OK;
}

