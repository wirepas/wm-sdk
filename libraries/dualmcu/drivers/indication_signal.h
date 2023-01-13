/* Copyright 2022 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file    indication_signal.h
 * \brief   Used to generate the Interrupt ReQuest ("IRQ") signal to notify the application that it has one or more pending indications.
 */

#ifndef INDICATION_SIGNAL_
#define INDICATION_SIGNAL_

/**
 * \brief   Enables indication signal (UART IRQ) pin
 */
void IndicationSignal_enable(void);

/**
 * \brief   Sets (asserts) indication signal (UART IRQ) pin
 */
void IndicationSignal_set(void);

/**
 * \brief   Clears (de-asserts) indication signal (UART IRQ) pin
 */
void IndicationSignal_clear(void);

#endif /* INDICATION_SIGNAL_ */
