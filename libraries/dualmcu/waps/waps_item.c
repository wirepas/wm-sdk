/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <stdint.h>
#include "waps_item.h"
#include "api.h"

/** Minimum amount of items that are needed to ensure proper functionality.
 * If the minimum amount of items won't fit in the RAM then a linker error
 * is generated.
 * This minimal value is set to 6 for now but should be validated under high load
 * of a sink attached to a gateway. Anyway, all the free remaining RAM will be used
 * for additional buffer, so this value is just a message to realize that we are
 * really low on RAM
**/
#define WAPS_MIN_ITEMS          6

// Addresses determined by the linker
extern uint32_t                 __bss_end__;
extern uint32_t                 __ram_end__;
extern uint32_t *               m_used_app_ram_end;

// Memory for min items, make sure first element aligns properly
static waps_item_t __attribute__((aligned(4))) waps_item_bank[WAPS_MIN_ITEMS];

#define WAPS_RESERVED_ITEMS     2

// Limit how many free items must be in the buffer in order to allow firmware to
// send new packets to application.
static uint32_t                 m_free_waps_items;

// Memory for items, make sure first element aligns properly
//static waps_item_t __attribute__((aligned(4))) waps_item;

// List of usable (free) items
static sl_list_head_t           free_items;

// Filter used by waps to receive data. Needed here to pause reception
static waps_free_item_threshold_cb_t m_threshold_cb;

void Waps_itemInit(waps_free_item_threshold_cb_t thresold_cb,
                   uint8_t thresold_percent)
{
    uint32_t * free_ram_start = &__bss_end__;
    uint32_t * free_ram_ends = &__ram_end__;

    uint32_t i = 0;
    uint32_t free_bytes = (uint32_t)((uint8_t *)free_ram_ends -
                                      (uint8_t *)free_ram_start);

    // First the min items and then the dyn ones.
    uint32_t max_waps_items = WAPS_MIN_ITEMS;

    m_threshold_cb = thresold_cb;

    max_waps_items += (uint32_t)(free_bytes / sizeof(waps_item_t));

    m_free_waps_items = max_waps_items * thresold_percent / 100;
    // Initialize free items list
    sl_list_init(&free_items);
    // Clear items
    memset((void *)&waps_item_bank[0], 0, sizeof(waps_item_bank));
    memset((void *)free_ram_start, 0, (size_t)free_bytes);
    // Push items to free list
    // The min items first
    for(i = 0; i < WAPS_MIN_ITEMS; i++)
    {
        sl_list_push_front(&free_items, (sl_list_t *)&waps_item_bank[i]);
    }
    // and then the dyn items
    for(; i < max_waps_items; i++)
    {
        sl_list_push_front(&free_items, (sl_list_t *)free_ram_start);
        free_ram_start += (uint32_t)(sizeof(waps_item_t) / 4);
    }

    // Set correct pointer value to update ram_top pointer value
    // in _start function.
    m_used_app_ram_end = free_ram_start;
}

waps_item_t * Waps_itemReserve(waps_item_type_e type)
{
    // Reserve item from free items
    waps_item_t * item = NULL;
    lib_system->enterCriticalSection();
    if((type == WAPS_ITEM_TYPE_REQUEST) ||
       (sl_list_size(&free_items) > WAPS_RESERVED_ITEMS))
    {
        // Either a request frame, or we have memory for new indication
        item = (waps_item_t *)sl_list_pop_front(&free_items);
    }
    lib_system->exitCriticalSection();
    return item;
}

void Waps_itemFree(waps_item_t * item)
{
    // Push item to free items list
    lib_system->enterCriticalSection();
    sl_list_push_front(&free_items, (sl_list_t *)item);
    lib_system->exitCriticalSection();
    // If there is enough room in the buffer, announce firmware to send new
    // packets
    if (sl_list_size(&free_items) >= m_free_waps_items
        && m_threshold_cb != NULL)
    {
        m_threshold_cb();
    }
}
