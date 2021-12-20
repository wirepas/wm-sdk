/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef EXTI_H_
#define EXTI_H_

#include "api.h"

/**
 * \brief   Initialize IO's to a known state
 *
 */
void Io_init(void);

/**
 * \brief   Enables UART IRQ pin
 */
void Io_enableUartIrq(void);

/**
 * \brief   Sets (asserts) UART IRQ pin
 */
void Io_setUartIrq(void);

/**
 * \brief   Clears (de-asserts) UART IRQ pin
 */
void Io_clearUartIrq(void);

typedef enum
{
    EXTI_IRQ_RISING_EDGE    = 0x01,
    EXTI_IRQ_FALLING_EDGE   = 0x10,
    EXTI_IRQ_STATE_CHANGE   = EXTI_IRQ_RISING_EDGE | EXTI_IRQ_FALLING_EDGE
}exti_irq_config_e;

/**
 * \brief   Mandatory callback for EXTI events
 */
typedef void (*wakeup_cb_f)(void);

/**
 * \brief   Setup wake-up pin
 * \param   cb
 *          Mandatory callback that is invoked on event
 * \post    Pin (EXTI channel) is fired and waiting for a falling edge trigger
 */
void Wakeup_pinInit(wakeup_cb_f cb);

/**
 * \brief   Disable wake-up functionality.
 */
void Wakeup_off(void);

/**
 * \brief   Clear interrupt source flag
 */
void Wakeup_clearIrq(void);

/**
 * \brief   Change edge for wake-up pin
 * \param   edge
 *          Edge type to configure
 * \param   enable
 *          Enable pin interrrupt
 */
void Wakeup_setEdgeIRQ(exti_irq_config_e edge, bool enable);
#endif /* EXTI_H_ */
