/* Copyright 2022 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file    indication_signal.c
 * \brief   Used to generate the Interrupt ReQuest ("IRQ") signal to notify the application that it has one or more pending indications.
 */

#include "board.h"
#include "gpio.h"

#ifdef BOARD_GPIO_ID_UART_IRQ

void IndicationSignal_enable(void)
{
    const gpio_out_cfg_t gpio_out_cfg =
    {
        .out_mode_cfg = GPIO_OUT_MODE_PUSH_PULL,
        .level_default = GPIO_LEVEL_HIGH // Active low IRQ pin
    };

    Gpio_outputSetCfg(BOARD_GPIO_ID_UART_IRQ, &gpio_out_cfg);
}

void IndicationSignal_set(void)
{
    // Active low IRQ pin
    Gpio_outputWrite(BOARD_GPIO_ID_UART_IRQ, GPIO_LEVEL_LOW);
}

void IndicationSignal_clear(void)
{
    // Active low IRQ pin
    Gpio_outputWrite(BOARD_GPIO_ID_UART_IRQ, GPIO_LEVEL_HIGH);
}

#else // else if BOARD_GPIO_ID_UART_IRQ is undefined

void IndicationSignal_enable(void)
{
}

void IndicationSignal_set(void)
{
}

void IndicationSignal_clear(void)
{
}

#endif // BOARD_GPIO_ID_UART_IRQ
