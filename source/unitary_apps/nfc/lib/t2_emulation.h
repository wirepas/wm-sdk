/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file t2_emulation.h
 */
#ifndef __T2_EMU_H__
#define __T2_EMU_H__

/** Header Size of a Type II NFC Tag */
#define T2_HEADER_SIZE 16

/** Type 2 Tag command identifier */
typedef enum
{
    T2_EMU_READ_CMD      = 0x30u,  /** Type 2 Tag Read command identifier */
    T2_EMU_WRITE_CMD     = 0xA2u,  /** Type 2 Tag Read command identifier */
    T2_EMU_SECTOR_SELECT = 0xC2u   /** Type 2 Tag Sector Select command identifier */
} t2_emu_cmd_e;

/** NFC Type II Event */
typedef enum
{
    T2_EMU_EVENT_FIELD_ON,  /* Field is detected */
    T2_EMU_EVENT_FIELD_OFF, /* Field is lost     */
    T2_EMU_EVENT_SELECTED,  /* NFC is selected   */
    T2_EMU_DATA_WRITTEN     /* Data written      */
} t2_emu_event_e;

/** Return codes of Type II Emulation functions */
typedef enum
{
    T2_EMU_RES_OK,
    T2_EMU_RES_ERR
} t2_emu_res_e;

/**
 * \brief   User callback for event handling
 * \param   event
 *          NFC Type II event to process
 * */
typedef void (* t2_emu_callback_f)(t2_emu_event_e event);

/**
 * \brief   Force Type II Tag to be readonly
 * \param   bool
 *          if param is true, the tag will be read-only
 *          and will return NAK to any WRITE command.
 */
void t2_emu_readonly(bool readonly);

/**
 * \brief   Initialize Type II Tag Emulation.
 * \param   callback
 *          Pointer to an optional callback that will be called
 *          - on field event
 *          - when data is written on Type II Tag memory
 * \param   buffer
 *          Pointer to the Type 2 Tag simulated user memory
 * \param   size
 *          Size of the Type 2 Tag simulated user memory
 * \return  T2_EMU_OK if launched successfully.
 */
t2_emu_res_e t2_emu_init(t2_emu_callback_f callback, uint8_t *buffer, size_t size);

#endif /* __T2_EMU_H__ */
