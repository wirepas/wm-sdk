/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <stdint.h>

#include "waps_private.h"

#include "io.h"
#include "ds.h"
#include "usart.h"

#include "util.h"

#include "waps_uart.h"
#include "board.h"
#include "api.h"

/**
 * \file    waps_usart_power.c
 *          The purpose of this file is to provide an automatic power on/off
 *          switch for the USART transceivers RX functionality.
 *          This is done by level aware interrupt.
 *          A low level on the line starts the power-up procedure, and triggers
 *          a high level interrupt. When the high level is present, the UART +
 *          receiver are turned on.
 */

/** USART power-up procedure states */
typedef enum
{
    USART_POWER_OFF, /**< Power is completely off (IDLE state) */
    USART_POWER_UP,  /**< Usart is powering up (between off and on) */
    USART_POWER_ON   /**< Usart is powered up */
}usart_power_state_e;

/** Default shutdown time after no characters received */
#define USART_SHUTDOWN_TIME         100000

// State of receiver (receiving / frame done)
volatile bool                       m_keep_power_on;

// Current state of receiver power-up
static volatile usart_power_state_e m_power_on = USART_POWER_OFF;

/**
 * \brief   Callback for RX pin state change
 */
static void                         uart_gpio_isr(void);

/**
 * \brief   Turn receiver power off
 */
static void                         power_off(void);

void Waps_uart_AutoPowerOn(void)
{
    Sys_enterCriticalSection();
    m_power_on = USART_POWER_OFF;
    Wakeup_pinInit(uart_gpio_isr);
    Wakeup_disableIrq();
    Wakeup_clearIrq();
    // Expecting falling edge
    Wakeup_setEdge(EXTI_IRQ_FALLING_EDGE);
    Wakeup_enableIrq();
    Sys_exitCriticalSection();
}

void Waps_uart_AutoPowerOff(void)
{
    Sys_enterCriticalSection();
    m_power_on = USART_POWER_OFF;
    Wakeup_disableIrq();
    Wakeup_clearIrq();
    Wakeup_off();
    Sys_exitCriticalSection();
}

void Waps_uart_keepPowerOn(void)
{
    // Inform us that UART power is to be kept on (receiving frame)
    m_keep_power_on = true;
}

void Waps_uart_powerOff(void)
{
    // Valid frame received (can shut down UART now)
    m_keep_power_on = false;
}

uint32_t Waps_uart_powerExec(void)
{
    // Time to keep power on if no activity for a while
    static uint32_t power_time;
    uint32_t ret = APP_LIB_SYSTEM_STOP_PERIODIC;
    Sys_enterCriticalSection();
    if(m_power_on == USART_POWER_OFF)
    {
        // Already off
        goto _exit;
    }
    uint32_t now = lib_time->getTimestampHp();
    // How much time until shutdown
    ret = lib_time->getTimeDiffUs(now, power_time);
    if (m_keep_power_on)
    {
        // Start timed shutdown now
        power_time = now;
        m_keep_power_on = false;
        ret = USART_SHUTDOWN_TIME;
    }
    else if (ret >= USART_SHUTDOWN_TIME)
    {
        // Time to power off
        power_off();
        ret = APP_LIB_SYSTEM_STOP_PERIODIC;
    }
_exit:
    Sys_exitCriticalSection();
    return ret;
}

/** This function expects three preamble bytes, will not work otherwise */
static void uart_gpio_isr(void)
{
    if(m_power_on == USART_POWER_OFF)
    {
        /* MCU Wake-up (falling edge)! Start edge trigger VERY quickly */
        Sys_enterCriticalSection();
        /* Change sense direction and enable rising edge trigger */
        Wakeup_setEdge(EXTI_IRQ_RISING_EDGE);
        m_power_on = USART_POWER_UP;
        m_keep_power_on = true;
        Sys_exitCriticalSection();
        // Rising edge comes very fast, may not be safe to enter deep sleep
        DS_Disable(DS_SOURCE_USART_POWER);
        wakeup_task();
    }
    else if(m_power_on == USART_POWER_UP)
    {
        /* Rising edge (receiver is on!) enable receiver and disable isr */
        Sys_enterCriticalSection();
        /* UART must be enabled here (after receiver is on), don't know why */
        Usart_setEnabled(true);
        Usart_receiverOn();
        m_power_on = USART_POWER_ON;
        Sys_exitCriticalSection();
        // Uart is now awake, safe to enable deep sleep
        DS_Enable(DS_SOURCE_USART_POWER);
        Wakeup_disableIrq();
        Wakeup_clearIrq();
        Wakeup_setEdge(EXTI_IRQ_FALLING_EDGE);
    }
}

static void power_off(void)
{
    /* If already off then do nothing */
    if(m_power_on == USART_POWER_OFF)
    {
        return;
    }

    /* Power off the UART and receiver if necessary */
    if(m_power_on == USART_POWER_ON)
    {
        /* USART is on: power it off */
        Usart_receiverOff();
        Usart_setEnabled(false);
    }
    else if(m_power_on == USART_POWER_UP)
    {
        // Uart is in powering up state, clear deep sleep disable bit
        DS_Enable(DS_SOURCE_USART_POWER);
    }

    /* Power is now off */
    m_power_on = USART_POWER_OFF;
    /* Change edge to falling edge */
    Wakeup_setEdge(EXTI_IRQ_FALLING_EDGE);
    /* Re-enable power-up pin */
    Wakeup_clearIrq();
    Wakeup_enableIrq();
}
