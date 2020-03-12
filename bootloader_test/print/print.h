/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef _PRINT_H_
#define _PRINT_H_

#include <stdarg.h>

/**
 * \brief   Initialize the print module
 */
void Print_init(void);

/**
 * \brief   printf functionality
 * \param   fmt
 *          Formatted text to print
 * \param   ...
 *          Argument list for formatted text
 * \return  Amount of characters written to standard output
 */
int Print_printf(const char * fmt, ...);

#endif /* _PRINT_H_ */
