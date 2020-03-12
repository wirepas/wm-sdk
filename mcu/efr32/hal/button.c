/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    button.c
 * \brief   Board-specific button module for efr32
 */

#include "button.h"
#include "mcu.h"
#include "board.h"
#include "api.h"


void Button_init()
{
    // Not implemented yet
}

button_res_e Button_getState(uint8_t button_id, bool * state_p)
{
    // Not implemented yet
    return BUTTON_RES_INVALID_ID;
}

button_res_e Button_register_for_event(uint8_t button_id,
                                       button_event_e event,
                                       on_button_event_cb cb)
{
    return BUTTON_RES_INVALID_ID;
}

uint8_t Button_get_number()
{
    return 0;
}
