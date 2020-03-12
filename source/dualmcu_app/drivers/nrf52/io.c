/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */


#include "hal_api.h"
#include "board.h"

#ifdef BOARD_UART_IRQ_PIN

void Io_init(void)
{
    // Disconnect uart_irq_pin
    nrf_gpio_cfg_default(BOARD_UART_IRQ_PIN);
    // But set light pull-up
    nrf_gpio_pin_set(BOARD_UART_IRQ_PIN);
}

void Io_enableUartIrq(void)
{
    nrf_gpio_cfg(BOARD_UART_IRQ_PIN,
                 NRF_GPIO_PIN_DIR_OUTPUT,
                 NRF_GPIO_PIN_INPUT_CONNECT,
                 NRF_GPIO_PIN_NOPULL,
                 NRF_GPIO_PIN_S0S1,
                 NRF_GPIO_PIN_NOSENSE);
}

void Io_setUartIrq(void)
{
    // Active low IRQ pin
    nrf_gpio_pin_clear(BOARD_UART_IRQ_PIN);
}

void Io_clearUartIrq(void)
{
    // To clear we pull pin up
    nrf_gpio_pin_set(BOARD_UART_IRQ_PIN);
}

#else

// IRQ pin not defined, no functionality
void Io_init(void)
{
}

void Io_enableUartIrq(void)
{
}

void Io_setUartIrq(void)
{
}

void Io_clearUartIrq(void)
{
}

#endif
