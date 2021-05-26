/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file nfc_hw.c
 *
 * low level NFC driver
 */

#include <stdlib.h>

#include "api.h"
#include "mcu.h"
#include "nfc_hw.h"
#include "app_scheduler.h"
#include "hw_delay.h"

#define MSB_32(a) (((a) & 0xFF000000) >> 24)
#define LSB_32(a) ((a) & 0x000000FF)

#define NFC_FIELDPRESENT_MASK      (NFCT_FIELDPRESENT_LOCKDETECT_Msk | \
                                    NFCT_FIELDPRESENT_FIELDPRESENT_Msk)

#define NFC_FIELDPRESENT_IS_LOST   ((NFCT_FIELDPRESENT_FIELDPRESENT_NoField <<  \
                                     NFCT_FIELDPRESENT_FIELDPRESENT_Pos) |      \
                                    (NFCT_FIELDPRESENT_LOCKDETECT_NotLocked <<  \
                                     NFCT_FIELDPRESENT_LOCKDETECT_Pos))

#define NFC_FRAMESTATUS_RX_MSK    (NFCT_FRAMESTATUS_RX_OVERRUN_Msk      | \
                                   NFCT_FRAMESTATUS_RX_PARITYSTATUS_Msk | \
                                   NFCT_FRAMESTATUS_RX_CRCERROR_Msk)

#if (MCU_SUB == 840)
/* Mask for checking FIELDPRESENT register for state: FIELD ON. */
#define NFC_FIELD_ON_MASK            NFCT_FIELDPRESENT_LOCKDETECT_Msk

/* Mask for checking FIELDPRESENT register for state: FIELD OFF. */
#define NFC_FIELD_OFF_MASK           NFCT_FIELDPRESENT_FIELDPRESENT_Msk
#endif
/* Bugfix for FTPAN-116 (IC-12886) */
#define NFC_POWER                 (*(uint32_t volatile *)(0x40005FFC))

#define NFC_DEFAULTSTATESLEEP     (*(uint32_t volatile *)(0x40005420)) /* NFCT default state. */
#define NFC_DEFAULTSTATESLEEP_MSK 0x1UL

#if (MCU_SUB == 840)
#define NFC_ERRORSTATUS_ALL    (NFCT_ERRORSTATUS_FRAMEDELAYTIMEOUT_Msk)
#elif (MCU_SUB == 832)
#define NFC_ERRORSTATUS_ALL    (NFCT_ERRORSTATUS_NFCFIELDTOOWEAK_Msk    | \
                                NFCT_ERRORSTATUS_NFCFIELDTOOSTRONG_Msk  | \
                                NFCT_ERRORSTATUS_FRAMEDELAYTIMEOUT_Msk)
#else
#error NFC Supported on nrf52840 or 52832 only.
#endif

#define NFCID1_2ND_LAST_BYTE2_SHIFT 16u
#define NFCID1_2ND_LAST_BYTE1_SHIFT 8u
#define NFCID1_2ND_LAST_BYTE0_SHIFT 0u

#define NFCID1_LAST_BYTE3_SHIFT     24u
#define NFCID1_LAST_BYTE2_SHIFT     16u
#define NFCID1_LAST_BYTE1_SHIFT     8u
#define NFCID1_LAST_BYTE0_SHIFT     0u

#define CASCADE_TAG_BYTE            0x88u /* ISO/EIC 14443-3 */
#define NFC_CRC_SIZE                2u

#define NFC_BUFFER_SIZE             16u

#if (MCU_SUB == 840)
#define CHECK_HFXO_PERIOD           100 /* us */
#endif

#if (MCU_SUB == 832)
#define CHECK_HFXO_PERIOD             1 /* ms */
#define CHECK_FIELD_PERIOD           90 /* us */
#endif

#define NFC_SLP_REQ_CMD           0x50u /* NFC SLP_REQ command identifier */

/* Check if there is a buffer to put the NFC Tag Payload */
/* And that it's 46 bytes minimum so we can put 1 NDEF Record */
#define NFC_TAG_MIN_SIZE 46

