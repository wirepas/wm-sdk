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
#include "em_gpio.h"

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
#if defined(_SILICON_LABS_32B_SERIES_1)
    /** IFC is a latch register, safe to use without bit-band */
    GPIO->IFC = (1 << pin);
#elif defined(_SILICON_LABS_32B_SERIES_2)
    GPIO->IF_CLR = (1 << pin);
#else
#error "Unknown EFR32 series and config"
#endif
}

static void pin_config(uint32_t pin, uint32_t port)
{
    // Code does not set GPIO->EXTIPINSELL or GPIO->EXTIPINSELH
    // thus direct mapping: pin x ==> EXTI x is used.
    if (pin < 8)
    {
        GPIO->EXTIPSELL = (GPIO->EXTIPSELL & ~(0xF << (4 * pin))) |
                            (port << (4 * pin));
    }
#if !defined(_SILICON_LABS_32B_SERIES_2_CONFIG_1)
    else
    {
        GPIO->EXTIPSELH = (GPIO->EXTIPSELH & ~(0xF << (4 * (pin - 8)))) |
                            (port << (4 * (pin - 8)));
    }
#endif
}

void Wakeup_pinInit(wakeup_cb_f cb)
{
    /* Clear interrupt sources and flags */
    GPIO->IEN = _GPIO_IEN_RESETVALUE;
#if defined(_SILICON_LABS_32B_SERIES_1)
    GPIO->IFC = _GPIO_IFC_MASK;
#elif defined(_SILICON_LABS_32B_SERIES_2)
    GPIO->IF_CLR = _GPIO_IF_MASK;
#else
#error "Unknown EFR32 series and config"
#endif
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

void Wakeup_clearIrq(void)
{
    clear_interrupt(BOARD_USART_RX_PIN);
}

void Wakeup_setEdgeIRQ(exti_irq_config_e edge, bool enable)
{
    bool risingEdge, fallingEdge;
    edge & EXTI_IRQ_RISING_EDGE ? (risingEdge = true) : (risingEdge = false);
    edge & EXTI_IRQ_FALLING_EDGE ? (fallingEdge = true) : (fallingEdge = false);
    GPIO_IntConfig((GPIO_Port_TypeDef)BOARD_USART_GPIO_PORT,  BOARD_USART_RX_PIN,
                                    risingEdge,
                                    fallingEdge,
                                    enable);
}

void __attribute__((__interrupt__)) GPIO_EVEN_IRQHandler(void)
{
#if defined(_SILICON_LABS_32B_SERIES_1)
    uint32_t epin = GPIO->IF & _GPIO_IF_EXT_MASK;
    epin &= GPIO->IEN & _GPIO_IEN_EXT_MASK;
#elif defined(_SILICON_LABS_32B_SERIES_2_CONFIG_1)
    uint32_t epin = GPIO->IF & _GPIO_IF_EXT_MASK;
    epin &= GPIO->IEN & _GPIO_IEN_EXTIEN_MASK;
#elif defined(_SILICON_LABS_32B_SERIES_2_CONFIG_2) || defined(_SILICON_LABS_32B_SERIES_2_CONFIG_3)
    uint32_t epin = GPIO->IF & 0x0FFF;
    epin &= GPIO->IEN & 0x0FFF;
#else
#error "Unknown EFR32 series and config"
#endif
    uint32_t index;
    for(index = 0; index < IRQ_CHANNELS_PER_VECTOR; index++)
    {
        if((epin & 0x01) == 0x01)
        {
            clear_interrupt(2 * index);
            if(m_even_callbacks[index] != NULL)
            {
                m_even_callbacks[index]();
            }
        }
        epin >>= 2;
    }
}

void __attribute__((__interrupt__)) GPIO_ODD_IRQHandler(void)
{
#if defined(_SILICON_LABS_32B_SERIES_1)
    uint32_t opin = GPIO->IF & _GPIO_IF_EXT_MASK;
    opin &= GPIO->IEN & _GPIO_IEN_EXT_MASK;
#elif defined(_SILICON_LABS_32B_SERIES_2_CONFIG_1)
    uint32_t opin = GPIO->IF & _GPIO_IF_EXT_MASK;
    opin &= GPIO->IEN & _GPIO_IEN_EXTIEN_MASK;
#elif defined(_SILICON_LABS_32B_SERIES_2_CONFIG_2) || defined(_SILICON_LABS_32B_SERIES_2_CONFIG_3)
    uint32_t opin = GPIO->IF & 0x0FFF;
    opin &= GPIO->IEN & 0x0FFF;
#else
#error "Unknown EFR32 series and config"
#endif
    opin >>= 1;
    uint32_t index;
    for(index = 0; index < IRQ_CHANNELS_PER_VECTOR; index++)
    {
        if((opin & 0x01) == 0x01)
        {
            clear_interrupt(2 * index + 1);
            if(m_odd_callbacks[index] != NULL)
            {
                m_odd_callbacks[index]();
            }
        }
        opin >>= 2;
    }
}

