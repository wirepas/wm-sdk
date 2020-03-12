/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file t2_emulation.c
 *
 */

#include <stdlib.h>
#include "api.h"

#include "nfc_hw.h"
#include "t2_emulation.h"


/* User callback for NFC Type II Event */
static t2_emu_callback_f m_t2emu_callback = (t2_emu_callback_f) NULL;

/* Param for nfc_hw_send_ack() when no error */
#define NFC_ACK true

/* Param for nfc_hw_send_ack() when error */
#define NFC_NAK false

/**
 * \brief   Type II Tag Emulation (run in IRQ context)
 * \param   event
 *          event received by low level IRQ, and dispatched to this callback.
 *          could be field event, or data received/send event.
 *          data received are parsed to emulated a Type II Tag.
 * \param   p_dst
 *          Pointer to the Type 2 Tag simulated user memory
 * \param   pData
 *          Pointer to NFC buffer filled by NFC controller
 * \param   data_length
 *          Number of bytes received.
 * \return  NFC_OK, or NFC_ERR if Error occurs
 */
static nfc_hw_res_e nfc_t2_callback(nfc_hw_event_e event, uint8_t * p_dst, uint8_t * pData, size_t data_length)
{
    nfc_hw_res_e ret = NFC_HW_RES_OK;

    uint8_t BNo, i, c;

    if (event == NFC_HW_EVENT_FIELD_ON)
    {
        if (m_t2emu_callback != NULL)
        {
            m_t2emu_callback(T2_EMU_EVENT_FIELD_ON);
        }
        return ret;
    }

    if (event == NFC_HW_EVENT_FIELD_OFF)
    {
        if (m_t2emu_callback != NULL)
        {
            m_t2emu_callback(T2_EMU_EVENT_FIELD_OFF);
        }
        return ret;
    }

    if (event == NFC_HW_EVENT_DATA_RECEIVED)
    {
        t2_emu_cmd_e t2_cmd = pData[0];

        switch (t2_cmd)
        {
            case T2_EMU_READ_CMD :
                BNo = pData[1];
                nfc_hw_send(&p_dst[BNo*4], 16);
            break;

            case T2_EMU_WRITE_CMD :
                BNo = pData[1];

                if ((data_length == 6) && (BNo > 3))
                {
                    for (i=0;i<4;i++)
                    {
                        c = pData[2+i];
                        p_dst[(BNo*4) + i] = c;
                    }

                    if (m_t2emu_callback != NULL)
                    {
                        m_t2emu_callback(T2_EMU_DATA_WRITTEN);
                    }

                    /* ACK for write command */
                    nfc_hw_send_ack(NFC_ACK);
                }
                else
                {
                    /* NAK for write command */
                    nfc_hw_send_ack(NFC_NAK);
                }
            break;

            case T2_EMU_SECTOR_SELECT :
                nfc_hw_send_ack(NFC_NAK);
            break;

            default:
                /* Nothing, handled by IRQ */
            break;
        }
    return ret;
    }

    if (event == NFC_HW_EVENT_DATA_TRANSMITTED)
    {
        /* Nothing */
        return ret;
    }
    return ret;
}


t2_emu_res_e t2_emu_init(t2_emu_callback_f callback, uint8_t *buffer, uint16_t size)
{
        nfc_hw_res_e ret = nfc_hw_init(nfc_t2_callback, buffer, size);

        if ((callback) && (ret == NFC_HW_RES_OK))
        {
            m_t2emu_callback = callback;
        }
        else
        {
            ret=NFC_HW_RES_ERR;
        }

        if (ret == NFC_HW_RES_OK)
        {
            return T2_EMU_RES_OK;
        }
        else
        {
            return T2_EMU_RES_ERR;
        }
}