static hw_nfc_callback_f            m_nfc_lib_callback = (hw_nfc_callback_f) NULL;

/* NFCID1 Content*/
static uint8_t                      m_nfcid1_data[10] = {0};

/* NFC data */
static volatile uint8_t             m_nfc_buffer[NFC_BUFFER_SIZE] = {0};

/* NFC field present */
static volatile bool                m_nfc_field_on;

/* SLP_REQ Command received */
static volatile bool                m_slp_req_received;

/* Type 2 Tag emulated memory */
static uint8_t                      *m_nfc_internal;

#if (MCU_SUB == 832)
/* Filter field_lost event */
static uint8_t                      m_filter_field_off;
#endif

#if (MCU_SUB == 840)
/* Mask used for NFC Field polling in NFCT_FIELDPRESENT register */
static volatile uint32_t            m_nfc_fieldpresent_mask = NFC_FIELD_OFF_MASK;
#endif

/* Filter Field Event */
static uint8_t                      m_filter_field;

/* NFC Driver Initialized */
static bool m_isNfcInit = false;

/* HFXO up and running */
static bool m_HFXO_active;

/* Store ACK or NAK value to send */
static uint8_t m_Ack;

#if (MCU_SUB == 840)
/* Number of required conditions to activate NFC. */
#define NFC_ACTIVATE_CONDS_THR    2

/* Minimal delay in us between NFC field detection and activation of NFC. */
#define NFC_ACTIVATE_DELAY        1000

/* Flag indicating that field events are ignored. */
static volatile bool      m_nfc_fieldevents_filter_active;

/* Number of activation conditions that are met. */
static volatile uint32_t  m_nfc_activate_conditions;

/* Waiting for field on confirmation */
static bool m_field_validation_pending;
#endif
/* Manage Field On <-> Off transition */
static void field_event_handler(volatile nfc_hw_field_sense_state_e field_state);

#if (MCU_SUB == 832)
/* Check Field regularly, and use field_event_handler if field is lost */
static uint32_t check_field_handler(void);
#endif

#if (MCU_SUB == 840)
/* Manage Field on State machine */
static uint32_t field_activate_check(void);
#endif

/** Declare the interrupt handler */
void __attribute__((__interrupt__)) IRQ_handler(void);

/**
 * \brief   Create m_nfcid1_data based on NRF_FICR->NFC registers.
 */
static void setup_nfc_header(void)
{
    uint32_t nfc_tag_header0 = NRF_FICR->NFC.TAGHEADER0;
    uint32_t nfc_tag_header1 = NRF_FICR->NFC.TAGHEADER1;
    uint32_t nfc_tag_header2 = NRF_FICR->NFC.TAGHEADER2;

    m_nfcid1_data[0] = (uint8_t) LSB_32(nfc_tag_header0 >> 0);
    m_nfcid1_data[1] = (uint8_t) LSB_32(nfc_tag_header0 >> 8);
    m_nfcid1_data[2] = (uint8_t) LSB_32(nfc_tag_header0 >> 16);
    m_nfcid1_data[3] = (uint8_t) LSB_32(nfc_tag_header1 >> 0);
    m_nfcid1_data[4] = (uint8_t) LSB_32(nfc_tag_header1 >> 8);
    m_nfcid1_data[5] = (uint8_t) LSB_32(nfc_tag_header1 >> 16);
    m_nfcid1_data[6] = (uint8_t) LSB_32(nfc_tag_header1 >> 24);
    m_nfcid1_data[7] = (uint8_t) LSB_32(nfc_tag_header2 >> 0);
    m_nfcid1_data[8] = (uint8_t) LSB_32(nfc_tag_header2 >> 8);
    m_nfcid1_data[9] = (uint8_t) LSB_32(nfc_tag_header2 >> 16);

    if (m_nfcid1_data[3] == 0x88)
    {
        m_nfcid1_data[3] |= 0x11;
    }
}


/**
 * \brief   Initialize NFC HW Controller Register
 * \return  NFC_OK if successfully launched
 */
