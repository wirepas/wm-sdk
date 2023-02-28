/* Copyright 2022 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file    gpio_weak.c
 * \brief   Definitions of gpio weak functions.
 * \attention Should be compatible with the gpio.h interface.
 */

#include "gpio.h"

gpio_res_e  __attribute__((weak))
        Gpio_init(void)
{
    return GPIO_RES_NOT_IMPLEMENTED;
}

gpio_res_e __attribute__((weak))
        Gpio_inputSetCfg(gpio_id_t id, const gpio_in_cfg_t *in_cfg)
{
    (void) id;
    (void) in_cfg;

    return GPIO_RES_NOT_IMPLEMENTED;
}

gpio_res_e __attribute__((weak))
        Gpio_inputRead(gpio_id_t id, gpio_level_e *level)
{
    (void) id;
    (void) level;

    return GPIO_RES_NOT_IMPLEMENTED;
}

gpio_res_e __attribute__((weak))
        Gpio_outputSetCfg(gpio_id_t id, const gpio_out_cfg_t *out_cfg)
{
    (void) id;
    (void) out_cfg;

    return GPIO_RES_NOT_IMPLEMENTED;
}

gpio_res_e __attribute__((weak)) Gpio_outputWrite(gpio_id_t id, gpio_level_e level)
{
    (void) id;
    (void) level;

    return GPIO_RES_NOT_IMPLEMENTED;
}

gpio_res_e __attribute__((weak))
        Gpio_outputToggle(gpio_id_t id)
{
    (void) id;

    return GPIO_RES_NOT_IMPLEMENTED;
}

gpio_res_e __attribute__((weak))
        Gpio_outputRead(gpio_id_t id, gpio_level_e *level)
{
    (void) id;
    (void) level;

    return GPIO_RES_NOT_IMPLEMENTED;
}

gpio_res_e __attribute__((weak))
        Gpio_getPin(gpio_id_t id, gpio_port_t *port, gpio_pin_t *pin)
{
    (void) id;
    (void) port;
    (void) pin;

    return GPIO_RES_NOT_IMPLEMENTED;
}

uint8_t __attribute__((weak))
        Gpio_getNumber(void)
{
    return 0;
}
