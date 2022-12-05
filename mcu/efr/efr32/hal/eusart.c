/* Copyright 2022 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "hal_api.h"
#include "board.h"
#include "api.h"
#include "gpio.h"
#include "em_eusart.h"
#include "em_cmu.h"

#ifdef WIRESHARK
// Increase it by default for Wireshark plugin
#define BUFFER_SIZE                     2048u
#else
#define BUFFER_SIZE                     512u
#endif
#include "ringbuffer.h"
static volatile ringbuffer_t            m_usart_tx_buffer;

static volatile serial_rx_callback_f    m_rx_callback;
static volatile uint32_t                m_enabled;
static volatile bool                    m_tx_active;

void __attribute__((__interrupt__))     EUSART_RX_IRQHandler(void);
void __attribute__((__interrupt__))     EUSART_TX_IRQHandler(void);

bool Usart_init(uint32_t baudrate, uart_flow_control_e flow_control)
{
#ifdef BOARD_GPIO_ID_VCOM_ENABLE
    const gpio_out_cfg_t usart_vcom_enable_cfg =
    {
        .out_mode_cfg = GPIO_OUT_MODE_PUSH_PULL,
        .level_default = GPIO_LEVEL_HIGH
    };
#endif

#ifdef BOARD_GPIO_ID_VCOM_ENABLE
    // Enable vcom
    Gpio_outputSetCfg(BOARD_GPIO_ID_VCOM_ENABLE, &usart_vcom_enable_cfg);
#endif

    (void)flow_control;

    // Module variables
    Ringbuffer_reset(m_usart_tx_buffer);
    m_rx_callback = NULL;
    m_tx_active = false;
    m_enabled = 0;

    // Disable for RX
    Sys_disableAppIrq(EUSART0_RX_IRQn);
    Sys_clearFastAppIrq(EUSART0_RX_IRQn);
    // Disable for TX
    Sys_disableAppIrq(EUSART0_TX_IRQn);
    Sys_clearFastAppIrq(EUSART0_TX_IRQn);

    CMU_ClockEnable(cmuClock_EUSART0, true);

    CMU_ClockSelectSet(cmuClock_EM01GRPCCLK, cmuSelect_HFXO);
    CMU_ClockSelectSet(cmuClock_EUSART0, cmuSelect_EM01GRPCCLK);

    EUSART_UartInit_TypeDef EUSART_init = EUSART_UART_INIT_DEFAULT_HF;
    EUSART_init.enable = eusartDisable;
    EUSART_init.baudrate = baudrate;
    EUSART_init.oversampling = EUSART_CFG0_OVS_X4;
    EUSART_UartInitHf(EUSART0, &EUSART_init);

    gpio_port_t eusart_rx_port, eusart_tx_port;
    gpio_pin_t eusart_rx_pin, eusart_tx_pin;
    Gpio_getPin(BOARD_GPIO_ID_USART_RX, &eusart_rx_port, &eusart_rx_pin);
    Gpio_getPin(BOARD_GPIO_ID_USART_TX, &eusart_tx_port, &eusart_tx_pin);

    GPIO->EUSARTROUTE[0].ROUTEEN = GPIO_EUSART_ROUTEEN_TXPEN;
    GPIO->EUSARTROUTE[0].TXROUTE = (eusart_tx_port << _GPIO_EUSART_TXROUTE_PORT_SHIFT) | (eusart_tx_pin << _GPIO_EUSART_TXROUTE_PIN_SHIFT);
    GPIO->EUSARTROUTE[0].RXROUTE = (eusart_rx_port << _GPIO_EUSART_RXROUTE_PORT_SHIFT) | (eusart_rx_pin << _GPIO_EUSART_RXROUTE_PIN_SHIFT);

    EUSART_IntDisable(EUSART0, _EUSART_IF_MASK);
    EUSART_IntClear(EUSART0, _EUSART_IF_MASK);

    // APP IRQ
    Sys_clearFastAppIrq(EUSART0_TX_IRQn);
    Sys_enableFastAppIrq(EUSART0_TX_IRQn,
                         APP_LIB_SYSTEM_IRQ_PRIO_HI,
                         EUSART_TX_IRQHandler);

    EUSART_Enable(EUSART0, eusartEnableTx);

    CMU_ClockEnable(cmuClock_EUSART0, false);

    return true;
}

void Usart_setEnabled(bool enabled)
{
    const gpio_out_cfg_t eusart_tx_enable_cfg =
    {
        .out_mode_cfg = GPIO_OUT_MODE_PUSH_PULL,
        .level_default = GPIO_LEVEL_HIGH
    };
    const gpio_in_cfg_t eusart_tx_disable_cfg =
    {
        .event_cb = NULL,
        .event_cfg = GPIO_IN_EVENT_NONE,
        .in_mode_cfg = GPIO_IN_PULL_UP
    };

    Sys_enterCriticalSection();
    if(enabled)
    {
        // Detect if someone is enabling UART but not disabling it ever
        if(m_enabled == 0)
        {
            // Enable clock
            CMU_ClockEnable(cmuClock_EUSART0, true);
            // Disable deep sleep
            DS_Disable(DS_SOURCE_USART);
            // Set output
            Gpio_outputSetCfg(BOARD_GPIO_ID_USART_TX, &eusart_tx_enable_cfg);
        }
        m_enabled++;
    }
    else
    {
        if (m_enabled > 0)
        {
            m_enabled--;
        }
        if(m_enabled == 0)
        {
            // Set light pullup
            Gpio_inputSetCfg(BOARD_GPIO_ID_USART_TX, &eusart_tx_disable_cfg);
            // Enable deep sleep
            DS_Enable(DS_SOURCE_USART);
            // Disable clock
            CMU_ClockEnable(cmuClock_EUSART0, false);
        }
    }
    Sys_exitCriticalSection();
}

void Usart_receiverOn(void)
{
    EUSART_Enable(EUSART0, eusartEnable);
}

void Usart_receiverOff(void)
{
    EUSART_Enable(EUSART0, eusartEnableTx);
}

bool Usart_setFlowControl(uart_flow_control_e flow)
{
    (void)flow;
    return false;
}

uint32_t Usart_sendBuffer(const void * buffer, uint32_t length)
{
    bool empty = false;
    uint32_t size_in = length;
    uint8_t * data_out = (uint8_t *)buffer;
    Sys_enterCriticalSection();
    if (Ringbuffer_free(m_usart_tx_buffer) < length)
    {
        size_in = 0;
        goto buffer_too_large;
    }
    empty = (Ringbuffer_usage(m_usart_tx_buffer) == 0);
    while(length--)
    {
        Ringbuffer_getHeadByte(m_usart_tx_buffer) = *data_out++;
        Ringbuffer_incrHead(m_usart_tx_buffer, 1);
    }
    if (empty)
    {
        Usart_setEnabled(true);
        EUSART_IntEnable(EUSART0, EUSART_IF_TXC);
        while ((EUSART_StatusGet(EUSART0) & EUSART_STATUS_TXFL) && (Ringbuffer_usage(m_usart_tx_buffer) != 0))
        {
            EUSART_Tx(EUSART0, Ringbuffer_getTailByte(m_usart_tx_buffer));
            Ringbuffer_incrTail(m_usart_tx_buffer, 1);
        }
        m_tx_active = true;
    }
buffer_too_large:
    Sys_exitCriticalSection();
    return size_in;
}

void Usart_enableReceiver(serial_rx_callback_f rx_callback)
{
    gpio_in_cfg_t usart_rx_cfg =
    {
        .event_cb = NULL,
        .event_cfg = GPIO_IN_EVENT_NONE,
        .in_mode_cfg = GPIO_IN_DISABLED
    };

    uint32_t __attribute__((unused))dummy;
    Sys_enterCriticalSection();
    // Set callback
    m_rx_callback = rx_callback;
    // Enable clock
    CMU_ClockEnable(cmuClock_EUSART0, true);
    if(rx_callback)
    {
        Sys_enableFastAppIrq(EUSART0_RX_IRQn,
                                APP_LIB_SYSTEM_IRQ_PRIO_HI,
                                EUSART_RX_IRQHandler);

        EUSART_IntEnable(EUSART0, EUSART_IF_RXFL);

        // Set light pull-up resistor
        usart_rx_cfg.in_mode_cfg = GPIO_IN_PULL_UP;
        Gpio_inputSetCfg(BOARD_GPIO_ID_USART_RX, &usart_rx_cfg);
    }
    else
    {
        Sys_disableAppIrq(EUSART0_RX_IRQn);

        EUSART_IntDisable(EUSART0, EUSART_IF_RXFL);

        // Disable input & input pull-up resistor
        usart_rx_cfg.in_mode_cfg = GPIO_IN_DISABLED;
        Gpio_inputSetCfg(BOARD_GPIO_ID_USART_RX, &usart_rx_cfg);
    }
    dummy = EUSART0->RXDATA;
    EUSART_IntClear(EUSART0, EUSART_IF_RXFL);
    CMU_ClockEnable(cmuClock_EUSART0, false);
    Sys_clearFastAppIrq(EUSART0_RX_IRQn);
    Sys_exitCriticalSection();
}

uint32_t Usart_getMTUSize(void)
{
    return BUFFER_SIZE;
}

void Usart_flush(void)
{
    volatile uint32_t timeout = 20000;
    while(m_tx_active && timeout > 0)
    {
        timeout--;
    }
}

void __attribute__((__interrupt__)) EUSART_RX_IRQHandler(void)
{
    // Data received
    uint16_t ch = EUSART0->RXDATA;
    // RXFULL must be explicitly cleared
    EUSART_IntClear(EUSART0, EUSART_IF_RXFL);

    if (m_rx_callback != NULL)
    {
        m_rx_callback((uint8_t *) &ch, 1);
    }
}

void __attribute__((__interrupt__)) EUSART_TX_IRQHandler(void)
{
    EUSART_IntClear(EUSART0, EUSART_IF_TXC);

    if (Ringbuffer_usage(m_usart_tx_buffer) == 0)
    {
        // when buffer becomes empty, reset indexes
        EUSART_IntDisable(EUSART0, EUSART_IF_TXC);
        Usart_setEnabled(false);
        m_tx_active = false;
        return;
    }

    while ((EUSART_StatusGet(EUSART0) & EUSART_STATUS_TXFL) && (Ringbuffer_usage(m_usart_tx_buffer) != 0))
    {
        EUSART_Tx(EUSART0, Ringbuffer_getTailByte(m_usart_tx_buffer));
        Ringbuffer_incrTail(m_usart_tx_buffer, 1);
    }
}