static nfc_hw_res_e setup_nfc_reg(void)
{
#if (MCU_SUB == 832)
    NRF_NFCT->INTENSET =
        (NFCT_INTENSET_FIELDDETECTED_Enabled << NFCT_INTENSET_FIELDDETECTED_Pos);
#endif

#if (MCU_SUB == 840)
    NRF_NFCT->INTENSET =
        (NFCT_INTENSET_FIELDDETECTED_Enabled << NFCT_INTENSET_FIELDDETECTED_Pos) |
        (NFCT_INTENSET_FIELDLOST_Enabled     << NFCT_INTENSET_FIELDLOST_Pos);
#endif

    NRF_NFCT->INTENSET =
        (NFCT_INTENSET_ERROR_Enabled << NFCT_INTENSET_ERROR_Pos) |
        (NFCT_INTENSET_SELECTED_Enabled << NFCT_INTENSET_SELECTED_Pos);

    /* Use Window Grid frame delay mode */
    NRF_NFCT->FRAMEDELAYMODE =
        (NFCT_FRAMEDELAYMODE_FRAMEDELAYMODE_WindowGrid <<
         NFCT_FRAMEDELAYMODE_FRAMEDELAYMODE_Pos) & NFCT_FRAMEDELAYMODE_FRAMEDELAYMODE_Msk;

    NRF_NFCT->NFCID1_2ND_LAST =
        ((uint32_t) m_nfcid1_data[0] << NFCID1_2ND_LAST_BYTE2_SHIFT) |
        ((uint32_t) m_nfcid1_data[1] << NFCID1_2ND_LAST_BYTE1_SHIFT) |
        ((uint32_t) m_nfcid1_data[2] << NFCID1_2ND_LAST_BYTE0_SHIFT);

    NRF_NFCT->NFCID1_LAST =
        ((uint32_t) m_nfcid1_data[3] << NFCID1_LAST_BYTE3_SHIFT) |
        ((uint32_t) m_nfcid1_data[4] << NFCID1_LAST_BYTE2_SHIFT) |
        ((uint32_t) m_nfcid1_data[5] << NFCID1_LAST_BYTE1_SHIFT) |
        ((uint32_t) m_nfcid1_data[6] << NFCID1_LAST_BYTE0_SHIFT);

    /* Begin: Bugfix for FTPAN-25 (IC-9929) */
    NRF_NFCT->SENSRES =
        (NFCT_SENSRES_NFCIDSIZE_NFCID1Double << NFCT_SENSRES_NFCIDSIZE_Pos) |
        (NFCT_SENSRES_BITFRAMESDD_SDD00100   << NFCT_SENSRES_BITFRAMESDD_Pos);
    /* End:   Bugfix for FTPAN-25 (IC-9929)*/

    return NFC_HW_RES_OK;
}


static void event_clear(volatile uint32_t * p_event)
{
    *p_event = 0;

    /* Perform read to ensure clearing is effective */
    volatile uint32_t dummy = *p_event;
    (void)dummy;
}


/**
 * \brief   Manage HFXO:
 *          called on FIELD_ON event, after activating HFXO.
 *          If HFXO is active, activate NFC Controller.
 */
#if (MCU_SUB == 832)
static void handle_hfxo(void)
{
    /* 350us is the maximum time (measured) to have HFXO activated */
    lib_hw->isPeripheralActivated(APP_LIB_HARDWARE_PERIPHERAL_HFXO,&m_HFXO_active,0);

    if (m_HFXO_active)
    {
        NRF_NFCT->TASKS_ACTIVATE = 1;
    }
}
#endif

#if (MCU_SUB == 840)
static uint32_t handle_hfxo(void)
{
    uint32_t ret = CHECK_HFXO_PERIOD;

    if (m_HFXO_active)
    {
        ret = field_activate_check();
    }
    else
    {
        /* 350 us is the maximum time (measured) to have HFXO activated */
        /* Poll every 100us until it's ready */
        lib_hw->isPeripheralActivated(APP_LIB_HARDWARE_PERIPHERAL_HFXO,
                                      &m_HFXO_active,
                                      0);
    }
    return ret;
}
#endif

