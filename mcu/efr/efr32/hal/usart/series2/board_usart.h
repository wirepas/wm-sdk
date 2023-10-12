/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef BOARD_USART_H_
#define BOARD_USART_H_

// Map some registers constant to the USART selected
#if BOARD_USART_ID == 0
#define BOARD_USART         USART0
#define BOARD_USART_CMU_BIT CMU_CLKEN0_USART0
#define BOARD_USARTINPUT    PRS->CONSUMER_USART0_RX
#define BOARD_USARTROUTE    GPIO->USARTROUTE[0]
#define BOARD_UART_RX_IRQn  USART0_RX_IRQn
#define BOARD_UART_TX_IRQn  USART0_TX_IRQn
#define BOARD_UART_LDMA_RX  ldmaPeripheralSignal_USART0_RXDATAV
#define BOARD_UART_LDMA_TX  ldmaPeripheralSignal_USART0_TXEMPTY
#elif BOARD_USART_ID == 1
#define BOARD_USART         USART1
#define BOARD_USART_CMU_BIT CMU_CLKEN0_USART1
#define BOARD_USARTINPUT    PRS->CONSUMER_USART1_RX
#define BOARD_USARTROUTE    GPIO->USARTROUTE[1]
#define BOARD_UART_RX_IRQn  USART1_RX_IRQn
#define BOARD_UART_TX_IRQn  USART1_TX_IRQn
#define BOARD_UART_LDMA_RX  ldmaPeripheralSignal_USART1_RXDATAV
#define BOARD_UART_LDMA_TX  ldmaPeripheralSignal_USART1_TXEMPTY
#else // BOARD_USART_ID
#error USART ID must be 0 or 1
#endif // BOARD_USART_ID
#define BOARD_USART_ROUTE   BOARD_USARTROUTE.RXROUTE = BOARD_USART_RX_PIN << _GPIO_USART_RXROUTE_PIN_SHIFT | \
                                                       BOARD_USART_RX_PORT << _GPIO_USART_RXROUTE_PORT_SHIFT; \
                            BOARD_USARTROUTE.TXROUTE = BOARD_USART_TX_PIN << _GPIO_USART_RXROUTE_PIN_SHIFT | \
                                                       BOARD_USART_TX_PORT << _GPIO_USART_RXROUTE_PORT_SHIFT
#define BOARD_USART_PINS    BOARD_USARTROUTE.ROUTEEN = GPIO_USART_ROUTEEN_RXPEN | \
                                                       GPIO_USART_ROUTEEN_TXPEN
#define BOARD_USART_CLR_IRQ_ALL       BOARD_USART->IF_CLR = _USART_IF_MASK
#define BOARD_USART_CLR_IRQ_RXFULL    BOARD_USART->IF_CLR = USART_IF_RXFULL
#define BOARD_USART_CLR_IRQ_TXC       BOARD_USART->IF_CLR = _USART_IF_TXC_MASK
#define BOARD_USART_CLR_IRQ_TCMP1     BOARD_USART->IF_CLR = USART_IF_TCMP1
#define BOARD_USART_LDMA_CLR_IRQ      LDMA->IF_CLR
#define BOARD_USART_LDMA_ENABLE       LDMA->EN = LDMA_EN_EN // TODO: Change to LDMA_Init()
#define BOARD_USART_LDMA_DISABLE      LDMA->EN = 0          // TODO: Change to LDMA_DeInit()
#if (_SILICON_LABS_32B_SERIES_2_CONFIG == 1)
#define BOARD_USART_ENABLE_USART_CLK
#define BOARD_USART_DISABLE_USART_CLK
#define BOARD_USART_ENABLE_LDMA_CLK
#define BOARD_USART_DISABLE_LDMA_CLK
#define BOARD_USART_ENABLE_GPIO_CLK
#define BOARD_USART_DISABLE_GPIO_CLK
#else
#define BOARD_USART_ENABLE_USART_CLK  CMU->CLKEN0_SET = BOARD_USART_CMU_BIT
#define BOARD_USART_DISABLE_USART_CLK CMU->CLKEN0_CLR = BOARD_USART_CMU_BIT
#define BOARD_USART_ENABLE_LDMA_CLK   CMU->CLKEN0_SET = CMU_CLKEN0_LDMA | CMU_CLKEN0_LDMAXBAR
#define BOARD_USART_DISABLE_LDMA_CLK  CMU->CLKEN0_CLR = CMU_CLKEN0_LDMA | CMU_CLKEN0_LDMAXBAR
#define BOARD_USART_ENABLE_GPIO_CLK   CMU->CLKEN0_SET = CMU_CLKEN0_GPIO | CMU_CLKEN0_PRS
#define BOARD_USART_DISABLE_GPIO_CLK  CMU->CLKEN0_CLR = CMU_CLKEN0_GPIO | CMU_CLKEN0_PRS
#endif // (_SILICON_LABS_32B_SERIES_2_CONFIG == 1)

#endif // BOARD_USART_H_
