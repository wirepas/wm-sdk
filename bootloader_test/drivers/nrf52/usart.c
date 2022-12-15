/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <stdint.h>
#include <stdbool.h>

#include "board.h"
#include "mcu.h"
#include "gpio.h"

void Usart_init(uint32_t baudrate)
{
    gpio_pin_t usart_tx_pin, usart_rx_pin;

    const gpio_out_cfg_t usart_tx_gpio_cfg = {
        .out_mode_cfg = GPIO_OUT_MODE_PUSH_PULL,
        .level_default = GPIO_LEVEL_HIGH
    };

    /* GPIO init */
    Gpio_outputSetCfg(BOARD_GPIO_ID_USART_TX, &usart_tx_gpio_cfg);

    Gpio_getPin(BOARD_GPIO_ID_USART_TX, NULL, &usart_tx_pin);
    Gpio_getPin(BOARD_GPIO_ID_USART_RX, NULL, &usart_rx_pin);

    NRF_UART0->PSELTXD = usart_tx_pin;
    NRF_UART0->PSELRXD = usart_rx_pin;
    NRF_UART0->TASKS_STOPTX = 1;
    NRF_UART0->TASKS_STOPRX = 1;

    NRF_UART0->ENABLE = UART_ENABLE_ENABLE_Enabled;

    /* Set flow control */
    NRF_UART0->PSELRTS = 0xFFFFFFFF;
    NRF_UART0->PSELCTS = 0xFFFFFFFF;
    /* No parity, no HW flow control */
    NRF_UART0->CONFIG = 0;

    /* Serial port init */
    switch (baudrate)
    {
    case 115200:
        NRF_UART0->BAUDRATE = (uint32_t)UART_BAUDRATE_BAUDRATE_Baud115200;
        break;
    case 125000:
        /* UART_BAUDRATE_BAUDRATE_Baud125000 is not defined by Nordic */
        NRF_UART0->BAUDRATE = (uint32_t) (0x02000000UL);
        break;
    case 1000000:
        NRF_UART0->BAUDRATE = (uint32_t)UART_BAUDRATE_BAUDRATE_Baud1M;
        break;
    default:
        break;
    }

    NRF_UART0->EVENTS_TXDRDY = 0;
}

uint32_t Usart_sendBuffer(const void * buffer, uint32_t length)
{
    uint32_t sent = 0;
    uint8_t * _buffer = (uint8_t *) buffer;
    NRF_UART0->TASKS_STARTTX = 1;

    while (length--)
    {
        NRF_UART0->TXD = _buffer[sent++];
        while (!NRF_UART0->EVENTS_TXDRDY);
        NRF_UART0->EVENTS_TXDRDY = 0;
    }

    NRF_UART0->TASKS_STOPTX = 1;

    return sent;
}
