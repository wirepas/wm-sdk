/* Copyright 2022 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file    uart_wakeup.c
 * \brief   Used to detect rising/falling edges on the UART RX GPIO in order to wake up the UART receiver.
 */

#ifndef UART_WAKEUP_
#define UART_WAKEUP_

#include "gpio.h"

/**
 * \brief   Enable wake-up functionality.
 * \param   cb
 *          Callback that is invoked on GPIO edge transition
 */
void UartWakeup_enable(gpio_in_event_cb_f cb);

/**
 * \brief   Disable wake-up functionality.
 */
void UartWakeup_disable(void);

#endif /* UART_WAKEUP_ */
