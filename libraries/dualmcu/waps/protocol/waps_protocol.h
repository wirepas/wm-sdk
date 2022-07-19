/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef WAPS_PROTOCOL_H_
#define WAPS_PROTOCOL_H_

#include <stdint.h>
#include <stdbool.h>

#include "waps_item.h"
#include "sl_list.h"

// Callback for incoming item
typedef void (*waps_request_receive_f)(waps_item_t * item);

// Define lower layer directives as function pointers
// This way output can easily be selected on boot
typedef struct
{
    void                                (*send_reply)(void);
    bool                                (*write_hw)(const void *, uint32_t);
    void                                (*frame_removed)(void);
    bool                                (*process_response)(waps_item_t *);
    void                                (*update_irq)(bool);
    void                                (*flush_hw)(void);
}waps_prot_t;

extern waps_prot_t                      waps_prot;

// Handy access to lower layer via macros
#define Waps_prot_sendReply()           waps_prot.send_reply()
#define Waps_hw_write(a,b)              waps_prot.write_hw(a,b)
#define Waps_prot_frameRemoved()        waps_prot.frame_removed()
#define Waps_prot_processResponse(x)    waps_prot.process_response(item)
#define Waps_prot_updateIrq(x)          waps_prot.update_irq(x)
#define Waps_prot_flush_hw()            waps_prot.flush_hw();

void Waps_force_send_indication();

/**
 * \brief   Initialize WAPS protocol layer and low level layer
 * \return  True if successful, false otherwise
 */
bool Waps_prot_init(waps_request_receive_f cb, uint32_t baudrate, bool flow_ctrl);

/**
 * \brief   Check if protocol layer has indication stashed
 */
bool Waps_prot_hasIndication(void);

/**
 * \brief   Update IRQ pin state depending on queued indications
 */
void Waps_prot_updateIrqPin(void);

/** WAPS Global queues */
extern sl_list_head_t                   waps_ind_queue;
extern sl_list_head_t                   waps_reply_queue;

#endif /* WAPS_PROTOCOL_H_ */
