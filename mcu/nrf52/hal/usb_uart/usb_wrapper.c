/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    usb.c
 * \brief   This file is the Wirepas wrapper for tinyusb stack
 */

#include <stdlib.h>

#include "api.h"
#include "tusb.h"
#include "tusb_config.h"

#include "mcu.h"
#include "board.h"
#include "uart_print.h"
#include "app_scheduler.h"
#include "usb_wrapper.h"

/** \brief  Callback when bytes are received */
static Usb_wrapper_rx_callback_f m_rx_cb;

// Wait up to 1s for VBUS to stabilize
#define MAX_DELAY_VBUS_READY_MS 1000

/** \brief  Maximum execution time for USB task (max value measured = 200us)*/
#define USB_TASK_EXEC_TIME_US   400

/** \brief  tinyusb function that handles power event (detected, ready, removed) */
extern void tusb_hal_nrf_power_event(uint32_t event);

/**
 * \brief  temporary buffer. As CFG_TUD_CDC_RX_BUFSIZE can be up to 512 bytes,
 *         do not allocate it on stack
 */
static uint8_t m_rx_buf[CFG_TUD_CDC_RX_BUFSIZE];

/**
 * \brief   Ask data from tinyusb stack and forward it to upper layer
 */
static void read_from_usb(void)
{
    if ( tud_cdc_connected() )
    {
        // connected and there are data available
        if ( tud_cdc_available() )
        {
            // read and call upper layer
            uint32_t count = tud_cdc_read(m_rx_buf, sizeof(m_rx_buf));
            m_rx_cb(m_rx_buf, count);
        }
    }
}

/**
 * \brief   Task to execute all work that is not execute din interrupt context
 */
static uint32_t usb_task()
{
    // tinyusb device task
    tud_task();
    // read from usb and dispatch it
    read_from_usb();

    return APP_SCHEDULER_STOP_TASK;
}

/*
 * \brief USB IRQ handler
 *        It is capture here so that we can schedule usb_task in case
 *        something has to be proceed (Rx). It will always happen from interrupt.
 */
void USBD_IRQHandler(void)
{
    // Call back tinyusb low level handler
    tud_int_handler(0);
    // Schedule our task. It may be for nothing but it doesn't harm.
    App_Scheduler_addTask_execTime(usb_task,
                                   APP_SCHEDULER_SCHEDULE_ASAP,
                                   USB_TASK_EXEC_TIME_US);
}

static bool tiny_usb_board_init(void)
{
    volatile uint32_t usb_reg;
    app_lib_time_timestamp_hp_t timeout;

    // We can only poll for power event as events are catched by Wirepas stack
    // and not shared with app.
    // We assume that USB connection is used only on devices powered by USB,
    // ie, the plug and play feature powered by a different source is not
    // supported yet
    usb_reg = NRF_POWER->USBREGSTATUS;
    if (!(usb_reg & POWER_USBREGSTATUS_VBUSDETECT_Msk))
    {
        // We are not powered through USB
        return false;
    }
    // Plugged event
    tusb_hal_nrf_power_event(0);

    // Wait a bit for VBUS ready (typical time measured at 1ms)
    timeout = lib_time->addUsToHpTimestamp(lib_time->getTimestampHp(),
                                           MAX_DELAY_VBUS_READY_MS * 1000);

    while (!(usb_reg & POWER_USBREGSTATUS_OUTPUTRDY_Msk)
            && (lib_time->isHpTimestampBefore(lib_time->getTimestampHp(),
                                             timeout)))
    {
        // Read register again
        usb_reg = NRF_POWER->USBREGSTATUS;
    }

    if (!(usb_reg & POWER_USBREGSTATUS_OUTPUTRDY_Msk))
    {
        // USB power is not ready after MAX_DELAY_VBUS_READY_MS
        return false;
    }

    // VBUS ready event
    tusb_hal_nrf_power_event(2);
    return true;
}

uint32_t Usb_wrapper_sendBuffer(const void * buffer, uint32_t length)
{
    length = tud_cdc_write(buffer, length);
    tud_cdc_write_flush();

    // Reschedule asap our task
    App_Scheduler_addTask_execTime(usb_task,
                                   APP_SCHEDULER_SCHEDULE_ASAP,
                                   USB_TASK_EXEC_TIME_US);

    return length;
}


/**
 * \brief   Initialize the tiny usb wrapper
 */
bool Usb_wrapper_init(Usb_wrapper_rx_callback_f cb)
{
    m_rx_cb = cb;

    // Initialize hardware resources
    if (!tiny_usb_board_init())
    {
        return false;
    }

    // initialize tinyusb stack
    // Do not check return code as API said bool but
    // on first init, it returns error_code 0.
    tusb_init();

    return true;
}

