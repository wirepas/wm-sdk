/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */


#ifndef UART_PRINT_H_
#define UART_PRINT_H_
#include <stdarg.h>

#ifdef APP_PRINTING

/**
 * \brief   Initialize the uart print module
 * \param   baudrate
 *          Baudrate to be used; min 115200 bits/s
 * \note    Uart pins are define in board.h
 */
void UartPrint_init(uint32_t baudrate);

/**
 * \brief   printf functionality
 * \param   fmt
 *          Formatted text to print
 * \param   ...
 *          Argument list for formatted text
 * \return  Amount of characters written to standard output
 */
int UartPrint_printf(const char * fmt, ...);

#else
#define UartPrint_init(baudrate)
#define UartPrint_printf(fmt, ...)
#endif

#endif /* UART_PRINT_H_ */