#if (MCU_SUB == 832)
/**
 * \brief   Check field periodically.
 */
static uint32_t check_field_handler(void)
{
    uint32_t field_present = NRF_NFCT->FIELDPRESENT & NFC_FIELDPRESENT_MASK;

    if ((!m_HFXO_active) && (m_nfc_field_on))
    {
        handle_hfxo();
        return CHECK_FIELD_PERIOD;
    }

    /* Check Field */
    if (field_present == NFC_FIELDPRESENT_IS_LOST)
    {
        m_filter_field_off++;

        if (m_filter_field_off > 7)
        {
            /* Field is lost, stop probing it */
            field_event_handler(NFC_HW_FIELD_SENSE_STATE_OFF);
            return 0;
        }
        return CHECK_FIELD_PERIOD;
    }

    m_filter_field_off = 0;
    return CHECK_FIELD_PERIOD;
}
#endif

#if (MCU_SUB == 840)
/**
 * \brief   Manage Field on State machine:
 *          [190] NFCT: Event FIELDDETECTED may be generated too early
 */
static uint32_t field_activate_check(void)
{
    if (m_field_validation_pending)
    {
        m_field_validation_pending     = false;
        m_nfc_fieldevents_filter_active = false;

        /* Check the field status with FIELDPRESENT
         * and take action if field is lost. */
        field_event_handler(NFC_HW_FIELD_SENSE_STATE_UNKNOWN);
        return 0;
    }

    m_nfc_activate_conditions++;

    if (m_nfc_activate_conditions == NFC_ACTIVATE_CONDS_THR)
    {
        m_nfc_activate_conditions = 0;

        NRF_NFCT->TASKS_ACTIVATE    = 1;
        m_field_validation_pending = true;

        /* Run again to validate if tag has locked to the field */
        return NFC_ACTIVATE_DELAY;
    }
    else
    {
        return NFC_ACTIVATE_DELAY;
    }
}
#endif

/**
 * \brief   Handle Field off/on on/off change
 * \param   field_state
 */
static void field_event_handler(volatile nfc_hw_field_sense_state_e field_state)
{
#if (MCU_SUB == 840)
    if (m_nfc_fieldevents_filter_active)
    {
        return;
    }

    if (field_state == NFC_HW_FIELD_SENSE_STATE_UNKNOWN)
    {
        /* Probe NFC field */
        uint32_t field_present = NRF_NFCT->FIELDPRESENT;

        if (field_present & m_nfc_fieldpresent_mask)
        {
            field_state = NFC_HW_FIELD_SENSE_STATE_ON;
        }
        else
        {
            field_state = NFC_HW_FIELD_SENSE_STATE_OFF;
        }
    }
#endif
    if ((field_state == NFC_HW_FIELD_SENSE_STATE_ON) && (!m_nfc_field_on))
    {
        if (m_filter_field == 0)
        {
            m_filter_field ++;
            lib_hw->activatePeripheral(APP_LIB_HARDWARE_PERIPHERAL_HFXO);
            m_nfc_field_on = true;
#if (MCU_SUB == 832)
            m_filter_field_off = 0;

            hw_delay_trigger_us(check_field_handler, CHECK_FIELD_PERIOD);
#endif
#if (MCU_SUB == 840)
            hw_delay_trigger_us(handle_hfxo, CHECK_HFXO_PERIOD);

            m_nfc_activate_conditions       = 0;
            m_nfc_fieldevents_filter_active = true;
#endif
            if (m_nfc_lib_callback != NULL)
            {
                m_nfc_lib_callback(NFC_HW_EVENT_FIELD_ON, 0, 0, 0);
            }
        }
    }
    else if ((field_state == NFC_HW_FIELD_SENSE_STATE_OFF) && (m_nfc_field_on))
    {
        if (m_filter_field)
        {
            m_nfc_field_on = false;
            m_filter_field --;

#if (MCU_SUB == 840)
            NRF_NFCT->TASKS_SENSE = 1;
#endif

            lib_hw->deactivatePeripheral(APP_LIB_HARDWARE_PERIPHERAL_HFXO);
            m_HFXO_active = false;

            NRF_NFCT->INTENCLR =
                (NFCT_INTENCLR_RXFRAMEEND_Clear << NFCT_INTENCLR_RXFRAMEEND_Pos) |
                (NFCT_INTENCLR_RXERROR_Clear    << NFCT_INTENCLR_RXERROR_Pos);

#if (MCU_SUB == 840)
            /* Change mask to FIELD_OFF state
             * trigger FIELD_ON even if HW has not locked to the field */
            m_nfc_fieldpresent_mask = NFC_FIELD_OFF_MASK;
#endif

#if (MCU_SUB == 832)
            /* Bugfix for FTPAN-116 (IC-12886) */
            /* Reset NFC Controller to release clock */
            __DMB();
            NFC_POWER = 0;
            __DMB();
            NFC_POWER = 1;
#endif
            setup_nfc_reg();

            if ((m_nfc_lib_callback != NULL) )
            {
                m_nfc_lib_callback(NFC_HW_EVENT_FIELD_OFF, 0, 0, 0);
            }
        }
    }
}

