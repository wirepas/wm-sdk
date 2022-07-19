/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef WAPS_ITEM_H_
#define WAPS_ITEM_H_

#include "mcu.h"
#include "waps_frames.h"
#include "util.h"
#include "sl_list.h"

/** Types of buffers: requests and indications */
typedef enum
{
    WAPS_ITEM_TYPE_REQUEST,     /**< Item is a request from lower layer */
    WAPS_ITEM_TYPE_INDICATION   /**< Item is an indication from upper layer */
} waps_item_type_e;

/** Forward declaration magick for the callbacks */
typedef struct waps_item_t waps_item_t;

/** Callback for preprocessing the frame just before attempting sending. */
typedef void (*waps_pre_tx_cb_f)(waps_item_t *);

/** Callback for performing operations after successfully sending the frame */
typedef void (*waps_post_tx_cb_f)(waps_item_t *);

/** Callback to be called when the defined thershold of free buffer is reached */
typedef void (*waps_free_item_threshold_cb_t)(void);

typedef struct waps_item_t
{
    sl_list_t           list;
    /** Creation timestamp of item */
    uint32_t            time;
    waps_pre_tx_cb_f    pre_cb;
    waps_post_tx_cb_f   post_cb;
    /** Note that the frame is reused for the reply */
    waps_frame_t        frame;
    /* Do padding automatically because waps_frame_t is packed structure which
     * size can vary */
    STRUCT_ALIGN_4(waps_frame_t);
} waps_item_t;

/**
 * \brief   Initialize common frame fields
 * \param   item
 *          Pointer to the item to initialize
 * \param   func
 *          Function code to set
 * \param   len
 *          Length to set.
 */
__STATIC_INLINE void Waps_item_init(waps_item_t * item,
                                    uint8_t func,
                                    uint8_t len)
{
    if (item != NULL)
    {
        item->pre_cb = NULL;
        item->post_cb = NULL;
        item->frame.sfunc = func;
        item->frame.splen = len;
    }
}

/**
 * \brief   Initialize dynamic item bank
 */
void Waps_itemInit(waps_free_item_threshold_cb_t thresold_cb, uint8_t thresold_percent);

/**
 * \brief   Reserve item from item bank
 * \param   type
 *          Type of item reserved
 * \return  item or NIL if no free items
 */
waps_item_t * Waps_itemReserve(waps_item_type_e type);

/**
 * \brief   Free item for item bank
 * \param   item
 *          Item to free
 */
void Waps_itemFree(waps_item_t * item);

#endif /* WAPS_ITEM_H_ */
