/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef USART_H_
#define USART_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * \brief   Initialize all USART blocks
 *          Enables peripheral clocks and disables peripherals
 *          for the duration of the initialization procedure.
 *          Clears all internal variables:
 *          Callback handlers
 *          Transfer busy flags
 *          After the initialization procedure is complete, it enables
 *          all peripherals, but leaves peripheral clocks disabled
 * \param   baudrate
 *          Baudrate to be used
 * \return none
 */
void Usart_init(uint32_t baudrate);

/**
 * \brief   Send a buffer to USART
 * \param   buf
 *          Buffer to send
 * \param   len
 *          Size of buffer to send
 * \note    This method will either send the whole buffer or nothing at all
 * \return  Number of bytes written (zero or len)
 */
uint32_t Usart_sendBuffer(const void * buf, uint32_t len);

#endif /* USART_H_ */