static void default_state_reset(void)
{
    if (NFC_DEFAULTSTATESLEEP & NFC_DEFAULTSTATESLEEP_MSK)
    {
        /* Default : SLEEP_A state */
        NRF_NFCT->TASKS_GOSLEEP = 1;
    }
    else
    {
        NRF_NFCT->TASKS_GOIDLE = 1;
    }

    /* Disable RX until enabled at SELECTED) */
    NRF_NFCT->INTENCLR = NFCT_INTENCLR_RXFRAMEEND_Clear <<
                         NFCT_INTENCLR_RXFRAMEEND_Pos;
}

/**
 * \brief   Function to handle the NFC Interrupt.
 */
void __attribute__((__interrupt__)) IRQ_handler(void)
{
    nfc_hw_field_sense_state_e current_field = NFC_HW_FIELD_SENSE_STATE_NONE;
    nfc_hw_res_e ret = NFC_HW_RES_OK;
    uint32_t rx_status = 0;

    if ((NRF_NFCT->EVENTS_FIELDDETECTED) && (NRF_NFCT->INTEN & NFCT_INTEN_FIELDDETECTED_Msk))
    {
        event_clear(&NRF_NFCT->EVENTS_FIELDDETECTED);
        current_field = NFC_HW_FIELD_SENSE_STATE_ON;
    }

#if (MCU_SUB == 840)
    if (NRF_NFCT->EVENTS_FIELDLOST && (NRF_NFCT->INTEN & NFCT_INTEN_FIELDLOST_Msk))
    {
        event_clear(&NRF_NFCT->EVENTS_FIELDLOST);
        current_field =
           (current_field == NFC_HW_FIELD_SENSE_STATE_NONE) ?
                         NFC_HW_FIELD_SENSE_STATE_OFF : NFC_HW_FIELD_SENSE_STATE_UNKNOWN;
    }
#endif

    if (current_field != NFC_HW_FIELD_SENSE_STATE_NONE)
    {
       field_event_handler(current_field);
    }

    if ((NRF_NFCT->EVENTS_RXERROR) && (NRF_NFCT->INTEN & NFCT_INTEN_RXERROR_Msk))
    {
        rx_status = NRF_NFCT->FRAMESTATUS.RX;
        event_clear(&NRF_NFCT->EVENTS_RXERROR);
        /* Clear rx frame status */
        NRF_NFCT->FRAMESTATUS.RX = NFC_FRAMESTATUS_RX_MSK;
    }

    if ((NRF_NFCT->EVENTS_RXFRAMEEND) && (NRF_NFCT->INTEN & NFCT_INTEN_RXFRAMEEND_Msk))
    {
        uint32_t rx_data_size = ((NRF_NFCT->RXD.AMOUNT & NFCT_RXD_AMOUNT_RXDATABYTES_Msk) >>
                                NFCT_RXD_AMOUNT_RXDATABYTES_Pos);

        if (rx_data_size >= NFC_CRC_SIZE)
        {
            rx_data_size -= NFC_CRC_SIZE;
        }

        event_clear(&NRF_NFCT->EVENTS_RXFRAMEEND);

        if((rx_data_size == 0) || (rx_data_size > NFC_BUFFER_SIZE) || (rx_status))
        {
            ret = NFC_HW_RES_ERR;
        }
        else
        {
            if (m_nfc_buffer[0] == NFC_SLP_REQ_CMD)
            {
                /* SLP_REQ was received : this will cause FRAMEDELAYTIMEOUT error */
                m_slp_req_received = true;
                /* No need to parse incoming frames, wait for SELECTED */
                NRF_NFCT->INTENCLR = NFCT_INTENCLR_RXFRAMEEND_Clear <<
                                     NFCT_INTENCLR_RXFRAMEEND_Pos;
            }
            else
            {
                if (m_nfc_lib_callback != NULL)
                {
                    ret = m_nfc_lib_callback(NFC_HW_EVENT_DATA_RECEIVED,
                                            (void *)m_nfc_internal,
                                            (void*)m_nfc_buffer,
                                            rx_data_size);
                }
            }
        }
    }

    if (ret != NFC_HW_RES_OK)
    {
        default_state_reset();
    }

    if ((NRF_NFCT->EVENTS_TXFRAMEEND) && (NRF_NFCT->INTEN & NFCT_INTEN_TXFRAMEEND_Msk))
    {
        event_clear(&NRF_NFCT->EVENTS_TXFRAMEEND);

        /* Disable TX END event to ignore frame transmission other than READ response */
        NRF_NFCT->INTENCLR = (NFCT_INTENCLR_TXFRAMEEND_Clear << NFCT_INTENCLR_TXFRAMEEND_Pos);

        /* Set up for reception */
        NRF_NFCT->PACKETPTR          = (uint32_t) m_nfc_buffer;
        NRF_NFCT->MAXLEN             = NFC_BUFFER_SIZE;
        NRF_NFCT->TASKS_ENABLERXDATA = 1;

        if (m_nfc_lib_callback != NULL)
        {
            m_nfc_lib_callback(NFC_HW_EVENT_DATA_TRANSMITTED, 0, 0, 0);
        }
    }

    if ((NRF_NFCT->EVENTS_SELECTED) && (NRF_NFCT->INTEN & NFCT_INTEN_SELECTED_Msk))
    {
        event_clear(&NRF_NFCT->EVENTS_SELECTED);
        /* Clear also RX END and RXERROR events because SW does not take care of
         * commands which were received before selecting the tag */
        event_clear(&NRF_NFCT->EVENTS_RXFRAMEEND);
        event_clear(&NRF_NFCT->EVENTS_RXERROR);

        /* Set up registers for EasyDMA and start receiving packets */
        NRF_NFCT->PACKETPTR          = (uint32_t) m_nfc_buffer;
        NRF_NFCT->MAXLEN             = NFC_BUFFER_SIZE;
        NRF_NFCT->TASKS_ENABLERXDATA = 1;

        NRF_NFCT->INTENSET = (NFCT_INTENSET_RXFRAMEEND_Enabled << NFCT_INTENSET_RXFRAMEEND_Pos) |
                             (NFCT_INTENSET_RXERROR_Enabled    << NFCT_INTENSET_RXERROR_Pos);

        /* At this point any previous error status can be ignored */
        NRF_NFCT->FRAMESTATUS.RX = NFC_FRAMESTATUS_RX_MSK;
        NRF_NFCT->ERRORSTATUS    = NFC_ERRORSTATUS_ALL;
#if (MCU_SUB == 840)
        /* Change mask to FIELD_ON state - trigger FIELD_ON only if HW has locked to the field */
        m_nfc_fieldpresent_mask = NFC_FIELD_ON_MASK;
#endif

        if (m_nfc_lib_callback != NULL)
        {
            m_nfc_lib_callback(NFC_HW_EVENT_SELECTED, 0, 0, 0);
        }
    }

    if ((NRF_NFCT->EVENTS_ERROR) && (NRF_NFCT->INTEN & NFCT_INTEN_ERROR_Msk))
    {
        uint32_t err_status = NRF_NFCT->ERRORSTATUS;
        event_clear(&NRF_NFCT->EVENTS_ERROR);

        /* Clear FRAMEDELAYTIMEOUT error (expected HW behavior) when SLP_REQ command was received */
        if ((err_status & NFCT_ERRORSTATUS_FRAMEDELAYTIMEOUT_Msk) && m_slp_req_received)
        {
            NRF_NFCT->ERRORSTATUS = NFCT_ERRORSTATUS_FRAMEDELAYTIMEOUT_Msk;
            m_slp_req_received    = false;
        }
        /* Report any other error */
        err_status &= ~NFCT_ERRORSTATUS_FRAMEDELAYTIMEOUT_Msk;

        if (err_status)
        {
            /* Nothing to do */
        }

        /* Clear error status */
        NRF_NFCT->ERRORSTATUS = NFC_ERRORSTATUS_ALL;
    }
}


