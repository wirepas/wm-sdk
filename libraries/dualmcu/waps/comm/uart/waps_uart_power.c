/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <stdint.h>

#include "waps_private.h"
#include "uart_wakeup.h"
#include "gpio.h"
#include "ds.h"
#include "usart.h"

#include "util.h"

#include "waps_uart.h"
#include "board.h"
#include "api.h"
#include "app_scheduler.h"

/**
 * \file    waps_usart_power.c
 *          The purpose of this file is to provide an automatic power on/off
 *          switch for the USART transceivers RX functionality.
 */

/** USART power-up procedure states */
typedef enum
{
    USART_POWER_OFF, /**< Power is completely off (IDLE state) */
    USART_POWER_UP,  /**< Usart is powering up (between off and on), not used with EFR32 */
    USART_POWER_ON   /**< Usart is powered up */
}usart_power_state_e;

/** Default shutdown time after no characters received on UART
 *  It is a failsafe mechanism to be sure the uart doesn't stay
 *  up forever but in theory, all bytes received should end up with
 *  a valid frame reception that will shutdown the uart explicitly
 *  100ms is quite huge but it is only a timeout
 */
#define USART_SHUTDOWN_TIMEOUT_MS         100

/** Exec time to shutdown uart. 100us is more than enough */
#define USART_SHUTDOWN_EXEC_TIME_US       100

/** \brief  Is autopower enabled? */
static bool m_autopower_enabled = false;

// Current state of receiver power-up
static volatile usart_power_state_e m_power_on = USART_POWER_OFF;

/**
 * \brief   Callback for RX pin state change
 */
static void                         uart_gpio_isr(gpio_id_t gpio_id, gpio_in_event_e gpio_event);

/**
 * \brief   Turn receiver power off
 */
static void                         power_off(void);

/**
 * \brief   Task to be executed to disable uart
 */
static uint32_t                     shutdown_uart();

void Waps_uart_AutoPowerOn(void)
{
    Sys_enterCriticalSection();
    m_power_on = USART_POWER_OFF;
    UartWakeup_enable(uart_gpio_isr);
    m_autopower_enabled = true;
    Sys_exitCriticalSection();
}

void Waps_uart_AutoPowerOff(void)
{
    Sys_enterCriticalSection();
    m_power_on = USART_POWER_OFF;
    UartWakeup_disable();
    m_autopower_enabled = false;
    Sys_exitCriticalSection();
}

void Waps_uart_keepPowerOn(void)
{
    if (!m_autopower_enabled)
    {
        // Nothing to do
        return;
    }

    // Inform us that UART power is to be kept on (receiving frame)
    // Just in case UART was receiving garbage or incomplete frame,
    // activate (or update) a task to automatically shutdown the uart
    // later on. But the task should never be executed and uart automatically
    // stopped once a valid frame is received.
    App_Scheduler_addTask_execTime(shutdown_uart,
                                   USART_SHUTDOWN_TIMEOUT_MS,
                                   USART_SHUTDOWN_EXEC_TIME_US);
}

void Waps_uart_powerOff(void)
{
    if (!m_autopower_enabled)
    {
        // Nothing to do
        return;
    }

    // Valid frame received (can shut down UART now)
    // No need to wait for timeout and shutdown uart immediately
    // Stopping the uart could be done directly but let's schedule
    // the task asap instead to be more symmetric
    App_Scheduler_addTask_execTime(shutdown_uart,
                                   APP_SCHEDULER_SCHEDULE_ASAP,
                                   USART_SHUTDOWN_EXEC_TIME_US);
}

/** This function expects few preamble bytes, will not work otherwise */
static void uart_gpio_isr(gpio_id_t gpio_id, gpio_in_event_e gpio_event)
{
    // No need to check GPIO ID here.
    (void)gpio_id;

#if defined(EFR32_PLATFORM)
    // Do not care is it rising or falling edge,
    // any activity will wake up UART.
    (void)gpio_event;

    if(m_power_on == USART_POWER_OFF)
    {
        // MCU Wake-up
        Sys_enterCriticalSection();
        Usart_setEnabled(true);
        Usart_receiverOn();
        m_power_on = USART_POWER_ON;
        Sys_exitCriticalSection();
        UartWakeup_disable();
        wakeup_task();
    }
#elif defined(NRF52_PLATFORM) || defined(NRF91_PLATFORM)
    if((m_power_on == USART_POWER_OFF) && (gpio_event == GPIO_IN_EVENT_FALLING_EDGE))
    {
        /* MCU Wake-up (falling edge)! Start edge trigger VERY quickly */
        Sys_enterCriticalSection();
        m_power_on = USART_POWER_UP;
        Sys_exitCriticalSection();
        wakeup_task();
    }
    else if((m_power_on == USART_POWER_UP) && (gpio_event == GPIO_IN_EVENT_RISING_EDGE))
    {
        /* Rising edge (receiver is on!) enable receiver and disable isr */
        Sys_enterCriticalSection();
        /* UART must be enabled here (after receiver is on), don't know why */
        Usart_setEnabled(true);
        Usart_receiverOn();
        m_power_on = USART_POWER_ON;
        Sys_exitCriticalSection();
        UartWakeup_disable();
    }
#else
#error "Unknown platform"
#endif
}

static uint32_t shutdown_uart(void)
{
    // The delay without uart activity has elapsed
    power_off();
    return APP_SCHEDULER_STOP_TASK;
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
        Waps_uart_clean();
    }

    /* Power is now off */
    m_power_on = USART_POWER_OFF;
    /* Enable detection of UART RX rising/falling edges, so that the UART receiver can be woken */
    UartWakeup_enable(uart_gpio_isr);
}
