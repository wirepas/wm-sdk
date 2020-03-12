/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */


#include "hal_api.h"
#include "io.h"
#include "board.h"

#if defined(BOARD_UART_INT_PIN) && defined(BOARD_UART_INT_PORT)

void Io_init(void)
{
//    /* Enable clocks */
    CMU->HFBUSCLKEN0 |= CMU_HFBUSCLKEN0_GPIO;

    hal_gpio_set_mode(BOARD_UART_INT_PORT,
                      BOARD_UART_INT_PIN,
                      GPIO_MODE_DISABLED);
    hal_gpio_clear(BOARD_UART_INT_PORT,
                   BOARD_UART_INT_PIN);
}

void Io_enableUartIrq(void)
{

    hal_gpio_set_mode(BOARD_UART_INT_PORT,
                      BOARD_UART_INT_PIN,
                      GPIO_MODE_OUT_PP);
}

void Io_setUartIrq(void)
{
    // Active low IRQ pin
    hal_gpio_clear(BOARD_UART_INT_PORT,
                   BOARD_UART_INT_PIN);
}

void Io_clearUartIrq(void)
{
    // To clear we pull pin up
    hal_gpio_set(BOARD_UART_INT_PORT,
                 BOARD_UART_INT_PIN);
}

void Io_setModeDisabled(void)
{
    // Disable pin
    hal_gpio_set_mode(BOARD_UART_INT_PORT,
                      BOARD_UART_INT_PIN,
                      GPIO_MODE_DISABLED);
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
