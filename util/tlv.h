/**
    @file  tlv.h
    @brief Library to encode/decode buffers in TLV (Type Length Value) format.
    @copyright Wirepas Oy 2019
*/
#ifndef TLV_H_
#define TLV_H_

#include <stdint.h>

/**
     @brief List of return codes
*/
typedef enum
{
    TLV_RES_OK,    /**< Successfully decoded one TLV item from the buffer. */
    TLV_RES_ERROR, /**< Decode error, item is too long or lenth field is 0. */
    TLV_RES_END,   /**< End of buffer, nothing to decode. */
} tlv_res_e;


/**
    @brief Structure describing a Type Length Value item
*/
typedef struct
{
    uint8_t * value;  /**< Pointer to the buffer containing the value */
    uint16_t type;    /**< Type of the TLV item. */
    uint8_t length;   /**< Length of the value buffer. */
} tlv_item_t;

/**
    @brief   This structure holds the buffer containing TLV items and data to
             manage it, max. buffer size and current index.
*/
typedef struct
{
    uint8_t * buffer; /**< Pointer to a buffer containing TLV Items. */
    uint8_t length;   /**< Length of the buffer. */
    uint8_t index;    /**< Current encode/decode index in the buffer. */
} tlv_record;

/**
    @brief      Initialize a TLV record.
    @param[in] rcd Pointer to the tlv_record structure to initialize.
    @param[in] buffer Buffer used by the tlv_record.
    @param[in] length Size in bytes of the buffer.
    @note      This funtion must be used before using the
               Tlv_Decode_getNextItem or Tlv_Encode_addItem functions.
*/
void Tlv_init(tlv_record * rcd, uint8_t * buffer, uint8_t length);

/**
    @brief      Decode the next tlv_item in the tlv_record passed in parameter.
    @param[in]  rcd Pointer to a tlv_record structure to decode items from.
    @param[in]  item Pointer to a tlv_item. Updated by the call if return
                code is TLV_RES_OK
    @return     TLV_RES_OK if OK, see tlv_res_e otherwise.
*/
tlv_res_e Tlv_Decode_getNextItem(tlv_record * rcd, tlv_item_t * item);

/**
    @brief     Add a TLV item to the buffer contained in the TLV record.
    @param[in] rcd Pointer to a tlv_record structure
    @param[in] item Pointer to a tlv_item to add in the TLV record
    @return    TLV_RES_OK if OK or TLV_RES_ERROR if the buffer is to small to
               fit the new item.
*/
tlv_res_e Tlv_Encode_addItem(tlv_record * rcd, tlv_item_t * item);

/**
    @brief     Returns the size of the generated buffer by successive calls to
               Tlv_Encode_addItem.
    @param[in] rcd Pointer to a tlv_record structure.
    @return    Total size of the buffer in bytes.
*/
int Tlv_Encode_getBufferSize(tlv_record * rcd);

#endif /* TLV_H_ */
