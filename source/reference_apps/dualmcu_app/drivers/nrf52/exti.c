/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <stdint.h>
#include <stddef.h>

#include "hal_api.h"
#include "io.h"
#include "api.h"
#include "board.h"

#ifndef UART_USE_USB
// Callback for wake-up pin
static wakeup_cb_f                  m_callback;

// Declare the GPIOTE ISR
void __attribute__((__interrupt__)) GPIOTE_IRQHandler(void);

void Wakeup_pinInit(wakeup_cb_f cb)
{
    // Clear interrupt sources and flags
    NRF_GPIOTE->INTENCLR = 0xFFFFFFFF;
    NRF_GPIOTE->EVENTS_PORT = 0;
    // Start with falling edge trigger
    nrf_gpio_cfg_sense_set(BOARD_USART_RX_PIN, NRF_GPIO_PIN_SENSE_LOW);
    m_callback = cb;
    Sys_clearFastAppIrq(GPIOTE_IRQn);
    Sys_enableFastAppIrq(GPIOTE_IRQn,
                         APP_LIB_SYSTEM_IRQ_PRIO_HI,
                         GPIOTE_IRQHandler);
}

void Wakeup_off(void)
{
    Sys_disableAppIrq(GPIOTE_IRQn);
    Sys_clearFastAppIrq(GPIOTE_IRQn);
}

void Wakeup_clearIrq(void)
{
    NRF_GPIOTE->EVENTS_PORT = 0;
}

void Wakeup_setEdgeIRQ(exti_irq_config_e edge, bool enable)
{
    uint8_t mode = NRF_GPIO_PIN_NOSENSE;
    if(edge & EXTI_IRQ_RISING_EDGE)
    {
        mode = NRF_GPIO_PIN_SENSE_HIGH;
    }
    else if(edge & EXTI_IRQ_FALLING_EDGE)
    {
        mode = NRF_GPIO_PIN_SENSE_LOW;
    }

    /* Enable/disable the rising edge interrupt. */
    nrf_gpio_cfg_sense_set(BOARD_USART_RX_PIN, mode);

    /* Clear any pending interrupt. */
    Wakeup_clearIrq();

    /* Finally enable/disable interrupt. */
    if (enable)
    {
        NRF_GPIOTE->INTENSET = GPIOTE_INTENSET_PORT_Msk;
    }
    else
    {
        NRF_GPIOTE->INTENCLR = GPIOTE_INTENSET_PORT_Msk;
    }
}

void __attribute__((__interrupt__)) GPIOTE_IRQHandler(void)
{
    if (NRF_GPIOTE->EVENTS_PORT != 0)
    {
        NRF_GPIOTE->EVENTS_PORT = 0;
        // read any event from peripheral to flush the write buffer:
        EVENT_READBACK = NRF_GPIOTE->EVENTS_PORT;
        if (m_callback)
        {
            m_callback();
        }
    }
}
#else
// With USB connection, no wakeup mechanism (not needed)
void Wakeup_pinInit(wakeup_cb_f cb)
{
    (void) cb;
}

void Wakeup_off(void)
{
}

void Wakeup_clearIrq(void)
{
}

void Wakeup_setEdgeIRQ(exti_irq_config_e edge, bool enable)
{
}
#endif
