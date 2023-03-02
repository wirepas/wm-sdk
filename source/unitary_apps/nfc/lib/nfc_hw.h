/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file nfc_hw.h
 *
 */

#ifndef __NFC_HW_H__
#define __NFC_HW_H__

/** NFC Field State*/
typedef enum
{
    NFC_HW_FIELD_SENSE_STATE_NONE,    /* Initial value indicating no NFCT Field events. */
    NFC_HW_FIELD_SENSE_STATE_OFF,     /* NFCT FIELDLOST Event has been set. */
    NFC_HW_FIELD_SENSE_STATE_ON,      /* NFCT FIELDDETECTED Event has been set. */
    NFC_HW_FIELD_SENSE_STATE_UNKNOWN  /* Both NFCT Field Events have been set - ambiguous state. */
} nfc_hw_field_sense_state_e;

/** NFC Field Event*/
typedef enum
{
    NFC_HW_EVENT_FIELD_ON,           /* Field is detected. */
    NFC_HW_EVENT_FIELD_OFF,          /* Field is lost. */
    NFC_HW_EVENT_SELECTED,           /* NFC is selected   */
    NFC_HW_EVENT_DATA_RECEIVED,      /* Data is received. */
    NFC_HW_EVENT_DATA_TRANSMITTED    /* Data is Transmitted. */
} nfc_hw_event_e;

/** Return codes of NFC functions */
typedef enum
{
    NFC_HW_RES_OK,
    NFC_HW_RES_INVALID_CONFIG,
    NFC_HW_RES_NOT_INITIALIZED,
    NFC_HW_RES_ALREADY_INITIALIZED,
    NFC_HW_RES_ERR
} nfc_hw_res_e;

/**
 * \brief   NFC callback called on FIELD ON/OFF and data transfer
 * \param   event
 *          NFC Event to have the callback called.
 * \param   p_dst
 *          pointer to destination memory :
 *            RAM area used for NFC device
 * \param   p_src
 *          pointer to source memory :
 *            RAM used by NFC controller
 * \param   data_length
 *          Bytes received by NFC controller to process.
 * \return  Result code
 *          \ref NFC_HW_RES_OK if successful
 */
typedef nfc_hw_res_e (* hw_nfc_callback_f)(nfc_hw_event_e event,
                                           uint8_t * p_dst,
                                           uint8_t * p_src,
                                           size_t data_length);

/**
 * \brief   Initialize NFC HW Controller
 * \param   callback
 *          function called on
 *          - field on/off
 *          - data received (with raw payload)
 * \param   buffer
 *          Pointer to the simulated user memory.
 * \param   size
 *          num of bytes until end of simulated  memory
 * \return  NFC_OK if successfully launched
 */
nfc_hw_res_e nfc_hw_init(hw_nfc_callback_f callback, uint8_t *buffer, size_t size);

/**
 * \brief   Start NFC controller and activate IRQ
 */
nfc_hw_res_e nfc_hw_start(void);

/**
 * \brief   Stop NFC controller and disable IRQ
 */
nfc_hw_res_e nfc_hw_stop(void);

/**
 * \brief   Send raw payload over NFC
 * \param   pData
 *          Pointer to the data to send
 * \param   dataLength
 *          Number of bytes to send..
 * \return  NFC_OK if successfully sent.
 */
nfc_hw_res_e nfc_hw_send(const uint8_t *pData, size_t dataLength);

/**
 * \brief   send a ACK or a NAK message
 * \param   ack_nack
 *          if true, send ACK, NAK otherwise
 * \return  NFC_OK if successfully sent.
 */
nfc_hw_res_e nfc_hw_send_ack(bool ack_nack);

/**
 * \brief   Return the field status (ON or OFF)
 * \return  true if ON
 */
bool nfc_hw_is_field_on(void);

/**
 * \brief   Activate Wake on NFC and Power Off the node
 */
void nfc_hw_wake_from(void);

/**
 * \brief   Check if we power on from NFC Field event
 * \return  true if NFC Wake up
 */
bool nfc_hw_is_nfc_reset_reason(void);

#endif /* __NFC_HW_H__ */