nfc_hw_res_e nfc_hw_init(hw_nfc_callback_f callback, uint8_t *buffer, size_t size)
{
    m_nfc_lib_callback = callback;

    if ((buffer == NULL) || (size < NFC_TAG_MIN_SIZE))
    {
        return NFC_HW_RES_INVALID_CONFIG;
    }

    m_nfc_internal = buffer;

    setup_nfc_header();
    setup_nfc_reg();

    m_nfc_internal[0] = m_nfcid1_data[0];    /* UID0 = Manufacturer ID */
    m_nfc_internal[1] = m_nfcid1_data[1];    /* UID1 */
    m_nfc_internal[2] = m_nfcid1_data[2];    /* UID2 */
    m_nfc_internal[3] = /* BCC0 : XOR over the 4 previous bytes */
            (uint8_t) CASCADE_TAG_BYTE ^ m_nfc_internal[0] ^ m_nfc_internal[1] ^ m_nfc_internal[2];
    m_nfc_internal[4] = m_nfcid1_data[3];    /* UID3 */
    m_nfc_internal[5] = m_nfcid1_data[4];    /* UID4 */
    m_nfc_internal[6] = m_nfcid1_data[5];    /* UID5 */
    m_nfc_internal[7] = m_nfcid1_data[6];    /* UID6 */
    m_nfc_internal[8] = /* BCC1 : XOR over the 4 previous bytes */
            m_nfc_internal[4] ^ m_nfc_internal[5] ^ m_nfc_internal[6] ^ m_nfc_internal[7];
    m_nfc_internal[9] =  0xFF;    /* Free to use */
    m_nfc_internal[10] = 0x00;    /* R-W Capability */
    m_nfc_internal[11] = 0x00;    /* R-W Capability */
    m_nfc_internal[12] = 0xE1;    /* Magic */
    m_nfc_internal[13] = 0x11;    /* NFC Forum Version */
    m_nfc_internal[14] = ((size-32)>>3); /* Tag Size / 8 */
    m_nfc_internal[15] = 0x00;    /* R-W Capability */

    /* Init HW Timer module*/
    hw_delay_init();

    return NFC_HW_RES_OK;
}


