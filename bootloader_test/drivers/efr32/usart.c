/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <stdint.h>
#include <stdbool.h>

#include "board.h"
#include "mcu.h"

/* Map some registers constant to the USART selected */
#if BOARD_USART_ID == 0
#define BOARD_USART         USART0
#define BOARD_USART_CMU_BIT CMU_HFPERCLKEN0_USART0
#define BOARD_UART_RX_IRQn  USART0_RX_IRQn
#define BOARD_UART_TX_IRQn  USART0_TX_IRQn
#elif BOARD_USART_ID == 1
#define BOARD_USART         USART1
#define BOARD_USART_CMU_BIT CMU_HFPERCLKEN0_USART1
#define BOARD_UART_RX_IRQn  USART1_RX_IRQn
#define BOARD_UART_TX_IRQn  USART1_TX_IRQn
#elif BOARD_USART_ID == 2
#define BOARD_USART         USART2
#define BOARD_USART_CMU_BIT CMU_HFPERCLKEN0_USART2
#define BOARD_UART_RX_IRQn  USART2_RX_IRQn
#define BOARD_UART_TX_IRQn  USART2_TX_IRQn
#elif BOARD_USART_ID == 3
#define BOARD_USART         USART3
#define BOARD_USART_CMU_BIT CMU_HFPERCLKEN0_USART3
#define BOARD_UART_RX_IRQn  USART3_RX_IRQn
#define BOARD_UART_TX_IRQn  USART3_TX_IRQn
#else
#error USART ID must be 0, 1, 2 or 3
#endif

static uint32_t getHFPERCLK()
{
    uint32_t div;
    div = 1U + ((CMU->HFPERPRESC & _CMU_HFPERPRESC_PRESC_MASK) >>
                                                _CMU_HFPERPRESC_PRESC_SHIFT);
    return 38000000 / div;
}

static void set_baud(uint32_t baud)
{
    volatile uint32_t baud_gen;
    /* Calculate baudrate: see em_usart.c in emlib for reference */
    baud_gen = 32 * getHFPERCLK() + (4 * baud) / 2;
    baud_gen /= (4 * baud);
    baud_gen -= 32;
    baud_gen *= 8;
    baud_gen &= _USART_CLKDIV_DIV_MASK;

    /* Set oversampling bit (8) */
    BOARD_USART->CTRL  &= ~_USART_CTRL_OVS_MASK;
    BOARD_USART->CTRL  |= USART_CTRL_OVS_X4;
    BOARD_USART->CLKDIV = baud_gen;
}

void Usart_init(uint32_t baudrate)
{
    /* Wait for HFRCO to be ready */
    while ((CMU->SYNCBUSY & CMU_SYNCBUSY_HFRCOBSY) ||
           !(CMU->STATUS & _CMU_STATUS_HFRCORDY_MASK)) {}

    /* Set 1 wait state */
    MSC->READCTRL = (MSC->READCTRL & ~_MSC_READCTRL_MODE_MASK) |
                    MSC_READCTRL_MODE_WS1;

    /* Set HFRCO to 38 MHz */
    CMU->HFRCOCTRL = DEVINFO->HFRCOCAL12;

     /* Enable clocks */
    CMU->HFBUSCLKEN0 |= CMU_HFBUSCLKEN0_GPIO;

    /* Configure Uart Tx pin */
    hal_gpio_set_mode(BOARD_USART_GPIO_PORT,
                      BOARD_USART_TX_PIN,
                      GPIO_MODE_OUT_PP);
    hal_gpio_clear(BOARD_USART_GPIO_PORT, BOARD_USART_TX_PIN);

    /* Must enable clock for configuration period */
    CMU->HFPERCLKEN0 |= BOARD_USART_CMU_BIT;

    /* Set UART output pins */
    BOARD_USART->ROUTEPEN = USART_ROUTEPEN_TXPEN;

    /* Set UART route */
    BOARD_USART->ROUTELOC0 = BOARD_USART_ROUTELOC_TXLOC;

    /* Initialize UART for asynch mode with baudrate baud */
    /* Disable transceiver */
    BOARD_USART->CMD = 0;
    BOARD_USART->CTRL = 0;
    BOARD_USART->I2SCTRL = 0;
    /* Disables PRS */
    BOARD_USART->INPUT = 0;
    /* Set frame params: 8bit, nopar, 1stop */
    BOARD_USART->FRAME = USART_FRAME_DATABITS_EIGHT |
                         USART_FRAME_PARITY_NONE |
                         USART_FRAME_STOPBITS_ONE;
    set_baud(baudrate);

    /* Enable transmitter */
    BOARD_USART->CMD = USART_CMD_TXEN;
}

uint32_t Usart_sendBuffer(const void * buffer, uint32_t length)
{
    uint32_t sent = 0;
    uint8_t * _buffer = (uint8_t *) buffer;

    while (length--)
    {
        BOARD_USART->TXDATA = _buffer[sent++];
        while (!(BOARD_USART->STATUS & USART_STATUS_TXC));
    }

    return sent;
}
