/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef USART_H_
#define USART_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/** User callback for character received event */
typedef void (*serial_rx_callback_f)(uint8_t * ch, size_t n);

typedef enum
{
    UART_FLOW_CONTROL_NONE,
    UART_FLOW_CONTROL_HW,
} uart_flow_control_e;

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
 * \param   flow_control
 *          Flow control to be used
 * \return  True if successful, false otherwise
 */
bool Usart_init(uint32_t baudrate, uart_flow_control_e flow_control);

/**
 * \brief   Enable or disable USART
 * \param   enabled
 *          True, if USART should be enabled
 */
void Usart_setEnabled(bool enabled);

/**
 * \brief   Enable USART Receiver
 */
void Usart_receiverOn(void);

/**
 * \brief   Disable USART Receiver
 */
void Usart_receiverOff(void);

/**
 * \brief   Set USART flow control mode
 * \pre     USART is not enabled
 * \param   flow
 *          Desired flow control mode
 * \return  True, if the baud rate was updated successfully
 */
bool Usart_setFlowControl(uart_flow_control_e flow);

/**
 * \brief   Send a buffer to USART
 * \param   buf
 *          Buffer to send
 * \param   len
 *          Size of buffer to send (max = \ref Usart_getMTUSize())
 * \note    This method will either send the whole buffer or nothing at all
 * \return  Number of bytes written (zero or len)
 */
uint32_t Usart_sendBuffer(const void * buf, uint32_t len);
/**
 * \brief   Enable UART receiver
 *          Set callback handlers for character RX event
 *          Enables interrupt source and event, and the input pin
 * \param   callback Callback function for a character rx event
 *          Can be NULL, which clears the callback and disables IRQ
 * \return none
 */
void Usart_enableReceiver(serial_rx_callback_f callback);

/**
 * \brief   Returns the maximum transmission unit in bytes for the USART
 *          (= maximum length of a single message)
 */
uint32_t Usart_getMTUSize(void);

/**
 * \brief   Soft flush (dump data to I/O) UART TX buffer
 */
void Usart_flush(void);

#endif /* USART_H_ */
