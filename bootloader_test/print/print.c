/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>

#include "print.h"
/* USART driver */
#include "usart.h"

/* Storage for debug messages */
#define BUFFER_SIZE     128ul
static uint8_t          m_buffer[BUFFER_SIZE];

void Print_init(void)
{
    /* Initialize the hardware module */
    Usart_init(115200, UART_FLOW_CONTROL_NONE );
}

int Print_printf(const char * fmt, ...)
{
    uint32_t len;
    va_list arg;

    va_start(arg, fmt);
    len = vsnprintf((char *)&m_buffer[0], BUFFER_SIZE, fmt, arg);
    /* Try sending */
    len = Usart_sendBuffer(m_buffer, len);
    va_end(arg);

    return len;
}
