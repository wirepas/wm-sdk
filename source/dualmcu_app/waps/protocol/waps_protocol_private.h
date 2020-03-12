/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef WAPS_PROTOCOL_PRIVATE_H_
#define WAPS_PROTOCOL_PRIVATE_H_

bool prot_send_item(waps_item_t * item);

/** Current reply frame */
extern waps_item_t *    prot_reply;

/** Last indication information */
extern waps_item_t *    prot_indication;
extern uint8_t          prot_seq;

#endif /* WAPS_PROTOCOL_PRIVATE_H_ */
