/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef WAPS_UART_PROTOCOL_H_
#define WAPS_UART_PROTOCOL_H_

#include <stdbool.h>
#include "waps_item.h"

/**
 * \brief   (Re-)Send reply
 */
void Waps_protUart_sendReply(void);

/**
 * \brief   Tell protocol that a frame has been removed (invalid)
 */
void Waps_protUart_frameRemoved(void);

/**
 * \brief   Process response and send indication if necessary
 * \return  true
 *          Frame is a response frame
 *          false
 *          Frame is not a response frame
 */
bool Waps_protUart_processResponse(waps_item_t * item);

#endif /* WAPS_UART_PROTOCOL_H_ */
