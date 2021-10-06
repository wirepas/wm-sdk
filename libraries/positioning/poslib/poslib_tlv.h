/**
 * @file  poslib_tlv.h
 * @brief Library to encode/decode buffers in PosLibTLV compact format.
 * @copyright Wirepas Ltd.2021
*/

#ifndef _POSLIB_TLV_H_
#define _POSLIB_TLV_H_

#include <stdint.h>

/**
     @brief List of return codes
*/
typedef enum
{
    POSLIB_TLV_RES_OK,    /**< Successfully decoded one TLV item from the buffer. */
    POSLIB_TLV_RES_ERROR, /**< Decode error, item is too long or lenth field is 0. */
    POSLIB_TLV_RES_END,   /**< End of buffer, nothing to decode. */
} poslib_tlv_res_e;


/**
    @brief Structure describing a Type Length Value item
*/
typedef struct
{
    uint16_t type;    /**< Type of the TLV item. */
    uint8_t * value;  /**< Pointer to the buffer containing the value */
    uint8_t length;   /**< Length of the value buffer. */
} poslib_tlv_item_t;

/**
    @brief   This structure holds the buffer containing TLV items and data to
             manage it, max. buffer size and current index.
*/
typedef struct
{
    uint8_t * buffer; /**< Pointer to a buffer containing TLV Items. */
    uint8_t length;   /**< Length of the buffer. */
    uint8_t index;    /**< Current encode/decode index in the buffer. */
} poslib_tlv_record;

/**
    @brief     Initialize a TLV record.
    @param[in] rcd Pointer to the tlv_record structure to initialize.
    @param[in] buffer Buffer used by the tlv_record.
    @param[in] length Size in bytes of the buffer.
    @note      This funtion must be used before using the
               Tlv_Decode_getNextItem or Tlv_Encode_addItem functions.
*/
void PosLibTlv_init(poslib_tlv_record * rcd, uint8_t * buffer, uint8_t length);

/**
    @brief      Decode the next tlv_item in the tlv_record passed in parameter.
    @param[in]  rcd Pointer to a tlv_record structure to decode items from.
    @param[in]  item Pointer to a tlv_item. Updated by the call if return
                code is POSLIB_TLV_RES_OK
    @return     POSLIB_TLV_RES_OK if OK, see poslib_tlv_res_e otherwise.
*/
poslib_tlv_res_e PosLibTlv_Decode_getNextItem(poslib_tlv_record * rcd,
                                              poslib_tlv_item_t * item);


/**
    @brief      Encode tlv_item in the tlv_record passed as parameter.
    @param[in]  rcd Pointer to a tlv_record structure to encode items into.
    @param[in]  item Pointer to a tlv_item to add. 
    @return     POSLIB_TLV_RES_OK if encoding possible, see poslib_tlv_res_e otherwise.
*/
poslib_tlv_res_e PosLibTlv_Encode_addItem(poslib_tlv_record * rcd,
                                            poslib_tlv_item_t * item);


#endif /* _POSLIB_TLV_H_ */
