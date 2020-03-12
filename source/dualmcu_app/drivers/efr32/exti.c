/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <stdint.h>
#include <stddef.h>

#include "hal_api.h"
#include "io.h"
#include "board.h"
#include "api.h"

// Amount of channels per vector (IO port width / 2)
#define IRQ_CHANNELS_PER_VECTOR     8

// Callbacks
static wakeup_cb_f                  m_even_callbacks[IRQ_CHANNELS_PER_VECTOR];
static wakeup_cb_f                  m_odd_callbacks[IRQ_CHANNELS_PER_VECTOR];

// Declare ISR
void __attribute__((__interrupt__)) GPIO_EVEN_IRQHandler(void);
void __attribute__((__interrupt__)) GPIO_ODD_IRQHandler(void);

static inline void clear_interrupt(uint32_t pin)
{
    /** IFC is a latch register, safe to use without bit-band */
    GPIO->IFC = (1 << pin);
}

static void pin_config(uint32_t pin, uint32_t port)
{
    if (pin < 8)
    {
        GPIO->EXTIPSELL = (GPIO->EXTIPSELL & ~(0xF << (4 * pin))) |
                            (port << (4 * pin));
    }
    else
    {
        GPIO->EXTIPSELH = (GPIO->EXTIPSELH & ~(0xF << (4 * (pin - 8)))) |
                            (port << (4 * (pin - 8)));
    }
}

void Wakeup_pinInit(wakeup_cb_f cb)
{
    /* Clear interrupt sources and flags */
    GPIO->IEN = _GPIO_IEN_RESETVALUE;
    GPIO->IFC = _GPIO_IFC_MASK;

    uint32_t    pin = BOARD_USART_RX_PIN;
    uint32_t    index = pin;
    index >>= 1;
    if((pin % 2) == 0)
    {
        /* Even IRQ (only one pin per channel supported) */
        m_even_callbacks[index] = cb;
    }
    else
    {
        /* Odd IRQ (only one pin per channel supported) */
        m_odd_callbacks[index] = cb;
    }

    pin_config(BOARD_USART_RX_PIN, BOARD_USART_GPIO_PORT);
    /* Enable interrupt source for even pins */
    Sys_clearFastAppIrq(GPIO_EVEN_IRQn);
    Sys_enableFastAppIrq(GPIO_EVEN_IRQn,
                         APP_LIB_SYSTEM_IRQ_PRIO_HI,
                         GPIO_EVEN_IRQHandler);
    Sys_clearFastAppIrq(GPIO_ODD_IRQn);
    Sys_enableFastAppIrq(GPIO_ODD_IRQn,
                         APP_LIB_SYSTEM_IRQ_PRIO_HI,
                         GPIO_ODD_IRQHandler);
}

void Wakeup_off(void)
{
    Sys_disableAppIrq(GPIO_EVEN_IRQn);
    Sys_clearFastAppIrq(GPIO_EVEN_IRQn);
    Sys_disableAppIrq(GPIO_ODD_IRQn);
    Sys_clearFastAppIrq(GPIO_ODD_IRQn);
}

void Wakeup_enableIrq(void)
{
    BITBAND_Peripheral(&(GPIO->IEN), BOARD_USART_RX_PIN, 1);
}

void Wakeup_disableIrq(void)
{
    BITBAND_Peripheral(&(GPIO->IEN), BOARD_USART_RX_PIN, 0);
}

void Wakeup_clearIrq(void)
{
    clear_interrupt(BOARD_USART_RX_PIN);
}

void Wakeup_setEdge(uint8_t flags)
{
    uint32_t enable;
    /* Setup raising edge */
    flags & EXTI_IRQ_RISING_EDGE ? (enable = 1) : (enable = 0);
    BITBAND_Peripheral(&(GPIO->EXTIRISE), BOARD_USART_RX_PIN, enable);
    /* Setup falling edge */
    flags & EXTI_IRQ_FALLING_EDGE ? (enable = 1) : (enable = 0);
    BITBAND_Peripheral(&(GPIO->EXTIFALL), BOARD_USART_RX_PIN, enable);
}

void __attribute__((__interrupt__)) GPIO_EVEN_IRQHandler(void)
{
    uint32_t pin = GPIO->IF & _GPIO_IF_EXT_MASK;
    pin &= GPIO->IEN & _GPIO_IEN_EXT_MASK;
    uint32_t index;
    for(index = 0; index < IRQ_CHANNELS_PER_VECTOR; index++)
    {
        if((pin & 0x01) == 0x01)
        {
            clear_interrupt(2 * index);
            if(m_even_callbacks[index] != NULL)
            {
                m_even_callbacks[index]();
            }
        }
        pin >>= 2;
    }
}

void __attribute__((__interrupt__)) GPIO_ODD_IRQHandler(void)
{
    uint32_t pin = GPIO->IF & _GPIO_IF_EXT_MASK;
    pin &= GPIO->IEN & _GPIO_IEN_EXT_MASK;
    pin >>= 1;
    uint32_t index;
    for(index = 0; index < IRQ_CHANNELS_PER_VECTOR; index++)
    {
        if((pin & 0x01) == 0x01)
        {
            clear_interrupt(2 * index + 1);
            if(m_odd_callbacks[index] != NULL)
            {
                m_odd_callbacks[index]();
            }
        }
        pin >>= 2;
    }
}

