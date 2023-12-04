/* Copyright 2022 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file    uart_wakeup.c
 * \brief   Used to detect rising/falling edges on the UART RX GPIO in order to wake up the UART receiver.
 */

#include "gpio.h"
#include "board.h"
#include "hal_api.h"

#ifndef UART_USE_USB

void UartWakeup_enable(gpio_in_event_cb_f cb)
{
    const gpio_in_cfg_t gpio_in_cfg =
    {
        .event_cb = cb,
        .event_cfg = GPIO_IN_EVENT_RISING_EDGE | GPIO_IN_EVENT_FALLING_EDGE,
        .in_mode_cfg = GPIO_IN_PULL_NONE
    };

    Gpio_inputSetCfg(BOARD_GPIO_ID_USART_WAKEUP, &gpio_in_cfg);
}

void UartWakeup_disable(void)
{
    const gpio_in_cfg_t gpio_in_cfg =
    {
        .event_cb = NULL,
        .event_cfg = GPIO_IN_EVENT_NONE,
        .in_mode_cfg = GPIO_IN_PULL_NONE
    };

    Gpio_inputSetCfg(BOARD_GPIO_ID_USART_WAKEUP, &gpio_in_cfg);
}

#else // else if UART_USE_USB is defined
// With USB connection, no wakeup mechanism (not needed)

void UartWakeup_enable(gpio_in_event_cb_f cb)
{
}

void UartWakeup_disable(void)
{
}

#endif // UART_USE_USB
