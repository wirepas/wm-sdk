/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "board.h"
#include "hal_api.h"
#include "api.h"
#include "usb_wrapper.h"


static serial_rx_callback_f m_cb;

static void usb_cb(uint8_t * ch, size_t n)
{
    if (m_cb != NULL)
    {
        m_cb(ch, n);
    }
}

bool Usart_init(uint32_t baudrate, uart_flow_control_e flow_control)
{
    (void) baudrate;
    (void) flow_control;
    return Usb_wrapper_init(usb_cb);
}

void Usart_setEnabled(bool enabled)
{

}

void Usart_receiverOn(void)
{

}

void Usart_receiverOff(void)
{

}

bool Usart_setFlowControl(uart_flow_control_e flow)
{
    (void) flow;
    return true;
}


uint32_t Usart_sendBuffer(const void * buffer, uint32_t length)
{
    return Usb_wrapper_sendBuffer(buffer, length);
}

void Usart_enableReceiver(serial_rx_callback_f rx_callback)
{
    m_cb = rx_callback;
}

uint32_t Usart_getMTUSize(void)
{
    return 512;
}

void Usart_flush(void)
{
    // Flush is mainly called before a reboot (stop stack)
    // Just wait a bit for transfert to be finish toward the host
    volatile uint32_t timeout = 20000;
    while(timeout > 0)
    {
        timeout--;
    }
}