bool nfc_hw_is_field_on(void)
{
    return m_nfc_field_on;
}


nfc_hw_res_e nfc_hw_start(void)
{
    if (m_isNfcInit)
    {
        return NFC_HW_RES_ALREADY_INITIALIZED;
    }

    NRF_NFCT->ERRORSTATUS = NFC_ERRORSTATUS_ALL;
    NRF_NFCT->TASKS_SENSE = 1;

    lib_system->clearPendingFastAppIrq(NFCT_IRQn);
    lib_system->enableAppIrq(true, NFCT_IRQn, APP_LIB_SYSTEM_IRQ_PRIO_LO, IRQ_handler);

    m_isNfcInit = true;

    return NFC_HW_RES_OK;
}


nfc_hw_res_e nfc_hw_stop(void)
{
    if (!m_isNfcInit)
    {
        return NFC_HW_RES_NOT_INITIALIZED;
    }

    lib_hw->deactivatePeripheral(APP_LIB_HARDWARE_PERIPHERAL_HFXO);

    NRF_NFCT->TASKS_DISABLE = 1;

    lib_system->disableAppIrq(NFCT_IRQn);

    m_nfc_field_on = false;
    m_filter_field = 0;
    m_HFXO_active  = false;
    m_isNfcInit    = false;
#if (MCU_SUB == 840)
    m_nfc_fieldevents_filter_active = false;
    m_nfc_activate_conditions = 0;
    m_field_validation_pending = false;
#endif
    return NFC_HW_RES_OK;
}


