/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    app.c
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "api.h"
#include "node_configuration.h"

#include "nfc_hw.h"
#include "t2_emulation.h"
#include "tlv.h"
#include "ndef.h"
#include "app_scheduler.h"

/* Memory used for NFC driver */
#define NFC_MEMORY_SIZE 1024
static uint8_t m_nfc_memory[NFC_MEMORY_SIZE] = {0};

/* Size of Text Header in NDEF Record */
#define TXT_HDR_SIZE 0x3

/* Set on Field Off->On transition */
static bool m_field_on;

/* Set when data is written on NFC Memory */
static bool m_data_written;


/**
 * \brief   Process NDEF message in the Type II Tag memory.
 *          If a Valid TLB block with 1 or several NDEF message are present
 *          they are decoded.
 */
static uint32_t process_ndef(void)
{
    tlv_msg_t tlv;
    tlv_res_e ret;
    uint16_t idx;
    ndef_record_t ndef;
    int sleep = 1;
    int start = 1;

    tlv.len = 0;
    tlv.offset = 0;

    /* Check if we have a valid TLV block */
    ret = tlv_decode( &m_nfc_memory[T2_HEADER_SIZE],
                      NFC_MEMORY_SIZE-T2_HEADER_SIZE,
                      &tlv );

    if ((ret == TLV_RES_BLOCK) && (tlv.len))
    {
        /* Valid TLV block found */
        idx=0;
        do
        {
            /* Parse every NDEF message */
            ndef_res_e ret2 = ndef_parse(
                              &ndef,
                              &m_nfc_memory[T2_HEADER_SIZE+tlv.offset+idx]);

            /* If "sleep" is found, power off the node */
            sleep = memcmp("sleep",
                    &m_nfc_memory[T2_HEADER_SIZE+tlv.offset+idx+ndef.payload_offset+TXT_HDR_SIZE],
                    5);

            /* If "start" is found, power off the node */
            start = memcmp("start",
                    &m_nfc_memory[T2_HEADER_SIZE+tlv.offset+idx+ndef.payload_offset+TXT_HDR_SIZE],
                    5);

            idx += ndef.length;

            if ((ret2 != NDEF_RES_OK) || (idx > tlv.len))
            break;
        } while(true);
    }

    if (!sleep)
    {
        nfc_hw_wake_from();
    }

    if(!start)
    {
        lib_state->startStack();
    }

    return APP_SCHEDULER_STOP_TASK;
}


/**
 * \brief   Callback Function called from T2 Tag Emulation state machine
 *          Called when Field is stable, lost, and after data is written
 *          to emulated type II tag memory.
 */
static void t2_callback(t2_emu_event_e event)
{
    switch (event)
    {
    case T2_EMU_EVENT_FIELD_ON:
        m_field_on = true;
        break;

    case T2_EMU_EVENT_FIELD_OFF:
        m_field_on = false;
        break;

    case T2_EMU_DATA_WRITTEN:
        m_data_written = true;
        break;

    case T2_EMU_EVENT_SELECTED:
        break;

    default:
        /* Nothing */
        break;
    }

    /* Process data written, once Field is lost */
    if ((!m_field_on) && (m_data_written))
    {
        m_data_written = false;
        App_Scheduler_addTask_execTime(process_ndef, APP_SCHEDULER_SCHEDULE_ASAP, 350);
    }
}


/**
 * \brief   Fill T2 Memory with 1 TLV block and several NDEF message.
 * \param   num
 *          Number of NDEF record to write in 1 TLV block
 */
static void fill_t2_with_ndef(uint8_t num)
{
    ndef_record_t ndef_msg;
    uint16_t ndef_msg_size;
    uint16_t tlv_msg_size = 0;
    size_t msg_size;

    /* Copy of T2 Tag RAM */
    uint8_t ndef_msg_buf[NFC_MEMORY_SIZE];

    /* ".en" Header used for NDEF text record */
    uint8_t payload[255] = {0x02, 0x65, 0x6e};

    ndef_msg.buffer = &ndef_msg_buf[0];

    for (uint8_t index=1; index <= num ;index++)
    {
        /* 1: Create Payload */
        strcpy((char*)&payload[TXT_HDR_SIZE], " :THIS IS A NDEF RECORD");
        msg_size = strlen( (const char*)&payload[TXT_HDR_SIZE]);
        payload[TXT_HDR_SIZE] = 0x30 + index;

        /* 2: Create a NDEF record, with Payload embedded in it */
        ndef_msg_size = ndef_create (
                            &ndef_msg,
                            0x01, /* TNF : Well known message */
                            (index == 1) ? true : false,    /* 1st Record */
                            (index == num ) ? true : false, /* Last Record */
                            "T", /* TYPE = Text */
                            1,   /* Type Length */
                            (const uint8_t*)payload, /* Payload */
                            msg_size+TXT_HDR_SIZE ); /* Payload Length */

        tlv_msg_size += ndef_msg_size;
        ndef_msg.buffer = &ndef_msg_buf[tlv_msg_size];
    }
    /* 3: Encapsulate all NDEF message in a TLV block.    */
    /*    tlv_encode() will copy it to Type II Tag memory */
    tlv_encode(&ndef_msg_buf[0], tlv_msg_size, &m_nfc_memory[T2_HEADER_SIZE]);
}


/**
 * \brief   Initialization callback for application
 *
 * This function is called after hardware has been initialized but the
 * stack is not yet running.
 *
 */
void App_init(const app_global_functions_t * functions)
{
    t2_emu_res_e t2ret;

    /* Basic configuration of the node with a unique node address */
    if (configureNodeFromBuildParameters() != APP_RES_OK)
    {
        /* Could not configure the node */
        /* It should not happen except if one of the config value is invalid */
        return;
    }

    /* Check if NFC power-up the board */
    nfc_hw_is_nfc_reset_reason();

    /* Init Type II Tag Emulator with T2_RAM_SIZE bytes */
    /* t2_callback will be called, from IRQ context on every Read/Write and Field event */
    t2ret = t2_emu_init(t2_callback, &m_nfc_memory[0], NFC_MEMORY_SIZE);

    if (t2ret == T2_EMU_RES_OK)
    {
        /* Fill Type II Tag memory with 3 NDEF Record in 1 TLV Block */
        fill_t2_with_ndef(3);
        /* Start NFC Controller */
        nfc_hw_start();
    }
}