nfc_hw_res_e nfc_hw_send(const uint8_t *pData, size_t dataLength)
{
    if (!m_nfc_field_on)
    {
        return NFC_HW_RES_INVALID_CONFIG;
    }

    if ((dataLength == 0) || (dataLength > NFC_BUFFER_SIZE))
    {
        return NFC_HW_RES_INVALID_CONFIG;
    }

    event_clear(&NRF_NFCT->EVENTS_TXFRAMEEND);

    NRF_NFCT->TXD.FRAMECONFIG = (NFCT_TXD_FRAMECONFIG_PARITY_Parity << NFCT_TXD_FRAMECONFIG_PARITY_Pos)
                              | (NFCT_TXD_FRAMECONFIG_DISCARDMODE_DiscardStart << NFCT_TXD_FRAMECONFIG_DISCARDMODE_Pos)
                              | (NFCT_TXD_FRAMECONFIG_SOF_SoF << NFCT_TXD_FRAMECONFIG_SOF_Pos)
                              | (NFCT_TXD_FRAMECONFIG_CRCMODETX_CRC16TX << NFCT_TXD_FRAMECONFIG_CRCMODETX_Pos);


    NRF_NFCT->PACKETPTR     = (uint32_t) pData;
    NRF_NFCT->TXD.AMOUNT    = (dataLength << NFCT_TXD_AMOUNT_TXDATABYTES_Pos) & NFCT_TXD_AMOUNT_TXDATABYTES_Msk;
    NRF_NFCT->INTENSET      = (NFCT_INTENSET_TXFRAMEEND_Enabled << NFCT_INTENSET_TXFRAMEEND_Pos);

    NRF_NFCT->TASKS_STARTTX = 1;

    return NFC_HW_RES_OK;
}


nfc_hw_res_e nfc_hw_send_ack(bool ack_nack)
{
    if (ack_nack)
    {
        m_Ack = 0x0Au; /* ACKnowledge if no error. */
    }
    else
    {
        m_Ack = 0x00u; /* NonAcKnowledge in case of error */
    }

    if (!m_nfc_field_on)
    {
        return NFC_HW_RES_INVALID_CONFIG;
    }

    event_clear(&NRF_NFCT->EVENTS_TXFRAMEEND);

    NRF_NFCT->PACKETPTR     = (uint32_t)(&m_Ack);
    NRF_NFCT->TXD.AMOUNT = (4 << NFCT_TXD_AMOUNT_TXDATABITS_Pos) & NFCT_TXD_AMOUNT_TXDATABITS_Msk;

    NRF_NFCT->TXD.FRAMECONFIG =   (NFCT_TXD_FRAMECONFIG_SOF_SoF << NFCT_TXD_FRAMECONFIG_SOF_Pos);

    NRF_NFCT->INTENSET      = (NFCT_INTENSET_TXFRAMEEND_Enabled << NFCT_INTENSET_TXFRAMEEND_Pos);

    NRF_NFCT->TASKS_STARTTX = 1;

    return NFC_HW_RES_OK;
}


void nfc_hw_wake_from(void)
{
    NRF_NFCT->TASKS_SENSE = 1;
    NRF_POWER->SYSTEMOFF = 1;
}


bool nfc_hw_is_nfc_reset_reason(void)
{
    bool ret = false;

    if (NRF_POWER->RESETREAS & POWER_RESETREAS_NFC_Msk)
    {
        NRF_POWER->RESETREAS |= POWER_RESETREAS_NFC_Msk;
        ret = true;
    }
    return ret;
}
